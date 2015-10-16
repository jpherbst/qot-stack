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

	// Create a caracter device  at /dev/qot for interacting with timelines
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
}

// Module definitions
module_init(qot_init);
module_exit(qot_cleanup);

// The line below enforces out code to be GPL, because of the POSIX license
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andrew Symington <asymingt@ucla.edu>");
MODULE_DESCRIPTION("QoT ioctl driver");


















/*

#include <linux/timecounter.h>
#include <linux/workqueue.h>
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

// Max adjustment for clock disciplining
#define QOT_MAX_ADJUSTMENT  	(1000000)
#define QOT_OVERFLOW_PERIOD		(HZ*8)

// Module information
#define MODULE_NAME "qot"
#define FIRST_MINOR 0
#define MINOR_CNT   1

// Stores information about a timeline
struct qot_timeline {
    char uuid[QOT_MAX_UUIDLEN];			// Unique id for this timeline
    struct hlist_node collision_hash;	// Pointer to next timeline with common hash key
    struct list_head head_acc;			// Head pointing to maximum accuracy structure
    struct list_head head_res;			// Head pointing to maximum resolution structure
    struct qot_metric actual;			// Achieved accuracy and resolution
    struct posix_clock clock;			// The POSIX clock 
	struct cyclecounter cc;				// Cycle counter
	struct timecounter tc;				// Time counter
	uint32_t cc_mult; 					// Nominal frequency
	int32_t dialed_frequency; 			// Frequency adjustment memory
	struct delayed_work overflow_work;	// Periodic overflow checking
	spinlock_t lock; 					// Protects time registers
    struct device *dev;					// Parent device
    dev_t devid;						// Device id
    int index;                      	// Index in clock map (X in /dev/timelineX)
};

// Stores information about an application binding to a timeline
struct qot_binding {
	struct qot_metric request;			// requested accuracy and resolution 
    struct list_head res_list;			// Next resolution
    struct list_head acc_list;			// Next accuracy
    struct qot_timeline* timeline;		// Parent timeline
};
 
// Device class (scheduler)
static dev_t dev;						// Devices
static struct cdev c_dev;				// Character device
static struct class *cl;				// Class
static struct device *dev_ret;			// Device

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

// SOFTWARE-BASED TIME DISCIPLINE //////////////////////////////////////////////

static int qot_adjfreq(struct qot_timeline *timeline, int32_t ppb)
{
	uint64_t adj;
	uint32_t diff, mult, flags;
	int32_t neg_adj = 0;
	if (ppb < 0)
	{
		neg_adj = 1;
		ppb = -ppb;
	}
	mult = timeline->cc_mult;
	adj = mult;
	adj *= ppb;
	diff = div_u64(adj, 1000000000ULL);
	spin_lock_irqsave(&timeline->lock, flags);
	timecounter_read(&timeline->tc);
	timeline->cc.mult = neg_adj ? mult - diff : mult + diff;
	spin_unlock_irqrestore(&timeline->lock, flags);
	return 0;
}

static int qot_adjtime(struct qot_timeline *timeline, int64_t delta)
{
	uint32_t flags;
	spin_lock_irqsave(&timeline->lock, flags);
	timecounter_adjtime(&timeline->tc, delta);
	spin_unlock_irqrestore(&timeline->lock, flags);
	return 0;
}

static int qot_gettime(struct qot_timeline *timeline, struct timespec64 *ts)
{
	uint64_t ns;
	uint32_t flags;
	spin_lock_irqsave(&timeline->lock, flags);
	ns = timecounter_read(&timeline->tc);
	spin_unlock_irqrestore(&timeline->lock, flags);
	*ts = ns_to_timespec64(ns);
	return 0;
}

static int qot_settime(struct qot_timeline *timeline, const struct timespec64 *ts)
{
	uint64_t ns;
	uint32_t flags;
	ns = timespec64_to_ns(ts);	
	spin_lock_irqsave(&timeline->lock, flags);
	timecounter_init(&timeline->tc, &timeline->cc, ns);
	spin_unlock_irqrestore(&timeline->lock, flags);
	return 0;
}

static void qot_overflow_check(struct work_struct *work)
{
	struct timespec64 ts;
	struct qot_timeline *timeline = container_of(work, struct qot_timeline, overflow_work.work);
	qot_gettime(timeline, &ts)
	schedule_delayed_work(&timeline->overflow_work, QOT_OVERFLOW_PERIOD);
}

static cycle_t qot_systim_read(const struct cyclecounter *cc)
{
	struct qot_timeline *timeline = container_of(cc, struct qot_timeline, cc);

	return val;
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
	struct qot_timeline *timeline = container_of(pc, struct qot_timeline, clock);
	ida_simple_remove(&qot_clocks_map, timeline->index);
}

static int qot_clock_register(struct qot_timeline* timeline)
{
	// Initialize the spin tlock
	spin_lock_init(&timeline->lock);

	// Get the clock index
	timeline->index = ida_simple_get(&qot_clocks_map, 0, MINORMASK + 1, GFP_KERNEL);
	if (timeline->index < 0)
		return -1;

	// Create the cycle and time counter
	timeline->cc.read  = qot_systim_read;		// Tick counter
	timeline->cc.mask  = CLOCKSOURCE_MASK(32);	// 32 bit counter
	timeline->cc_mult  = mult;					// Param for 24MHz
	timeline->cc.mult  = mult;					// Param for 24MHz
	timeline->cc.shift = shift;					// Param for 24MHz
	spin_lock_irqsave(&timeline->lock, flags);
	timecounter_init(&timeline->tc, &timeline->cc, ktime_to_ns(ktime_get_real()));
	spin_unlock_irqrestore(&timeline->lock, flags);

	// Overflow management
	INIT_DELAYED_WORK(&timeline->overflow_work, qot_overflow_check);
	schedule_delayed_work(&timeline->overflow_work, QOT_OVERFLOW_PERIOD);

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

    // Destroy 
    class_destroy(qot_clock_class);
	unregister_chrdev_region(qot_clock_devt, MINORMASK + 1);
	ida_destroy(&qot_clocks_map);
}

// Module definitions
module_init(qot_init);
module_exit(qot_cleanup);

// The line below enforces out code to be GPL, because of the POSIX license
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andrew Symington <asymingt@ucla.edu>");
MODULE_DESCRIPTION("QoT ioctl driver");

*/