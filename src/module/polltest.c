/*
 * @file polltest.h
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
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/clocksource.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/rbtree.h>

// Communication protocol
#include "polltest.h"

// General module information
#define MODULE_NAME "polltest"
#define FIRST_MINOR 0
#define MINOR_CNT   1

// Required for character devices
static dev_t dev;
static struct cdev c_dev;
static struct class *cl;
static struct device *dev_ret;
static int value = 0;
static unsigned long timer_interval_ns = 1e9;
static struct hrtimer hr_timer;

// Stores information about an application binding to a timeline
struct polltest_owner {
    struct rb_node node;			/* Red-black tree used to store app info on PID */
	struct file *fileobject;		/* Application's file object 	 */
	wait_queue_head_t wq;			/* Application's wait queue 	 */
	int flag;						/* Application's data ready flag */
};

// A hash table designed to quickly find a timeline based on its UUID
static struct rb_root owner_root = RB_ROOT;

// DATA STRUCTURE MANAGEMENT /////////////////////////////////////////////////////////

// Search our data structure for a given timeline
static struct polltest_owner *polltest_owner_search(struct rb_root *root, struct file *fileobject)
{
	int result;
	struct polltest_owner *owner;
	struct rb_node *node = root->rb_node;
	while (node)
	{
		owner = container_of(node, struct polltest_owner, node);
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

// Insert a timeline into our data structure
static int polltest_owner_insert(struct rb_root *root, struct polltest_owner *data)
{
	int result;
	struct polltest_owner *owner;
	struct rb_node **new = &(root->rb_node), *parent = NULL;
	while (*new)
	{
		owner = container_of(*new, struct polltest_owner, node);
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

static enum hrtimer_restart timer_callback(struct hrtimer *timer_for_restart)
{
	struct polltest_owner *owner;
  	struct rb_node *node;
  	ktime_t currtime, interval;
  	currtime = ktime_get();
  	interval = ktime_set(0, timer_interval_ns); 
  	hrtimer_forward(timer_for_restart, currtime, interval);
	printk("Bumping count\n");
	value++;	
  	for (node = rb_first(&owner_root); node; node = rb_next(node))
	{
		owner = rb_entry(node, struct polltest_owner, node);
		owner->flag = 1;
		wake_up_interruptible(&owner->wq);
	}
	return HRTIMER_RESTART;
}

static int polltest_ioctl_open(struct inode *i, struct file *f)
{
	struct polltest_owner *owner = kzalloc(sizeof(struct polltest_owner), GFP_KERNEL);
	if (!owner)
		return -ENOMEM;
	
	// Initialize the poll info for this file descriptor
	init_waitqueue_head(&owner->wq);
	owner->fileobject = f;
	owner->flag = 0;

	// Insert the poll info into he 
	if (polltest_owner_insert(&owner_root, owner))
    	return 0;

    // Only get here if there is a problem
    kfree(owner);
    return -1;
}

static int polltest_ioctl_close(struct inode *i, struct file *f)
{
	struct polltest_owner *owner = polltest_owner_search(&owner_root, f);
	if (owner)
		rb_erase(&owner->node, &owner_root);
    return 0;
}

static unsigned int polltest_poll(struct file *f, poll_table *wait)
{
	unsigned int mask = 0;
	struct polltest_owner *owner = polltest_owner_search(&owner_root, f);
	if (owner)
	{
		poll_wait(f, &owner->wq, wait);
		if (owner->flag) 
			mask |= (POLLIN | POLLRDNORM);
	}
	return mask;
}


static long polltest_ioctl_access(struct file *f, unsigned int cmd, unsigned long arg)
{
	struct polltest_owner *owner;
	switch (cmd)
	{
	case POLLTEST_GET_VALUE:
		if (copy_to_user((int*)arg, &value, sizeof(int)))
			return -EACCES;
		owner = polltest_owner_search(&owner_root, f);
		if (owner)
			owner->flag = 0;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

// Define the file operations over th ioctl
static struct file_operations polltest_fops = {
    .owner = THIS_MODULE,
    .open = polltest_ioctl_open,
    .release = polltest_ioctl_close,
    .unlocked_ioctl = polltest_ioctl_access,
    .poll = polltest_poll,
};

int polltest_init(void)
{
	int ret;
	ktime_t ktime;

	// Intialize the character device
    if ((ret = alloc_chrdev_region(&dev, FIRST_MINOR, MINOR_CNT, MODULE_NAME)) < 0)
        return ret;
    cdev_init(&c_dev, &polltest_fops); 
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

    // Start the timer
	printk("HR Timer module installing\n");
	ktime = ktime_set(0, timer_interval_ns);
	hrtimer_init(&hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hr_timer.function = &timer_callback;
 	hrtimer_start(&hr_timer, ktime, HRTIMER_MODE_REL);

	return 0;
} 

void polltest_cleanup(void)
{
	// Kill the timer
	int ret = hrtimer_cancel(&hr_timer);
  	if (ret) 
  		printk("The timer was still in use...\n");
	printk("HR Timer module uninstalling\n");

	// Remove the character device for ioctl
    device_destroy(cl, dev);
    class_destroy(cl);
    cdev_del(&c_dev);
    unregister_chrdev_region(dev, MINOR_CNT);
}

// Module definitions
module_init(polltest_init);
module_exit(polltest_cleanup);

// The line below enforces out code to be GPL, because of the POSIX license
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andrew Symington <asymingt@ucla.edu>");
MODULE_DESCRIPTION("Polling ioctl driver");