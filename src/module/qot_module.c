/*
 * @file qot_module.h
 * @brief Linux 4.1.x kernel module for creation anmd destruction of QoT timelines
 * @author Fatima Anwar 
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

#include <linux/version.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/hashtable.h>
#include <linux/dcache.h>
#include <linux/posix-clock.h>
#include <asm/uaccess.h>

// This module functionality
#include "qot_ioctl.h"

// Module information
#define MODULE_NAME "qot"
#define FIRST_MINOR 0
#define MINOR_CNT   1

// Stores information about a timeline
struct qot_timeline {
    char uuid[QOT_MAX_UUIDLEN];			// unique id for a timeline
    struct hlist_node collision_hash;	// pointer to next timeline for the same hash key
    struct list_head head_acc;			// head pointing to maximum accuracy structure
    struct list_head head_res;			// head pointing to maximum resolution structure
    struct qot_metric actual;			// achieved accuracy and resolution
    struct posix_clock clock;			// the posix clock 
    struct device *dev;					// parent device
    dev_t devid;						// device id
    int index;                      	// index into clocks.map
};

// Stores information about an application binding to a timeline
struct qot_binding {
	struct qot_metric request;			// requested accuracy and resolution 
    struct list_head res_list;			// Next resolution
    struct list_head acc_list;			// Next accuracy
    struct qot_timeline* timeline;		// Parent timeline
};
 
// Device class (scheduler)
static dev_t dev;					// Devices
static struct cdev c_dev;			// Character device
static struct class *cl;			// Class
static struct device *dev_ret;		// Device

// Device class (timelines)
static dev_t qot_clock_devt;
static struct class *qot_clock_class;

// An array of bindings (one for each application request)
static struct qot_binding *bindings[QOT_MAX_BINDINGS+1];

// The next available binding ID, as well as the total number alloacted
static int binding_next = 0;
static int binding_size = 0;

// A hash table designed to quickly find timeline based on UUID
DEFINE_HASHTABLE(qot_timelines_hash, QOT_HASHTABLE_BITS);

// Don't quite know why this is required
DEFINE_IDA(qot_clocks_map);

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
	struct qot_timeline *timeline = container_of(pc, struct qot_timeline, clock);
	struct qot_metric metric;
	switch (cmd)
	{
	case QOT_GET_ACTUAL_METRIC:
		if (copy_to_user((struct qot_metric*)arg, &timeline->actual, sizeof(struct qot_metric)))
			return -EFAULT;
		break;
	case QOT_GET_TARGET_METRIC:
		metric.acc = list_entry(timeline->head_acc.next, struct qot_binding, acc_list)->request.acc;
		metric.res = list_entry(timeline->head_res.next, struct qot_binding, res_list)->request.res;
		if (copy_to_user((struct qot_metric*)arg, &metric, sizeof(struct qot_metric)))
			return -EFAULT;
		break;
	case QOT_GET_UUID:
		if (copy_to_user((char*)arg, timeline->uuid, QOT_MAX_UUIDLEN*sizeof(char)))
			return -EFAULT;
		break;
	default:
		return -ENOTTY;
		break;
	}
	return 0;
}

static int qot_clock_getres(struct posix_clock *pc, struct timespec *tp)
{
	return 0;
}

static int qot_clock_settime(struct posix_clock *pc, const struct timespec *tp)
{
	return  0;
}

static int qot_clock_gettime(struct posix_clock *pc, struct timespec *tp)
{
	return 0;
}

static int qot_clock_adjtime(struct posix_clock *pc, struct timex *tx)
{
	return 0;
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
	struct qot_timeline *timeline = container_of(pc, struct qot_timeline, clock);
	ida_simple_remove(&qot_clocks_map, timeline->index);
}

static int qot_clock_register(struct qot_timeline* timeline)
{
	// Get the clock index
	timeline->index = ida_simple_get(&qot_clocks_map, 0, MINORMASK + 1, GFP_KERNEL);
	if (timeline->index < 0)
		return -1;

	// Set the clock
	timeline->clock.ops = qot_clock_ops;
	timeline->clock.release = qot_clock_delete;	
	timeline->devid = MKDEV(MAJOR(qot_clock_devt), timeline->index);

	// Create a new device in our class
	timeline->dev = device_create(qot_clock_class, NULL, timeline->devid, timeline, "timeline%d", timeline->index);
	if (IS_ERR(timeline->dev))
		return -2;

	// Set the driver data
	dev_set_drvdata(timeline->dev, timeline);

	// Create the POSIX clock
	if (posix_clock_register(&timeline->clock, timeline->devid))
		return -3;

	// Success
	return 0;
}

static int qot_clock_unregister(struct qot_timeline* timeline)
{
	// Release clock resources
	device_destroy(cl, timeline->devid);

	// Unregister clock
	posix_clock_unregister(&timeline->clock);

	return 0;
}

// DATA STRUCTURE MANAGEMENT ///////////////////////////////////////////////////

// Generate an unsigned int hash from a null terminated string
static unsigned int qot_hash_uuid(char *uuid)
{
	unsigned long hash = init_name_hash();
	while (*uuid)
		hash = partial_name_hash(*uuid++, hash);
	return end_name_hash(hash);
}

static inline void qot_add_acc(struct qot_binding *binding_list, struct list_head *head)
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

static inline void qot_add_res(struct qot_binding *binding_list, struct list_head *head)
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

static inline void qot_remove_binding(struct qot_binding *binding)
{
	// You cant interact will a null item
	if (!binding)
		return;

	// Remove the entries from the sort tables (ordering will be updated)
	list_del(&binding->res_list);
	list_del(&binding->acc_list);

	// If we have removed the last binding from a timeline, then the lists
	// associated with the timeline should both be empty
	if (	list_empty(&binding->timeline->head_acc) 
		&&  list_empty(&binding->timeline->head_res))
	{
		// Remove element from the hashmap
		hash_del(&binding->timeline->collision_hash);

		// Unregister the QoT clock
		qot_clock_unregister(binding->timeline);

		// Free the timeline entry memory
		kfree(binding->timeline);
	}

	// Free the memory used by this binding
	kfree(binding);

	// Set explicitly to NULL
	binding = NULL;
}

// SCHEDULER IOCTL FUNCTIONALITY /////////////////////////////////////////////////////

static int qot_ioctl_open(struct inode *i, struct file *f)
{
    return 0;
}
static int qot_ioctl_close(struct inode *i, struct file *f)
{
    return 0;
}

static long qot_ioctl_access(struct file *f, unsigned int cmd, unsigned long arg)
{
	struct qot_message msg;		// ioctl message
	struct qot_timeline *obj;	// timeline object
	unsigned int key;			// hash key

	switch (cmd)
	{

	case QOT_BIND_TIMELINE:
	
		// Get the parameters passed into the ioctl
		if (copy_from_user(&msg, (struct qot_message*)arg, sizeof(struct qot_message)))
			return -EACCES;

		// Allocate memory for the new binding
		bindings[binding_next] = kzalloc(sizeof(struct qot_binding), GFP_KERNEL);
		if (bindings[binding_next] == NULL)
			return -ENOMEM;

		// Find the next available binding (not necessarily contiguous)
		for (msg.bid = binding_next; binding_next <= QOT_MAX_BINDINGS; binding_next++)
			if (!bindings[binding_next])
				break;

		// Make sure we have the binding size
		if (binding_size < binding_next)
			binding_size = binding_next;		

		// Copy over data accuracy and resolution requirements
		bindings[msg.bid]->request.acc = msg.request.acc;
		bindings[msg.bid]->request.res = msg.request.res;

		// Generate a key for the UUID
		key = qot_hash_uuid(msg.uuid);

		// Presume non-existent until found
		bindings[msg.bid]->timeline = NULL;

		// Check to see if this hash element exists
		hash_for_each_possible(qot_timelines_hash, obj, collision_hash, key)
			if (!strcmp(obj->uuid, msg.uuid))
				bindings[msg.bid]->timeline = obj;

		// If the hash element does not exist, add it
		if (!bindings[msg.bid]->timeline)
		{
			// If we get to this point, there is no corresponding UUID in the hash
			bindings[msg.bid]->timeline = kzalloc(sizeof(struct qot_timeline), GFP_KERNEL);
			
			// Copy over the UUID
			strncpy(bindings[msg.bid]->timeline->uuid, msg.uuid, QOT_MAX_UUIDLEN);

			// Register the QoT clock
			if (qot_clock_register(bindings[msg.bid]->timeline))
				return -EACCES;

			// Save the clock index for the user
			msg.tid = bindings[msg.bid]->timeline->index;

			// Add the timeline
			hash_add(qot_timelines_hash, &bindings[msg.bid]->timeline->collision_hash, key);

			// Initialize list heads
			INIT_LIST_HEAD(&bindings[msg.bid]->timeline->head_acc);
			INIT_LIST_HEAD(&bindings[msg.bid]->timeline->head_res);
		}

		// In both cases, we must insert and sort the two linked lists
		qot_add_acc(bindings[msg.bid], &bindings[msg.bid]->timeline->head_acc);
		qot_add_res(bindings[msg.bid], &bindings[msg.bid]->timeline->head_res);

		// Send back the data structure with the new binding and clock info
		if (copy_to_user((struct qot_message*)arg, &msg, sizeof(struct qot_message)))
			return -EACCES;

		break;

	case QOT_UNBIND_TIMELINE:

		// Get the parameters passed into the ioctl
		if (copy_from_user(&msg, (struct qot_message*)arg, sizeof(struct qot_message)))
			return -EACCES;

		// Sanity check
		if (msg.bid < 0 || msg.bid > QOT_MAX_BINDINGS || !bindings[msg.bid])
			return -EACCES;

		// Try and remove the binding
		qot_remove_binding(bindings[msg.bid]);
		
		// If this is the earliest free binding, then cache it
		if (binding_next > msg.bid)
			binding_next = msg.bid;

		break;

	case QOT_SET_ACCURACY:

		// Get the parameters passed into the ioctl
		if (copy_from_user(&msg, (struct qot_message*)arg, sizeof(struct qot_message)))
			return -EACCES;

		// Sanity check
		if (msg.bid < 0 || msg.bid > QOT_MAX_BINDINGS || !bindings[msg.bid])
			return -EACCES;

		// Copy over the accuracy
		bindings[msg.bid]->request.acc = msg.request.acc;

		// Delete the item
		list_del(&bindings[msg.bid]->acc_list);

		// Add the item back
		qot_add_acc(bindings[msg.bid], &bindings[msg.bid]->timeline->head_acc);

		break;

	case QOT_SET_RESOLUTION:

		// Get the parameters passed into the ioctl
		if (copy_from_user(&msg, (struct qot_message*)arg, sizeof(struct qot_message)))
			return -EACCES;

		// Sanity check
		if (msg.bid < 0 || msg.bid > QOT_MAX_BINDINGS || !bindings[msg.bid])
			return -EACCES;

		// Copy over the resolution
		bindings[msg.bid]->request.res = msg.request.res;

		// Delete the item
		list_del(&bindings[msg.bid]->res_list);

		// Add the item back
		qot_add_res(bindings[msg.bid], &bindings[msg.bid]->timeline->head_res);

		break;

	case QOT_WAIT_UNTIL:

		// NOT IMPLEMENTED YET

		break;

	default:
		return -EINVAL;
	}

	// Success!
	return 0;
}

// Define the file operations over th ioctl
static struct file_operations qot_fops = {
    .owner = THIS_MODULE,
    .open = qot_ioctl_open,
    .release = qot_ioctl_close,
    .unlocked_ioctl = qot_ioctl_access
};

// Called on initialization
int qot_init(void)
{
	// Return value
	int ret;

	// Intialize our timeline hash table
	hash_init(qot_timelines_hash);

	// Initialize device for qot scheduler
    if ((ret = alloc_chrdev_region(&dev, FIRST_MINOR, MINOR_CNT, "qot")) < 0)
        return ret;
    cdev_init(&c_dev, &qot_fops); 
    if ((ret = cdev_add(&c_dev, dev, MINOR_CNT)) < 0)
        return ret;
    if (IS_ERR(cl = class_create(THIS_MODULE,"qot")))
    {
        cdev_del(&c_dev);
        unregister_chrdev_region(dev, MINOR_CNT);
        return PTR_ERR(cl);
    }
    if (IS_ERR(dev_ret = device_create(cl, NULL, dev, NULL, "qot")))
    {
        class_destroy(cl);
        cdev_del(&c_dev);
        unregister_chrdev_region(dev, MINOR_CNT);
        return PTR_ERR(dev_ret);
    }

    // Initilize device class for timelines
	qot_clock_class = class_create(THIS_MODULE, "timeline");
	if (IS_ERR(qot_clock_class))
		return PTR_ERR(qot_clock_class);
	ret = alloc_chrdev_region(&qot_clock_devt, 0, MINORMASK + 1, "timeline");
	if (ret < 0)
	{
		class_destroy(qot_clock_class);	
		return 1;
	}

    // Success
 	return 0;
} 

// Called on exit
void qot_cleanup(void)
{
	// Binding ID
	int bid;

	// Remove all bindings and timelines
	for (bid = 0; bid < binding_size; bid++)
		qot_remove_binding(bindings[bid]);

	// Stop ioctl
    device_destroy(cl, dev);
    class_destroy(cl);
    cdev_del(&c_dev);
    unregister_chrdev_region(dev, MINOR_CNT);
}

// Module definitions
module_init(qot_init);
module_exit(qot_cleanup);

// The line below enforces out code to be GPL, because of the POSIX license
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andrew Symington <asymingt@ucla.edu>");
MODULE_DESCRIPTION("QoT ioctl driver");