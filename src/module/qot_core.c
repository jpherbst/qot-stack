/*
 * @file qot_timelines.h
 * @brief Linux 4.1.6 kernel module for creation anmd destruction of QoT timelines
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

// Kernel aPIs
#include <linux/module.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

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

// Key data structures
static struct clocksource *clksrc = NULL;

// EXPORTED FUNCTIONALITY ////////////////////////////////////////////////////////

// Register a clock source as the primary driver of time
int32_t qot_register(struct clocksource *clksrc)
{
	return 0;
}
EXPORT_SYMBOL(qot_register);

// Add an oscillator
int32_t qot_add_oscillator(struct qot_oscillator* oscillator)
{
	return 0;
}
EXPORT_SYMBOL(qot_add_oscillator);

// Register the existence of a timer pin
int32_t qot_add_compare(int32_t id)
{
	return 0;
}
EXPORT_SYMBOL(qot_add_compare);

// Called by driver when a capture event occurs
int32_t qot_add_capture(int32_t id)
{
	return 0;
}
EXPORT_SYMBOL(qot_add_capture);

// Register the existence of a timer pin
int32_t qot_event_capture(int32_t id, struct qot_capture_event *event)
{
	return 0;
}
EXPORT_SYMBOL(qot_event_capture);

// Called by driver when a capture event occurs
int32_t qot_event_compare(int32_t id, struct qot_compare_event *event)
{
	return 0;
}
EXPORT_SYMBOL(qot_event_compare);

// Read the current hardware count of the current clock source
cycle_t qot_read_count(void)
{
	//if (clksrc)
	//	return clksrc->read();
	return 0;
}
EXPORT_SYMBOL(qot_read_count);

// Uncregister the  clock source, oscillators and pins
int32_t qot_unregister(struct clocksource *clk)
{
	return 0;
}
EXPORT_SYMBOL(qot_unregister);

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
	struct qot_message msg;
	int32_t ret;
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

// Define the file operations over th ioctl
static struct file_operations qot_fops = {
    .owner = THIS_MODULE,
    .open = qot_ioctl_open,
    .release = qot_ioctl_close,
    .unlocked_ioctl = qot_ioctl_access
};

// MODULE LOAD AND UNLOAD ////////////////////////////////////////////////////////

int32_t qot_init(void)
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