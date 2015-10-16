/*
 * @file qot_clock.c
 * @brief Linux 4.1.6 kernel module for creation anmd destruction of QoT clks
 * @author Fatima Anwar and Andrew Symington
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

// Kernel APIs
#include <linux/idr.h>
#include <linux/list.h>
#include <linux/workqueue.h>
#include <linux/timecounter.h>
#include <linux/posix-clock.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/slab.h>

// This code
#include "qot.h"
#include "qot_core.h"
#include "qot_clock.h"

// General module information
#define MODULE_NAME "timeline"
#define FIRST_MINOR 0
#define MINOR_CNT   1

// Clock maintenance
#define QOT_MAX_ADJUSTMENT  	(1000000)
#define QOT_OVERFLOW_PERIOD		(HZ*8)

// Stores sufficient information
struct qot_clock {
    struct device *dev;					// Parent device of the clock
    dev_t devid;						// Device identifier
    struct posix_clock clock;			// The POSIX clock 
	struct timecounter tc;				// Time counter
	struct cyclecounter cc;				// Cycle counter
	uint32_t cc_mult; 					// Nominal frequency
	int32_t dialed_frequency; 			// Frequency adjustment memory
	struct delayed_work overflow_work;	// Periodic overflow checking
};

// Required for character devices
static dev_t qot_clock_devt;
static struct class *qot_clock_class;

// Use the IDR mechanism to dynamically allocate clock ids and find them quickly
static struct idr idr_clocks;

// SOFTWARE-BASED TIME DISCIPLINE //////////////////////////////////////////////

static int qot_adjfreq(struct qot_clock *clk, int32_t ppb)
{
	uint64_t adj;
	uint32_t diff, mult;
	int32_t neg_adj = 0;
	if (ppb < 0)
	{
		neg_adj = 1;
		ppb = -ppb;
	}
	mult = clk->cc_mult;
	adj = mult;
	adj *= ppb;
	diff = div_u64(adj, 1000000000ULL);
	timecounter_read(&clk->tc);
	clk->cc.mult = neg_adj ? mult - diff : mult + diff;
	return 0;
}

static int qot_adjtime(struct qot_clock *clk, int64_t delta)
{
	timecounter_adjtime(&clk->tc, delta);
	return 0;
}

static int qot_gettime(struct qot_clock *clk, struct timespec64 *ts)
{
	uint64_t ns;
	ns = timecounter_read(&clk->tc);
	*ts = ns_to_timespec64(ns);
	return 0;
}

static int qot_settime(struct qot_clock *clk, const struct timespec64 *ts)
{
	uint64_t ns;
	ns = timespec64_to_ns(ts);	
	timecounter_init(&clk->tc, &clk->cc, ns);
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
	switch (cmd)
	{
	case QOT_GET_ACTUAL_METRIC:
		break;
	case QOT_GET_TARGET_METRIC:
		break;
	case QOT_GET_UUID:
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
	struct qot_clock *clk = container_of(pc, struct qot_clock, clock);
	struct timespec64 ts = timespec_to_timespec64(*tp);
	return qot_settime(clk, &ts);
}

static int qot_clock_gettime(struct posix_clock *pc, struct timespec *tp)
{
	struct qot_clock *clk = container_of(pc, struct qot_clock, clock);
	struct timespec64 ts;
	int err;
	err = qot_gettime(clk, &ts);
	if (!err)
		*tp = timespec64_to_timespec(ts);
	return err;
}

static int qot_clock_adjtime(struct posix_clock *pc, struct timex *tx)
{
	struct qot_clock *clk = container_of(pc, struct qot_clock, clock);
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
		err = qot_adjtime(clk, delta);
	} 
	else if (tx->modes & ADJ_FREQUENCY)
	{
		int64_t ppb = 1 + tx->freq;
		ppb *= 125;
		ppb >>= 13;
		if (ppb > QOT_MAX_ADJUSTMENT || ppb < -QOT_MAX_ADJUSTMENT)
			return -ERANGE;
		err = qot_adjfreq(clk, ppb);
		clk->dialed_frequency = tx->freq;
	} 
	else if (tx->modes == 0)
	{
		tx->freq = clk->dialed_frequency;
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

static void qot_overflow_check(struct work_struct *work)
{
	struct timespec64 ts;
	struct qot_clock *clk = container_of(work, struct qot_clock, overflow_work.work);
	qot_gettime(clk, &ts);
	schedule_delayed_work(&clk->overflow_work, QOT_OVERFLOW_PERIOD);
}

// FUNCTIONS DESIGNED TO BE CALLED FROM TIMELINE /////////////////////////////////////////

// Register a clock
int qot_clock_register(void)
{	
	struct qot_clock* clk;
	int index;

	// Try and allocate some memory
	clk = kzalloc(sizeof(struct qot_clock), GFP_KERNEL);
	if (clk == NULL)
		return -1;

	// Obtain a new binding ID
	index = idr_alloc(&idr_clocks, clk, 0, MINORMASK + 1, GFP_KERNEL);
	if (index < 0)
		goto free_clock;

	// Initialize a cycle counter with the info from the QoT core clock an initial 
	qot_cyclecounter_init(&clk->cc);

	// Initialize a timecounter as a wrapper to the cycle counter
	timecounter_init(&clk->tc, &clk->cc, 0);

	// Manage overflows for this clock
	INIT_DELAYED_WORK(&clk->overflow_work, qot_overflow_check);
	schedule_delayed_work(&clk->overflow_work, QOT_OVERFLOW_PERIOD);

	// Create a new character device for the clock
	clk->clock.ops = qot_clock_ops;
	clk->clock.release = qot_clock_delete;	
	clk->devid = MKDEV(MAJOR(qot_clock_devt), index);
	clk->dev = device_create(qot_clock_class, NULL, clk->devid, clk, "timeline%d", index);
	if (IS_ERR(clk->dev))
		goto free_clock;

	// Set the driver data
	dev_set_drvdata(clk->dev, clk);

	// Create the POSIX clock
	if (posix_clock_register(&clk->clock, clk->devid))
		goto free_device;

	// Success
	return index;

free_device:
	device_destroy(qot_clock_class, clk->devid);
free_clock:
	kfree(clk);
	return -2;
}


// Completely destroy a binding
int qot_destroy_clock(int id, void *p, void *data)
{
	// Cast to find the binding information
	struct qot_clock *clk = (struct qot_clock *) p;

	// Release clock resources
	device_destroy(qot_clock_class, clk->devid);

	// Unregister clock
	posix_clock_unregister(&clk->clock);

	// Remove the binding ID form the list
	idr_remove(&idr_clocks, id);

	// Success
	return 0;
}

// Unregister a clock
int qot_clock_unregister(int index)
{
	// Grab the binding from the ID
	struct qot_clock *clk = idr_find(&idr_clocks, index);
	if (!clk)
		return -1;

	// Success
	return qot_destroy_clock(index, (void *)clk, NULL);
}

// Initialize the clock subsystem
int qot_clock_init(void)
{
	int ret;

	// Allocates clock ids
	idr_init(&idr_clocks);
	
	// Create a device class for posix clock character devices
	qot_clock_class = class_create(THIS_MODULE, MODULE_NAME);
	if (IS_ERR(qot_clock_class))
		return PTR_ERR(qot_clock_class);
	ret = alloc_chrdev_region(&qot_clock_devt, 0, MINORMASK + 1, MODULE_NAME);
	if (ret < 0)
	{
		class_destroy(qot_clock_class);	
		return 1;
	}
	return 0;
} 

// Cleanup the clock subsystem
void qot_clock_cleanup(void)
{
	// Desttroy every binding
	idr_for_each(&idr_clocks, qot_destroy_clock, NULL);

	// Allocates clock ids
	idr_destroy(&idr_clocks);

	// Remove the clock class
    class_destroy(qot_clock_class);
	unregister_chrdev_region(qot_clock_devt, MINORMASK + 1);
}