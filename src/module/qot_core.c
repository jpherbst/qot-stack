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
#include <linux/posix-clock.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/rbtree.h>
#include <linux/idr.h>
#include <linux/timecounter.h>

// This file includes
#include "qot.h"
#include "qot_core.h"

// General options
#define QOT_MAX_ADJUSTMENT  (1000000)

// These are the data structures for the /devqot character device
static dev_t dev;
static struct cdev c_dev;
static struct class *cl;
static struct device *dev_ret;

// These are the data structures for the /dev/timeline character devices
static dev_t qot_clock_devt;
static struct class *qot_clock_class;

// Stores information about a timeline
struct qot_timeline {
    char uuid[QOT_MAX_NAMELEN];			// Unique id for this timeline
    struct rb_node node;				// Red-black tree is used to store timelines on UUID
    struct list_head head_acc;			// Head pointing to maximum accuracy structure
    struct list_head head_res;			// Head pointing to maximum resolution structure
    struct qot_metric actual;			// The actual accuracy/resolution
	struct posix_clock clock;			// Clock: POSIX interface
	int index;							// Clock: integer index (the X in /dev/timelineX)
	dev_t devid;						// Clock: device id
	struct device *dev;					// clock: device pointer
	int32_t dialed_frequency; 			// Discipline: dialed frequency
	uint32_t cc_mult; 					// Discipline: mult carry
	uint64_t last; 						// Discipline: last cycle count of discipline
	int64_t mult; 						// Discipline: ppb multiplier for errors
	int64_t nsec; 						// Discipline: global time offset
};

// Data structure for storing captures
struct qot_capture_item {
	struct qot_capture data;
	struct list_head list;
};

// Data structure for storing events
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
	int capture_flag;					// Data ready flag (captures)
	int event_flag;						// Data ready flag (events)
	struct list_head capture_list;		// Pointer to capture data
	struct list_head event_list;		// Pointer to event data
};

// Use the IDR mechanism to dynamically allocate clock ids and find them quickly
static struct idr idr_clocks;

// A red-black tree designed to quickly find a timeline based on its UUID
static struct rb_root timeline_root = RB_ROOT;

// A red-black tree designed to quickly find a binding based on its fileobject
static struct rb_root binding_root = RB_ROOT;

// The implementation of the driver
static struct qot_driver *driver = NULL;

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

// Search our data structure for a given timeline
static struct qot_timeline *qot_timeline_search(struct rb_root *root, const char *uuid)
{
	int result;
	struct qot_timeline *timeline;
	struct rb_node *node = root->rb_node;
	while (node)
	{
		timeline = container_of(node, struct qot_timeline, node);
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
		timeline = container_of(*new, struct qot_timeline, node);
		result = strcmp(data->uuid, timeline->uuid);
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
	// Failure
	if (!binding)
		return 1;

	// Delete the binding fromt the timeline list
	list_del(&binding->res_list);
	list_del(&binding->acc_list);

	// If we have removed the last binding from a timeline, then the lists
	// associated with the timeline should both be empty
	if (	list_empty(&binding->timeline->head_acc) 
		&&  list_empty(&binding->timeline->head_res))
	{
		// Unregister the QoT clock
		device_destroy(qot_clock_class, binding->timeline->devid);

		// Unregister clock
		posix_clock_unregister(&binding->timeline->clock);

		// Remove the binding ID form the list
		idr_remove(&idr_clocks, binding->timeline->index);

		// Remove the timeline node from the red-black tree
		rb_erase(&binding->timeline->node, &timeline_root);

		// Free the timeline entry memory
		kfree(binding->timeline);
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

static int qot_adjfreq(struct qot_timeline *timeline, int32_t ppb)
{
	int64_t core_t, core_n;
	core_t = driver->read();
	core_n = core_t - timeline->last;
	timeline->nsec += (core_n + timeline->mult * core_n);
	timeline->mult += ppb;
	timeline->last = core_t;
	return 0;
}

static int qot_adjtime(struct qot_timeline *timeline, int64_t delta)
{
	timeline->nsec += delta;
	return 0;
}

static int qot_gettime(struct qot_timeline *timeline, struct timespec64 *ts)
{
	int64_t core_n = driver->read() - timeline->last;
	*ts = ns_to_timespec64(timeline->nsec + core_n + timeline->mult * core_n);
	return 0;
}

static int qot_settime(struct qot_timeline *timeline, const struct timespec64 *ts)
{
	timeline->last = driver->read();
	timeline->nsec = timespec64_to_ns(ts);
	return 0;
}

static int qot_loc2rem(struct qot_timeline *timeline, int period, int64_t *val)
{
	if (period)
		*val += (*val * timeline->mult);
	else
	{
		*val -= (int64_t) timeline->last;
		*val  = timeline->nsec + *val + *val * timeline->mult;
	}
	return 0;
}

static int qot_rem2loc(struct qot_timeline *timeline, int period, int64_t *val)
{
	if (period)
		*val += (*val * timeline->mult);
	else
	{
		*val -= (int64_t) timeline->last;
		*val  = timeline->nsec + *val + *val * timeline->mult;
	}
	return 0;
}

// POSIX CLOCK MANAGEMENT //////////////////////////////////////////////////////

static int qot_clock_open(struct posix_clock *pc, fmode_t fmode)
{
	return 0;
}

static int qot_clock_close(struct posix_clock *pc)
{
	return 0;
}

static long qot_clock_ioctl(struct posix_clock *pc, unsigned int cmd, unsigned long arg)
{
	struct qot_message msg;
	struct qot_event event;
	struct qot_event_item *event_item;
	struct qot_binding *binding;
	struct qot_timeline *timeline = container_of(pc, struct qot_timeline, clock);
	switch (cmd)
	{

	// Set the actual accuracy and resolution that was achieved by the synchronization service
	case QOT_SET_ACTUAL_METRIC:
		if (copy_from_user(&timeline->actual, (struct qot_metric*)arg, sizeof(struct qot_metric)))
			return -EFAULT;
		break;

	// Get the actual accuracy and resolution that was achieved by the synchronization service
	case QOT_GET_ACTUAL_METRIC:
		if (copy_to_user((struct qot_metric*)arg, &timeline->actual, sizeof(struct qot_metric)))
			return -EFAULT;
		break;

	// Get the name of this timeline and the target metric
	case QOT_GET_INFORMATION:
		strcpy(msg.uuid, timeline->uuid);
		msg.request.acc = list_entry(timeline->head_res.next, struct qot_binding, res_list)->request.acc;
		msg.request.res = list_entry(timeline->head_res.next, struct qot_binding, res_list)->request.res;
		if (copy_to_user((struct qot_message*)arg, &msg, sizeof(struct qot_message)))
			return -EFAULT;
		break;

	// Push an event from the synchronization service down to all bindings to given UUID
	case QOT_SET_EVENT:
		if (copy_from_user(&event, (struct qot_event*)arg, sizeof(struct qot_event)))
			return -EFAULT;
		
		// Iterate over all bindings attached to this timeline
		list_for_each_entry(binding, &timeline->head_res, res_list)
		{
	  		// Allocate the memory to store the event for this binding
			event_item = kzalloc(sizeof(struct qot_event_item), GFP_KERNEL);
			if (!event_item)
				return -ENOMEM;

			// Copy over the name and data
			strcpy(event_item->data.name, event.name);
			event_item->data.type = event.type;

			// Add it to the binding queue for the binding
			list_add_tail(&event_item->list, &binding->event_list);

			// Poll the binding to show that there is some new capture data
			binding->event_flag = 1;
			wake_up_interruptible(&binding->wq);
		}

		break;

	default:
		return -ENOTTY;
		break;
	}
	return 0;
}

static int qot_clock_getres(struct posix_clock *pc, struct timespec *tp)
{
	tp->tv_sec  = 0;
	tp->tv_nsec = 1;
	return 0;
}

static int qot_clock_settime(struct posix_clock *pc, const struct timespec *tp)
{
	struct qot_timeline *timeline = container_of(pc, struct qot_timeline, clock);
	struct timespec64 ts = timespec_to_timespec64(*tp);
	return qot_settime(timeline, &ts);
}

static int qot_clock_gettime(struct posix_clock *pc, struct timespec *tp)
{
	struct qot_timeline *timeline = container_of(pc, struct qot_timeline, clock);
	struct timespec64 ts;
	int err;
	err = qot_gettime(timeline, &ts);
	if (!err)
		*tp = timespec64_to_timespec(ts);
	return err;
}

static int qot_clock_adjtime(struct posix_clock *pc, struct timex *tx)
{
	struct qot_timeline *timeline = container_of(pc, struct qot_timeline, clock);
	int err = -EOPNOTSUPP;
	if (tx->modes & ADJ_SETOFFSET)
	{
		struct timespec ts;
		ktime_t kt;
		int64_t delta;
		ts.tv_sec  = tx->time.tv_sec;
		ts.tv_nsec = tx->time.tv_usec;
		if (!(tx->modes & ADJ_NANO))
			ts.tv_nsec *= 1000;
		if ((unsigned long) ts.tv_nsec >= NSEC_PER_SEC)
			return -EINVAL;
		kt = timespec_to_ktime(ts);
		delta = ktime_to_ns(kt);
		err = qot_adjtime(timeline, delta);
	} 
	else if (tx->modes & ADJ_FREQUENCY)
	{
		int64_t ppb = 1 + tx->freq;
		ppb *= 125;
		ppb >>= 13;
		if (ppb > QOT_MAX_ADJUSTMENT || ppb < -QOT_MAX_ADJUSTMENT)
			return -ERANGE;
		err = qot_adjfreq(timeline, ppb);
		timeline->dialed_frequency = tx->freq;
	} 
	else if (tx->modes == 0)
	{
		tx->freq = timeline->dialed_frequency;
		err = 0;
	}

	return err;
}

static struct posix_clock_operations qot_clock_ops = {
    .owner          = THIS_MODULE,
    .clock_adjtime  = qot_clock_adjtime,
    .clock_gettime  = qot_clock_gettime,
    .clock_getres   = qot_clock_getres,
    .clock_settime  = qot_clock_settime,
    .ioctl          = qot_clock_ioctl,
    .open           = qot_clock_open,
    .release        = qot_clock_close,
};

static void qot_clock_delete(struct posix_clock *pc)
{
	// Don't do anything
}

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
	binding->capture_flag = 0;
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
		return -ENOMEM;
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
	pr_info("qot_core: qot_ioctl_access called\n");

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

	//////////////////////////////////////////////////////////////////////////////////
	////////// BIND TO A TIMELINE WITH A GIVEN ACCURACY AND RESOLUTION ///////////////
	//////////////////////////////////////////////////////////////////////////////////

	case QOT_BIND_TIMELINE:
	
		// Get the parameters passed into the ioctl
		if (copy_from_user(&msg, (struct qot_message*)arg, sizeof(struct qot_message)))
			return -EACCES;

		// Copy over data accuracy and resolution requirements
		binding->request.acc = msg.request.acc;
		binding->request.res = msg.request.res;
		binding->timeline = qot_timeline_search(&timeline_root, msg.uuid);

		// If the timeline does not exist, we need to create it
		if (!binding->timeline)
		{
			pr_info("Timeline not found!\n");

			// If we get to this point, there is no corresponding UUID
			binding->timeline = kzalloc(sizeof(struct qot_timeline), GFP_KERNEL);
			
			// Copy over the UUID
			strncpy(binding->timeline->uuid, msg.uuid, QOT_MAX_NAMELEN);
			
			// Get a new ID for the clock
			binding->timeline->index = idr_alloc(&idr_clocks, binding->timeline, 0, MINORMASK + 1, GFP_KERNEL);
			if (binding->timeline->index < 0)
			{
				kfree(binding->timeline);
				return -EACCES;
			}

			// Create the POSIX clock with the generated ID
			binding->timeline->clock.ops = qot_clock_ops;
			binding->timeline->clock.release = qot_clock_delete;	
			binding->timeline->devid = MKDEV(MAJOR(qot_clock_devt), binding->timeline->index);
			binding->timeline->dev = device_create(qot_clock_class, NULL, binding->timeline->devid, 
				binding->timeline, "timeline%d", binding->timeline->index);
			dev_set_drvdata(binding->timeline->dev, binding);
			if (posix_clock_register(&binding->timeline->clock, binding->timeline->devid))
			{
				kfree(binding->timeline);
				return -EACCES;
			}
			
			// Initialize list heads
			INIT_LIST_HEAD(&binding->timeline->head_acc);
			INIT_LIST_HEAD(&binding->timeline->head_res);

			// Add the timeline
			qot_timeline_insert(&timeline_root, binding->timeline);
		}
		else
		{
			pr_info("Timeline found!\n");
		}

		// In both cases, we must insert and sort the two linked lists
		qot_insert_list_acc(binding, &binding->timeline->head_acc);
		qot_insert_list_res(binding, &binding->timeline->head_res);

		// Save the clock index for passing back to the calling application
		msg.tid = binding->timeline->index;

		// Send back the data structure with the new binding and clock info
		if (copy_to_user((struct qot_message*)arg, &msg, sizeof(struct qot_message)))
			return -EACCES;

		break;

	//////////////////////////////////////////////////////////////////////////////////
	////////////////////////// UNBIND FROM THE TIMELINE //////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////

	case QOT_UNBIND_TIMELINE:

		// Get the parameters passed into the ioctl
		if (copy_from_user(&msg, (struct qot_message*)arg, sizeof(struct qot_message)))
			return -EACCES;

		// Try and remove this binding
		if (qot_binding_remove(binding))
			return -EACCES;

		break;

	//////////////////////////////////////////////////////////////////////////////////
	//////////////////// SET THE ACCURACY FOR THE TIMELINE ///////////////////////////
	//////////////////////////////////////////////////////////////////////////////////

	case QOT_SET_ACCURACY:

		// Get the parameters passed into the ioctl
		if (copy_from_user(&msg, (struct qot_message*)arg, sizeof(struct qot_message)))
			return -EACCES;
	
		// Copy over the accuracy, delete and re-add to sort
		binding->request.acc = msg.request.acc;
		list_del(&binding->acc_list);
		qot_insert_list_acc(binding, &binding->timeline->head_acc);

		break;

	//////////////////////////////////////////////////////////////////////////////////
	/////////////////// SET THE RESOLUTION FOR THE TIMELINE //////////////////////////
	//////////////////////////////////////////////////////////////////////////////////

	case QOT_SET_RESOLUTION:

		// Get the parameters passed into the ioctl
		if (copy_from_user(&msg, (struct qot_message*)arg, sizeof(struct qot_message)))
			return -EACCES;

		// Copy over the resolution, delete and re-add to sort
		binding->request.res = msg.request.res;
		list_del(&binding->res_list);
		qot_insert_list_res(binding, &binding->timeline->head_res);

		break;

	//////////////////////////////////////////////////////////////////////////////////
	/////////// SET A COMPARE ACTION ON A GIVEN HARDWARE TIMER PIN ///////////////////
	//////////////////////////////////////////////////////////////////////////////////

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

	//////////////////////////////////////////////////////////////////////////////////
	///////////////////// PULL ITEMS FROM THE CAPTURE LIST ///////////////////////////
	//////////////////////////////////////////////////////////////////////////////////

	case QOT_GET_CAPTURE:

		// We need to signal the user-space to stop pulling captures when there are no more left
		if (list_empty(&binding->capture_list))
		{
			binding->capture_flag = 0;
			return -EACCES;			
		}

		// Get the head of the capture queue
		capture_item = list_entry(binding->capture_list.next, struct qot_capture_item, list);

		// Copy to the return message
		memcpy(&msg.capture,&capture_item->data,sizeof(struct qot_capture));

		// Convert from a core (ns) to time (ns) along a timeline
		qot_loc2rem(binding->timeline, 0, &msg.capture.edge);

		// Free the memory used by this capture event
		list_del(&capture_item->list);
		kfree(capture_item);

		// Send back the data structure with the new binding and clock info
		if (copy_to_user((struct qot_message*)arg, &msg, sizeof(struct qot_message)))
			return -EACCES;

		break;

	//////////////////////////////////////////////////////////////////////////////////
	////////////////////// PULL ITEMS FROM THE EVENT LIST ////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////

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
		if (binding->capture_flag && !list_empty(&binding->capture_list))
			mask |= (POLLIN | POLLRDNORM);
		if (binding->event_flag && !list_empty(&binding->event_list))
			mask |= (POLLOUT | POLLWRNORM);
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

	// We must add the capture event to each qpplication listener queue
  	for (node = rb_first(&binding_root); node; node = rb_next(node))
  	{
  		// Get the binding container of this red-black tree node
  		binding = container_of(node, struct qot_binding, node);
  		if (!binding)
  		{
  			pr_info("qot_core: Cannot find binding\n");
  			continue;
  		}

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

		// Poll the binding to show that there is some new capture data
		binding->capture_flag = 1;
		wake_up_interruptible(&binding->wq);

		pr_info("qot_core: CAPTURE on %s of %lld\n",name, epoch);
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

	// Conveience code for allocating descriptor-like integer clock ids
	idr_init(&idr_clocks);
	
	// Create a character device at /dev/qot for interacting with timelines
    if ((ret = alloc_chrdev_region(&dev, 0, 1, "qot")) < 0)
        return ret;
    cdev_init(&c_dev, &qot_fops); 
    if ((ret = cdev_add(&c_dev, dev, 1)) < 0)
        return ret;
    if (IS_ERR(cl = class_create(THIS_MODULE, "qot")))
    {
        cdev_del(&c_dev);
        unregister_chrdev_region(dev, 1);
        return PTR_ERR(cl);
    }
    if (IS_ERR(dev_ret = device_create(cl, NULL, dev, NULL, "qot")))
    {
        class_destroy(cl);
        cdev_del(&c_dev);
        unregister_chrdev_region(dev, 1);
        return PTR_ERR(dev_ret);
    }

	// Create a device class for posix clock character devices
	qot_clock_class = class_create(THIS_MODULE, "timeline");
	if (IS_ERR(qot_clock_class))
		return PTR_ERR(qot_clock_class);
	ret = alloc_chrdev_region(&qot_clock_devt, 0, MINORMASK + 1, "timeline");
	if (ret < 0)
	{
		class_destroy(qot_clock_class);	
		return 1;
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

	// Remove the clock class
    class_destroy(qot_clock_class);
	unregister_chrdev_region(qot_clock_devt, MINORMASK + 1);

	// Allocates clock ids
	idr_destroy(&idr_clocks);
}

// Module definitions
module_init(qot_init);
module_exit(qot_cleanup);

// The line below enforces out code to be GPL, because of the POSIX license
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andrew Symington <asymingt@ucla.edu>");
MODULE_DESCRIPTION("QoT ioctl driver");