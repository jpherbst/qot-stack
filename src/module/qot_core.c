/*
 * @file qot_timelines.h
 * @brief Linux 4.1.6 kernel module for creation anmd destruction of QoT timelines
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

// Kernel aPIs
#include <linux/clocksource.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/rbtree.h>
#include <linux/sched.h>
#include <linux/poll.h>

// This file includes
#include "qot.h"
#include "qot_core.h"
#include "qot_timeline.h"
#include "qot_clock.h"

// General module information
#define MODULE_NAME "qot"
#define FIRST_MINOR 0
#define MINOR_CNT 1

// Required for character devices
static dev_t dev;
static struct cdev c_dev;
static struct class *cl;
static struct device *dev_ret;

// Stores information about an application binding to a timeline
struct qot_owner {
    struct rb_node node;			// Red-black tree node
	struct file *fileobject;		// Application's file object 	 
	wait_queue_head_t wq;			// Application's wait queue 	 
	int flag;						// Application's data ready flag 
};

// A hash table designed to quickly find a timeline based on its UUID
static struct rb_root owner_root = RB_ROOT;

// Key data structures
static struct clocksource *clksrc = NULL;

// DATA STRUCTURE MANAGEMENT /////////////////////////////////////////////////////////

static struct qot_owner *qot_owner_search(struct rb_root *root, struct file *fileobject)
{
	int result;
	struct qot_owner *owner;
	struct rb_node *node = root->rb_node;
	while (node)
	{
		owner = container_of(node, struct qot_owner, node);
		result = fileobject - owner->fileobject;
		if (result < 0)
			node = node->rb_left;
		else if (result > 0)
			node = node->rb_right;
		else
			return owner;
	}
	return NULL;
}

static int qot_owner_insert(struct rb_root *root, struct qot_owner *data)
{
	int result;
	struct qot_owner *owner;
	struct rb_node **new = &(root->rb_node), *parent = NULL;
	while (*new)
	{
		owner = container_of(*new, struct qot_owner, node);
		result = data->fileobject - owner->fileobject;
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

// SCHEDULER IOCTL FUNCTIONALITY /////////////////////////////////////////////////////

static int qot_ioctl_open(struct inode *i, struct file *f)
{
	struct qot_owner *owner = kzalloc(sizeof(struct qot_owner), GFP_KERNEL);
	if (!owner)
		return -ENOMEM;
	
	// Initialize the poll info for this file descriptor
	init_waitqueue_head(&owner->wq);
	owner->fileobject = f;
	owner->flag = 0;

	// Insert the poll info into he 
	if (qot_owner_insert(&owner_root, owner))
    	return 0;

    // Only get here if there is a problem
    kfree(owner);
    return 0;
}

static int qot_ioctl_close(struct inode *i, struct file *f)
{
	struct qot_owner *owner = qot_owner_search(&owner_root, f);
	if (owner)
	{
		rb_erase(&owner->node, &owner_root);
		kfree(owner);
	}
    return 0;
}

static long qot_ioctl_access(struct file *f, unsigned int cmd, unsigned long arg)
{
	struct qot_message msg;
	int32_t ret;
	struct qot_owner *owner;

	// Sanity check that owner of file exists
	owner = qot_owner_search(&owner_root, f);
	if (!owner)
		return -EACCES;

	// Deny all access if there is no clocksource registered
	if (!clksrc)
		return -EACCES;

	// Check what message was sent
	switch (cmd)
	{

	case QOT_BIND_TIMELINE:
	
		// Get the parameters passed into the ioctl
		if (copy_from_user(&msg, (struct qot_message*)arg, sizeof(struct qot_message)))
			return -EACCES;

		// Bind to a timeline with a given accuracy and resolution
		msg.bid = qot_timeline_bind(msg.uuid, msg.request.acc, msg.request.res);
		if (msg.bid < 0)
			return -EACCES;

		// Get the posix index
		msg.tid = qot_timeline_index(msg.bid);
		if (msg.tid < 0)
			return -EACCES;

		// Send back the data structure with the new binding and clock info
		if (copy_to_user((struct qot_message*)arg, &msg, sizeof(struct qot_message)))
			return -EACCES;

		break;

	case QOT_UNBIND_TIMELINE:

		// Get the parameters passed into the ioctl
		if (copy_from_user(&msg, (struct qot_message*)arg, sizeof(struct qot_message)))
			return -EACCES;

		// Try and remove the binding
		msg.bid = qot_timeline_unbind(msg.bid);
		if (msg.bid < 0)
			return -EACCES;

		break;

	case QOT_SET_ACCURACY:

		// Get the parameters passed into the ioctl
		if (copy_from_user(&msg, (struct qot_message*)arg, sizeof(struct qot_message)))
			return -EACCES;

		// Try and set the accuracy
		ret = qot_timeline_set_accuracy(msg.bid, msg.request.acc);
		if (ret)
			return -EACCES;

		break;

	case QOT_SET_RESOLUTION:

		// Get the parameters passed into the ioctl
		if (copy_from_user(&msg, (struct qot_message*)arg, sizeof(struct qot_message)))
			return -EACCES;

		// Try and set the accuracy
		ret = qot_timeline_set_resolution(msg.bid, msg.request.res);
		if (ret)
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
	struct qot_owner *owner = qot_owner_search(&owner_root, f);
	if (owner)
	{
		poll_wait(f, &owner->wq, wait);
		if (owner->flag) 
			mask |= (POLLIN | POLLRDNORM);
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
int qot_register(struct clocksource *clksrc_in)
{
	if (!clksrc_in)
	{
		clksrc = clksrc_in;
		return 0;
	}	
	return -1;
}
EXPORT_SYMBOL(qot_register);

// Register the existence of a timer pin
int qot_event_capture(int id, struct qot_capture_event *event)
{
	// TO BE IMPLEMENTED
	return 0;
}
EXPORT_SYMBOL(qot_event_capture);

// Called by driver when a capture event occurs
int qot_event_compare(int id, struct qot_compare_event *event)
{
	// TO BE IMPLEMENTED
	return 0;
}
EXPORT_SYMBOL(qot_event_compare);

// Remap the read query from a cycle counter to a clocksource
cycle_t qot_read_remap(const struct cyclecounter *cc)
{
	if (clksrc)
		return clksrc->read(clksrc);
	return 0;
}

// Copy over the read() function and initial mult/shift for projection
int qot_cyclecounter_init(struct cyclecounter *cc)
{
	// If we have a clock source set
	if (clksrc)
	{
		// Copy over the cycle counter info from the clocksource
		cc->read  = qot_read_remap;
		cc->mask  = clksrc->mask;
		cc->mult  = clksrc->mult;
		cc->shift = clksrc->shift;

		// Success
		return 0;
	}

	// Failure
	return -1;
}
EXPORT_SYMBOL(qot_cyclecounter_init);

// Unregister the clock source
int qot_unregister(void)
{
	if (clksrc)
	{
		clksrc = NULL;
		return 0;
	}	
	return -1;
}
EXPORT_SYMBOL(qot_unregister);


// MODULE LOAD AND UNLOAD ////////////////////////////////////////////////////////

int qot_init(void)
{
	int32_t ret;

	// Initialize the CLOCK subsystem
	ret = qot_clock_init();
	if (ret)
		return ret;

	// Initlialize the TIMELINE subsystem
	ret = qot_timeline_init();
	if (ret)
		return ret;

	// Create a character device  at /dev/qot for interacting with timelines
    if ((ret = alloc_chrdev_region(&dev, FIRST_MINOR, MINOR_CNT, MODULE_NAME)) < 0)
        return ret;
    cdev_init(&c_dev, &qot_fops); 
    if ((ret = cdev_add(&c_dev, dev, MINOR_CNT)) < 0)
        return ret;
    if (IS_ERR(cl = class_create(THIS_MODULE, MODULE_NAME)))
    {
        cdev_del(&c_dev);
        unregister_chrdev_region(dev, MINOR_CNT);
        return PTR_ERR(cl);
    }
    if (IS_ERR(dev_ret = device_create(cl, NULL, dev, NULL, MODULE_NAME)))
    {
        class_destroy(cl);
        cdev_del(&c_dev);
        unregister_chrdev_region(dev, MINOR_CNT);
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
    unregister_chrdev_region(dev, MINOR_CNT);

	// Shutdown the TIMELINE subsystem
    qot_timeline_cleanup();

	// Shutdown the CLOCK subsystem
	qot_clock_cleanup();
}

// Module definitions
module_init(qot_init);
module_exit(qot_cleanup);

// The line below enforces out code to be GPL, because of the POSIX license
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andrew Symington <asymingt@ucla.edu>");
MODULE_DESCRIPTION("QoT ioctl driver");