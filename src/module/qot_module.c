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
#include <asm/uaccess.h>

// This module functionality
#include "qot_ioctl.h"
//#include "qot_clock.h"

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
	//struct qot_clock clock;				// QoT clock
};

// Stores information about an application binding to a timeline
struct qot_binding {
	uint64_t res;						// Desired resolution
	uint64_t acc;						// Desired accuracy
    struct list_head res_list;			// Next resolution
    struct list_head acc_list;			// Next accuracy
    struct qot_timeline* timeline;		// Parent timeline
};

// We'll use the ioctl device as the PTP device
static struct device *dev_ret;

// An array of bindings (one for each application request)
static struct qot_binding *bindings[QOT_MAX_BINDINGS+1];

// The next available binding ID, as well as the total number alloacted
static int binding_next = 0;
static int binding_size = 0;

// A hash table designed to quickly find timeline based on UUID
DEFINE_HASHTABLE(qot_timelines_hash, QOT_HASHTABLE_BITS);

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
		if (bind_obj->acc > binding_list->acc)
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
		if (bind_obj->res > binding_list->res)
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
		//qot_clock_unregister(&binding->timeline->clock);

		// Free the timeline entry memory
		kfree(binding->timeline);
	}

	// Free the memory used by this binding
	kfree(binding);

	// Set explicitly to NULL
	binding = NULL;
}

// IOCTL FUNCTIONALITY /////////////////////////////////////////////////////////

// Required for ioctl
static dev_t dev;
static struct cdev c_dev;
static struct class *cl;

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
		bindings[msg.bid]->acc = msg.acc;
		bindings[msg.bid]->res = msg.res;

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
			//qot_clock_register(&binding->timeline->clock);

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
		//qot_remove_binding(bindings[msg.bid]);
		
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
		bindings[msg.bid]->acc = msg.acc;

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
		bindings[msg.bid]->res = msg.res;

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
	int ret;

	// Intialize clock framework
	//qot_clock_init();

	// Intialize our timeline hash table
	hash_init(qot_timelines_hash);

	// Initialize ioctl
    if ((ret = alloc_chrdev_region(&dev, FIRST_MINOR, MINOR_CNT, "qot_ioctl")) < 0)
        return ret;
    cdev_init(&c_dev, &qot_fops); 
    if ((ret = cdev_add(&c_dev, dev, MINOR_CNT)) < 0)
        return ret;
    if (IS_ERR(cl = class_create(THIS_MODULE, "char")))
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
 	
    // Success
 	return 0;
} 

// Called on exit
void qot_cleanup(void)
{
	int bid;

	// Remove all bindings and timelines
	for (bid = 0; bid < binding_size; bid++)
		qot_remove_binding(bindings[bid]);

	// Stop ioctl
    device_destroy(cl, dev);
    class_destroy(cl);
    cdev_del(&c_dev);
    unregister_chrdev_region(dev, MINOR_CNT);

	// Intialize clock framework
	//qot_clock_exit();
}

// Module definitions
module_init(qot_init);
module_exit(qot_cleanup);

// The line below enforces out code to be GPL, because of the POSIX license
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andrew Symington <asymingt@ucla.edu>");
MODULE_DESCRIPTION("QoT ioctl driver");