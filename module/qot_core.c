/*
 * @file qot_core.h
 * @brief Linux 4.1.x kernel module for creation anmd destruction of QoT timelines
 * @author Andrew Symington and Fatima Anwar 
 * 
 * Copyright (c) Regents of the University of California, 2015. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 * 	1. Redistributions of source code must retain the above copyright notice, 
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, 
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// Kernel functions
#include <linux/ptp_clock_kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/rbtree.h>
#include <linux/timecounter.h>

// This file includes
#include "qot.h"
#include "qot_core.h"

// These are the data structures for the /dev/qot character device
static dev_t dev;
static struct cdev c_dev;
static struct class *cl;
static struct device *dev_ret;

// Stores information about a timeline
struct qot_timeline {
	char uuid[QOT_MAX_NAMELEN];			// UUID
    struct rb_node node_uuid;			// Red-black tree is used to store timelines on UUID
	struct rb_node node_ptpi;			// Red-black tree is used to store timelines on PTP index
	struct ptp_clock_info info;			// PTP clock infomration
	struct ptp_clock *clock;			// PTP clock itself
	int index;							// Index of the clock
	spinlock_t lock; 					// Protects driver time registers
    struct list_head head_acc;			// Head pointing to maximum accuracy structure
    struct list_head head_res;			// Head pointing to maximum resolution structure
    struct qot_metric actual;			// The actual accuracy/resolution
	int32_t dialed_frequency; 			// Discipline: dialed frequency
	uint32_t cc_mult; 					// Discipline: mult carry
	uint64_t last; 						// Discipline: last cycle count of discipline
	int64_t mult; 						// Discipline: ppb multiplier for errors
	int64_t nsec; 						// Discipline: global time offset
};

// Data structure for storing capture events
struct qot_capture_item {
	struct qot_capture data;
	struct list_head list;
};

// Data structure for storing timeline events
struct qot_event_item {
	struct qot_event data;
	struct list_head list;
};

// Stores information about an application binding to a timeline
struct qot_binding {
	struct rb_node node;				// Red-black tree node
	struct qot_metric request;			// Requested accuracy and resolution 
    struct list_head res_list;			// Next resolution (ordered)
    struct list_head acc_list;			// Next accuracy (ordered)
    struct list_head list;				// Next binding (gives ability to iterate over all)
    struct qot_timeline* timeline;		// Parent timeline
	struct file *fileobject;			// File object 	 
	wait_queue_head_t wq;				// wait queue 	 
	int event_flag;						// Data ready flag (events)
	struct list_head capture_list;		// List of capture dat
	struct list_head event_list;		// List of events to process
};

// A red-black tree designed to quickly find a timeline based on its PTP index
static struct rb_root timindex_root = RB_ROOT;

// A red-black tree designed to quickly find a timeline based on its UUID
static struct rb_root timeline_root = RB_ROOT;

// A red-black tree designed to quickly find a binding based on its fileobject
static struct rb_root binding_root = RB_ROOT;

// The implementation of the driver
static struct qot_driver *driver = NULL;

// EVENT NOTIFICATION //////////////////////////////////////////////////////////////////

static int qot_push_event(struct qot_timeline *timeline, uint8_t type, const char *data)
{
	struct rb_node *node;
	struct qot_binding *binding;
	struct qot_event_item *event_item;

	if (!timeline)
	{
		pr_err("qot_core: tried to push event to a timeline that doesn't exsit");
		return 1;
	}

	// Yes, it would be more efficient to iterate over a res or acc list directly on a 
	// binding struct. However, since daemon bindings are not put on this list, they
	// would never be polled. This is obviously an issue.
  	for (node = rb_first(&binding_root); node; node = rb_next(node))
  	{
  		// Get the binding container of this red-black tree node
  		binding = container_of(node, struct qot_binding, node);
  		if (!binding || (binding->timeline != timeline))
  			continue;

		// Allocate the memory to store the event for this binding
		event_item = kzalloc(sizeof(struct qot_event_item), GFP_KERNEL);
		if (!event_item)
			return -ENOMEM;

		// Copy over the name and data
		event_item->data.type = type;
		if (data) 
			strncpy(event_item->data.info, data, QOT_MAX_DATALEN);

		// Add it to the binding queue for the binding
		list_add_tail(&event_item->list, &binding->event_list);

		// Poll the binding to show that there is some new capture data
		binding->event_flag = 1;
		wake_up_interruptible(&binding->wq);
	}

	return 0;
}

// DATA STRUCTURE MANAGEMENT FOR TIMELINES //////////////////////////////////////////////////////

// Insert a new binding into the accuracy list
static inline void qot_insert_list_acc(struct qot_binding *binding_list, struct list_head *head)
{
	struct qot_binding *bind_obj;
	list_for_each_entry(bind_obj, head, acc_list)
	{
		if (bind_obj->request.acc > binding_list->request.acc)
		{
			list_add_tail(&binding_list->acc_list, &bind_obj->acc_list);
			return;
		}
	}
	list_add_tail(&binding_list->acc_list, head);
}

// Insert a new binding into the resolution list
static inline void qot_insert_list_res(struct qot_binding *binding_list, struct list_head *head)
{
	struct qot_binding *bind_obj;
	list_for_each_entry(bind_obj, head, res_list)
	{
		if (bind_obj->request.res > binding_list->request.res)
		{
			list_add_tail(&binding_list->res_list, &bind_obj->res_list);
			return;
		}
	}
	list_add_tail(&binding_list->res_list, head);
}

// SEARCH FOR A TIMELINE BASED ON A UUID /////////////////////////////////////////////////////////

// Search our data structure for a given timeline
static struct qot_timeline *qot_timeline_search(struct rb_root *root, const char *uuid)
{
	int result;
	struct qot_timeline *timeline;
	struct rb_node *node = root->rb_node;
	while (node)
	{
		timeline = container_of(node, struct qot_timeline, node_uuid);
		result = strcmp(uuid, timeline->uuid);
		if (result < 0)
			node = node->rb_left;
		else if (result > 0)
			node = node->rb_right;
		else
			return timeline;
	}
	return NULL;
}

// Insert a timeline into our data structure
static int qot_timeline_insert(struct rb_root *root, struct qot_timeline *data)
{
	int result;
	struct qot_timeline *timeline;
	struct rb_node **new = &(root->rb_node), *parent = NULL;
	while (*new)
	{
		timeline = container_of(*new, struct qot_timeline, node_uuid);
		result = strcmp(data->uuid, timeline->uuid);
		parent = *new;
		if (result < 0)
			new = &((*new)->rb_left);
		else if (result > 0)
			new = &((*new)->rb_right);
		else
			return 0;
	}
	rb_link_node(&data->node_uuid, parent, new);
	rb_insert_color(&data->node_uuid, root);
	return 1;
}

// SEARCH FOR A TIMELINE BASED ON A CLOCK INDEX /////////////////////////////////////////////

// Search our data structure for a given timeline
static struct qot_timeline *qot_timindex_search(struct rb_root *root, int index)
{
	int result;
	struct qot_timeline *timeline;
	struct rb_node *node = root->rb_node;
	while (node)
	{
		timeline = container_of(node, struct qot_timeline, node_ptpi);
		result = index - timeline->index;
		if (result < 0)
			node = node->rb_left;
		else if (result > 0)
			node = node->rb_right;
		else
			return timeline;
	}
	return NULL;
}

// Insert a timeline into our data structure
static int qot_timindex_insert(struct rb_root *root, struct qot_timeline *data)
{
	int result;
	struct qot_timeline *timeline;
	struct rb_node **new = &(root->rb_node), *parent = NULL;
	while (*new)
	{
		timeline = container_of(*new, struct qot_timeline, node_ptpi);
		result = data->index - timeline->index;
		parent = *new;
		if (result < 0)
			new = &((*new)->rb_left);
		else if (result > 0)
			new = &((*new)->rb_right);
		else
			return 0;
	}
	rb_link_node(&data->node_ptpi, parent, new);
	rb_insert_color(&data->node_ptpi, root);
	return 1;
}

// SEARCH FOR A BINDING BASED ON A FILE OBJECT //////////////////////////////////////////////

static struct qot_binding *qot_binding_search(struct rb_root *root, struct file *fileobject)
{
	int result;
	struct qot_binding *binding;
	struct rb_node *node = root->rb_node;
	while (node)
	{
		binding = container_of(node, struct qot_binding, node);
		result = fileobject - binding->fileobject;
		if (result < 0)
			node = node->rb_left;
		else if (result > 0)
			node = node->rb_right;
		else
			return binding;
	}
	return NULL;
}

static int qot_binding_insert(struct rb_root *root, struct qot_binding *data)
{
	int result;
	struct qot_binding *binding;
	struct rb_node **new = &(root->rb_node), *parent = NULL;
	while (*new)
	{
		binding = container_of(*new, struct qot_binding, node);
		result = data->fileobject - binding->fileobject;
		parent = *new;
		if (result < 0)
			new = &((*new)->rb_left);
		else if (result > 0)
			new = &((*new)->rb_right);
		else
			return 0;
	}
	rb_link_node(&data->node, parent, new);
	rb_insert_color(&data->node, root);
	return 1;
}

static int qot_binding_remove(struct qot_binding *binding)
{
	// Failure to find either the binding ID or the timeline ID is a problem 
	if (!binding)
	{
		pr_info("qot_core: could not find binding\n");
		return 1;
	}
	if (!binding->timeline)
	{
		pr_info("qot_core: could not determine timeline\n");
		return 0;
	}

	// We have to be careful when deleting the lists, because if the client has
	// not yet bound to a timeline (or it is a daemon) then these list data 
	// structures will be empty! We cannot deference an empty pointer!
	if (!binding->acc_list.prev || !binding->res_list.prev)
		return 0;

	// Delete this binding from the timeline list
	list_del(&binding->res_list);
	list_del(&binding->acc_list);

	// If we have removed the last binding from a timeline, then the lists
	// associated with the timeline should both be empty
	if (list_empty(&binding->timeline->head_acc))
	{
		// Unregister clock
		if (binding->timeline->clock)
			ptp_clock_unregister(binding->timeline->clock);

		// Remove the timeline node from both red-black trees
		rb_erase(&binding->timeline->node_uuid, &timeline_root);
		rb_erase(&binding->timeline->node_ptpi, &timindex_root);

		// Free the timeline entry memory
		kfree(binding->timeline);
	}
	else
	{
		// We only need to call an event update if there are more bindings...
		qot_push_event(binding->timeline, EVENT_UPDATE, NULL);
	}

	// Success
	return 0;
}

// CLOCK OPERATIONS //////////////////////////////////////////////////////////////////

/* 
	In the first instance we'll represent the relationship between the slave clock 
   	and the master clock in a linear way. If we had really high precision floating
   	point math then we could do something like this:
						
							    1.0 + error
    master time at last discipline  |
			    |					|	      core ns elapsed since last disciplined
		________|_________   _______|______	  _________________|________________
   		T = timeline->nsec + timeline->skew * (driver->read() - timeline->last);

	We'll reduce this to the following...

		T = timeline->nsec 
		  + 				  (driver->read() - timeline->last) // Number of ns
		  + timeline->mult  * (driver->read() - timeline->last)	// PPB correction 

*/

// BASIC TIME PROJECTION FUNCTIONS /////////////////////////////////////////////

static int qot_loc2rem(struct qot_timeline *timeline, int period, int64_t *val)
{
	if (period)
		*val += ((*val) * timeline->mult);
	else
	{
		*val -= (int64_t) timeline->last;
		*val  = timeline->nsec + (*val) + (*val) * timeline->mult;
	}
	return 0;
}

static int qot_rem2loc(struct qot_timeline *timeline, int period, int64_t *val)
{
	if (period)
		*val = div_u64((uint64_t)(*val), (uint64_t) (timeline->mult + 1));
	else
	{
		*val = timeline->last 
		     + div_u64((uint64_t)(*val - timeline->nsec), (uint64_t) (timeline->mult + 1));
	}
	return 0;
}

static int qot_clock_adjfreq(struct ptp_clock_info *ptp, s32 ppb)
{
	int64_t core_t, core_n;
	unsigned long flags;
	struct qot_timeline *timeline = container_of(ptp, struct qot_timeline, info);
	spin_lock_irqsave(&timeline->lock, flags);
	core_t = driver->read();
	core_n = core_t - timeline->last;
	timeline->nsec += (core_n + timeline->mult * core_n);
	timeline->mult += ppb;
	timeline->last = core_t;
	spin_unlock_irqrestore(&timeline->lock, flags);
	return 0;
}

static int qot_clock_adjtime(struct ptp_clock_info *ptp, s64 delta)
{
	int64_t core_t, core_n;
	unsigned long flags;
	struct qot_timeline *timeline = container_of(ptp, struct qot_timeline, info);
	spin_lock_irqsave(&timeline->lock, flags);
	core_t = driver->read();
	core_n = core_t - timeline->last;
	timeline->nsec += (core_n + timeline->mult * core_n) + delta;
	timeline->last = core_t;
	spin_unlock_irqrestore(&timeline->lock, flags);
	return 0;
}

static int qot_clock_gettime(struct ptp_clock_info *ptp, struct timespec64 *ts)
{
	uint64_t core_n;
	unsigned long flags;
	struct qot_timeline *timeline = container_of(ptp, struct qot_timeline, info);
	spin_lock_irqsave(&timeline->lock, flags);
	core_n = driver->read() - timeline->last;
	spin_unlock_irqrestore(&timeline->lock, flags);
	*ts = ns_to_timespec64(timeline->nsec + core_n + timeline->mult * core_n);
	return 0;
}

static int qot_clock_settime(struct ptp_clock_info *ptp,
			    const struct timespec64 *ts)
{
	uint64_t ns;
	unsigned long flags;
	struct qot_timeline *timeline = container_of(ptp, struct qot_timeline, info);
	ns = timespec64_to_ns(ts);
	spin_lock_irqsave(&timeline->lock, flags);
	timeline->last = driver->read();
	timeline->nsec = ns;
	spin_unlock_irqrestore(&timeline->lock, flags);
	return 0;
}

static int qot_clock_enable(struct ptp_clock_info *ptp,
			   struct ptp_clock_request *rq, int on)
{
	return -EOPNOTSUPP;
}

// Basic PTP clock information
static struct ptp_clock_info qot_clock_info = {
	.owner		= THIS_MODULE,
	.name		= "QoT timer",
	.max_adj	= 1000000,
	.n_ext_ts	= 0,
	.n_pins		= 0,
	.pps		= 0,
	.adjfreq	= qot_clock_adjfreq,
	.adjtime	= qot_clock_adjtime,
	.gettime64	= qot_clock_gettime,
	.settime64	= qot_clock_settime,
	.enable		= qot_clock_enable,
};

// SCHEDULER IOCTL FUNCTIONALITY /////////////////////////////////////////////////////

static int qot_ioctl_open(struct inode *i, struct file *f)
{
	struct qot_binding *binding;
	pr_info("qot_core: qot_ioctl_open called\n");
	binding =kzalloc(sizeof(struct qot_binding), GFP_KERNEL);
	if (!binding)
		return -ENOMEM;
	
	// Initialize the capture list and polling structures for this ioctl channel
	INIT_LIST_HEAD(&binding->capture_list);
	INIT_LIST_HEAD(&binding->event_list);
	
	// INitialize polling data structures
	init_waitqueue_head(&binding->wq);
	binding->fileobject = f;
	binding->event_flag = 0;

	// Insert the binding into the red-black tree
	if (qot_binding_insert(&binding_root, binding))
    	return 0;

    // Only get here if there is a problem
    kfree(binding);
    return 0;
}

static int qot_ioctl_close(struct inode *i, struct file *f)
{
	struct qot_binding *binding;
	pr_info("qot_core: qot_ioctl_close called\n");
	binding = qot_binding_search(&binding_root, f);
	if (!binding)
	{
		pr_err("qot_core: could not find the bindng attached to ioctl\n");
		return -ENOMEM;
	}
	qot_binding_remove(binding);
	rb_erase(&binding->node, &binding_root);
	kfree(binding);
    return 0;
}

static long qot_ioctl_access(struct file *f, unsigned int cmd, unsigned long arg)
{
	struct qot_message msg;
	struct qot_binding *binding;
	struct qot_capture_item *capture_item;
	struct qot_event_item *event_item;
	struct timespec64 ts;
	int64_t ns;
	
	// Check that a hardware driver has registered itself with the QoT core
	binding = qot_binding_search(&binding_root, f);
	if (!binding)
	{
		pr_err("qot_core: qot_ioctl_access: binding not found\n");
		return -EACCES;
	}

	// Check that a hardware driver has registered itself with the QoT core
	if (!driver)
	{
		pr_err("qot_core: qot_ioctl_access: driver not found\n");
		return -EACCES;
	}

	// Check what message was sent
	switch (cmd)
	{

	//////////////////////////////////// API CALLS //////////////////////////////////////////////

	// API : bind to timeline
	case QOT_BIND_TIMELINE:
	
		// Get the parameters passed into the ioctl
		if (copy_from_user(&msg, (struct qot_message*)arg, sizeof(struct qot_message)))
			return -EACCES;

		// Special case: if a TID is specified, then it is the daemon calling...
		if (msg.tid < 0)
		{
			// Find the timeline based on the UUID
			binding->request.acc = msg.request.acc;
			binding->request.res = msg.request.res;
			binding->timeline = qot_timeline_search(&timeline_root, msg.uuid);
			if (!binding->timeline)
			{
				pr_info("qot_core: Timeline not found! Creating...\n");

				// If we get to this point, there is no corresponding UUID
				binding->timeline = kzalloc(sizeof(struct qot_timeline), GFP_KERNEL);
				binding->timeline->info = qot_clock_info;
				
				// Copy over the UUID to the PTP info
				strncpy(binding->timeline->uuid, msg.uuid, QOT_MAX_NAMELEN);

				// Create the PTP clock
				binding->timeline->clock = ptp_clock_register(&binding->timeline->info, dev_ret);
				if (IS_ERR(binding->timeline->clock))
				{
					binding->timeline->clock = NULL;
					return -EACCES;
				}

				// Get the index of the clock
				binding->timeline->index = ptp_clock_index(binding->timeline->clock);

				// Initialize the spin lock for driver time read() call
				spin_lock_init(&binding->timeline->lock);

				// Initialize list heads
				INIT_LIST_HEAD(&binding->timeline->head_acc);
				INIT_LIST_HEAD(&binding->timeline->head_res);

				// Add the timeline to both red-black trees
				qot_timeline_insert(&timeline_root, binding->timeline);
				qot_timindex_insert(&timindex_root, binding->timeline);

				// Notify all other bindings of this timeline creation
				qot_push_event(binding->timeline, EVENT_CREATE, NULL);
			}
			else
				pr_info("qot_core: Timeline found!\n");

			// In both cases, we must insert and sort the two linked lists
			qot_insert_list_acc(binding, &binding->timeline->head_acc);
			qot_insert_list_res(binding, &binding->timeline->head_res);

			// TODO: maybe optimize to not call event update if target did not change
			qot_push_event(binding->timeline, EVENT_UPDATE, NULL);
		}
		// This is a bind request from the daemon
		else
		{
			pr_info("qot_core: Daemon request for clock %d\n", msg.tid);

			// Find the binding based on the ptp clock index
			binding->timeline = qot_timindex_search(&timindex_root, msg.tid);
			if (!binding->timeline)
			{
				pr_info("qot_core: There is no timeline with corresponding clock %d\n", msg.tid);
				return -EACCES;
			}
		}

		// In both cases, copy the correct information and wors-case res and acc
		strncpy(msg.uuid,binding->timeline->uuid,QOT_MAX_NAMELEN);
		msg.tid = binding->timeline->index;
		msg.request.acc = list_entry(binding->timeline->head_acc.next, struct qot_binding, acc_list)->request.acc;
		msg.request.res = list_entry(binding->timeline->head_res.next, struct qot_binding, res_list)->request.res;

		// Send back the data structure with the new binding and clock info
		if (copy_to_user((struct qot_message*)arg, &msg, sizeof(struct qot_message)))
			return -EACCES;

		break;

	// API : unbind from timeline
	case QOT_UNBIND_TIMELINE:

		// Get the parameters passed into the ioctl
		if (copy_from_user(&msg, (struct qot_message*)arg, sizeof(struct qot_message)))
			return -EACCES;

		// Try and remove this binding
		if (qot_binding_remove(binding))
			return -EACCES;

		break;

	// API : update the accuract
	case QOT_SET_ACCURACY:

		// Get the parameters passed into the ioctl
		if (copy_from_user(&msg, (struct qot_message*)arg, sizeof(struct qot_message)))
			return -EACCES;
	
		// Copy over the accuracy, delete and re-add to sort
		binding->request.acc = msg.request.acc;
		list_del(&binding->acc_list);
		qot_insert_list_acc(binding, &binding->timeline->head_acc);

		break;

	// API : update the resolution
	case QOT_SET_RESOLUTION:

		// Get the parameters passed into the ioctl
		if (copy_from_user(&msg, (struct qot_message*)arg, sizeof(struct qot_message)))
			return -EACCES;

		// Copy over the resolution, delete and re-add to sort
		binding->request.res = msg.request.res;
		list_del(&binding->res_list);
		qot_insert_list_res(binding, &binding->timeline->head_res);

		break;

	// API : push a compare action
	case QOT_SET_COMPARE:
		
		// Get the parameters passed into the ioctl
		if (copy_from_user(&msg, (struct qot_message*)arg, sizeof(struct qot_message)))
			return -EACCES;

		// Check that we have a volid driver implmentation
		if (driver->compare)
		{
			// Convert from a core (ns) to time (ns) along a timeline
			qot_rem2loc(binding->timeline, 0, &msg.compare.start);
			qot_rem2loc(binding->timeline, 1, &msg.compare.high);
			qot_rem2loc(binding->timeline, 1, &msg.compare.low);

			// Check that the driver is willing to action this request
			if (driver->compare(msg.compare.name, msg.compare.enable, msg.compare.start, 
				msg.compare.high, msg.compare.low, msg.compare.repeat)) 
				return -EACCES;
		}

		break;

	// API : pull items from the capture list
	case QOT_GET_CAPTURE:

		// We need to signal the user-space to stop pulling captures when there are no more left
		if (list_empty(&binding->capture_list))
			return -EACCES;			
	
		// Get the head of the capture queue
		capture_item = list_entry(binding->capture_list.next, struct qot_capture_item, list);

		// Copy to the return message
		memcpy(&msg.capture, &capture_item->data, sizeof(struct qot_capture));

		// Convert from a core (ns) to time (ns) along a timeline
		qot_loc2rem(binding->timeline, 0, &msg.capture.edge);

		// Free the memory used by this capture event
		list_del(&capture_item->list);
		kfree(capture_item);

		// Send back the data structure with the new binding and clock info
		if (copy_to_user((struct qot_message*)arg, &msg, sizeof(struct qot_message)))
			return -EACCES;

		break;

	// API : pull items from the event list
	case QOT_GET_EVENT:

		// We need to signal the user-space to stop pulling captures when there are no more left
		if (list_empty(&binding->event_list))
		{
			binding->event_flag = 0;
			return -EACCES;			
		}

		// Get the head of the capture queue
		event_item = list_entry(binding->event_list.next, struct qot_event_item, list);

		// Copy to the return message
		memcpy(&msg.event,&event_item->data,sizeof(struct qot_event));

		// Free the memory used by this capture event
		list_del(&event_item->list);
		kfree(event_item);

		// Send back the data structure with the new binding and clock info
		if (copy_to_user((struct qot_message*)arg, &msg, sizeof(struct qot_message)))
			return -EACCES;

		break;

	//////////////////////////////////// DAEMON CALLS ///////////////////////////////////////////

	// Daemon : Get information about a timeline without actually binding to it (called by daemon)
	case QOT_GET_TARGET:

		// Set the information
		msg.request.acc = list_entry(binding->timeline->head_acc.next, struct qot_binding, acc_list)->request.acc;
		msg.request.res = list_entry(binding->timeline->head_res.next, struct qot_binding, res_list)->request.res;

		// Send back the data structure with the new binding and clock info
		if (copy_to_user((struct qot_message*)arg, &msg, sizeof(struct qot_message)))
			return -EACCES;

		break;

	// Daemon : Get information about a timeline without actually binding to it (called by daemon)
	case QOT_SET_ACTUAL:

		// Get the parameters passed into the ioctl
		if (copy_from_user(&msg, (struct qot_message*)arg, sizeof(struct qot_message)))
			return -EACCES;

		// mMake sure the timeline exists
		if (!binding->timeline)
			return -EACCES;

		// Update the performance metric
		binding->timeline->actual = msg.request;		

		// Iterate over all bindings attached to this timeline using the resolution list
		qot_push_event(binding->timeline, EVENT_SYNC, NULL);

		break;

	// Daemon : Get information about a timeline without actually binding to it (called by daemon)
	case QOT_PUSH_EVENT:

		// Get the parameters passed into the ioctl
		if (copy_from_user(&msg, (struct qot_message*)arg, sizeof(struct qot_message)))
			return -EACCES;

		// Determine the timeline based on the PTP clock id X (/dev/ptpX)
		if (!binding->timeline)
			return -EACCES;

		// Iterate over all bindings attached to this timeline using the resolution list
		qot_push_event(binding->timeline, msg.event.type, msg.event.info);

		break;

	///////////////////////////// USED BY LINUX PTP ///////////////////////////////////////////

	case QOT_PROJECT_TIME:

		// Get the parameters passed into the ioctl
		if (copy_from_user(&ts, (struct timespec*)arg, sizeof(struct timespec)))
			return -EACCES;

		// mMake sure the timeline exists
		if (!binding->timeline)
			return -EACCES;

		// Perform the projection into remore time
		ns = timespec64_to_ns(&ts);
		qot_loc2rem(binding->timeline, 0, &ns);
		ts = ns_to_timespec64(ns);

		// Send back the data structure with the updated timespec
		if (copy_to_user((struct timespec*)arg, &ts, sizeof(struct timespec)))
			return -EACCES;

		break;

	default:

		return -EINVAL;
	}

	// Success!
	return 0;
}

// Check to see if a capture event has occurred
static unsigned int qot_poll(struct file *f, poll_table *wait)
{
	unsigned int mask = 0;
	struct qot_binding *binding = qot_binding_search(&binding_root, f);
	if (binding)
	{
		poll_wait(f, &binding->wq, wait);
		if (binding->event_flag && !list_empty(&binding->event_list))
			mask |= POLLIN;
	}
	return mask;
}

// Define the file operations over th ioctl
static struct file_operations qot_fops = {
    .owner = THIS_MODULE,
    .open = qot_ioctl_open,
    .release = qot_ioctl_close,
    .unlocked_ioctl = qot_ioctl_access,
    .poll = qot_poll,
};

// EXPORTED FUNCTIONALITY ////////////////////////////////////////////////////////

// Register a clock source as the primary driver of time
int qot_register(struct qot_driver *d)
{
	if (driver)
		return 1;
	driver = d;
	return 0;
}
EXPORT_SYMBOL(qot_register);

// Pass a capture event from the paltform driver to the QoT core
int qot_push_capture(const char *name, int64_t epoch)
{
	struct rb_node *node;
	struct qot_capture_item *capevent;
	struct qot_binding *binding;
	struct qot_timeline *timeline;

	// We must add the capture event to each qpplication listener queue
  	for (node = rb_first(&binding_root); node; node = rb_next(node))
  	{
  		// Get the binding container of this red-black tree node
  		binding = container_of(node, struct qot_binding, node);
  		if (!binding)
  			continue;

  		// Allocate the memory to store the event for this binding
		capevent = kzalloc(sizeof(struct qot_capture_item), GFP_KERNEL);
		if (!capevent)
		{
			pr_info("qot_core: Cannot allocate memory\n");
			return -ENOMEM;
		}

		// Copy over the name and data
		strncpy(capevent->data.name, name, QOT_MAX_NAMELEN);
		capevent->data.edge = epoch;

		// Add it to the binding queue
		list_add_tail(&capevent->list, &binding->capture_list);
  	}

	// We must add the capture event to each qpplication listener queue
  	for (node = rb_first(&timeline_root); node; node = rb_next(node))
  	{
		timeline = container_of(node, struct qot_timeline, node_uuid);
  		if (!timeline)
  			continue;
		qot_push_event(timeline, EVENT_CAPTURE, NULL);
  	}

	return 0;
}
EXPORT_SYMBOL(qot_push_capture);

// Unregister the clock source
int qot_unregister(void)
{	
	// You cant unregister from something that is not already registered
	if (!driver)
		return 1;

	// Reset the driver
	driver = NULL;
	
	// Success!
	return 0;
}
EXPORT_SYMBOL(qot_unregister);

// MODULE LOAD AND UNLOAD ////////////////////////////////////////////////////////

int qot_init(void)
{
	int ret;
  	if ((ret = alloc_chrdev_region(&dev, 0, 1, QOT_IOCTL_QOT)) < 0)
        return ret;
    cdev_init(&c_dev, &qot_fops); 
    if ((ret = cdev_add(&c_dev, dev, 1)) < 0)
        return ret;
    if (IS_ERR(cl = class_create(THIS_MODULE, "char")))
    {
        cdev_del(&c_dev);
        unregister_chrdev_region(dev, 1);
        return PTR_ERR(cl);
    }
    if (IS_ERR(dev_ret = device_create(cl, NULL, dev, NULL, QOT_IOCTL_QOT)))
    {
        class_destroy(cl);
        cdev_del(&c_dev);
        unregister_chrdev_region(dev, 1);
        return PTR_ERR(dev_ret);
    }
	return 0;
} 

void qot_cleanup(void)
{
	// Remove the character device for ioctl
    device_destroy(cl, dev);
    class_destroy(cl);
    cdev_del(&c_dev);
    unregister_chrdev_region(dev, 1);
}

// Module definitions
module_init(qot_init);
module_exit(qot_cleanup);

// The line below enforces out code to be GPL, because of the POSIX license
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andrew Symington <asymingt@ucla.edu>");
MODULE_DESCRIPTION("QoT ioctl driver");