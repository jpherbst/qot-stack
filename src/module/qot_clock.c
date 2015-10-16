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
#include <linux/timecounter.h>
#include <linux/posix-clock.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/slab.h>

// This code
#include "qot_clock.h"

// General module information
#define MODULE_NAME "timeline"
#define FIRST_MINOR 0
#define MINOR_CNT   1

// Stores sufficient information
struct qot_clock {
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

// Required for character devices
static dev_t qot_clock_devt;
static struct class *qot_clock_class;

// Create a clock map
DEFINE_IDA(qot_clocks_map);

// SOFTWARE-BASED TIME DISCIPLINE //////////////////////////////////////////////

static int qot_adjfreq(struct qot_clock *clk, int32_t ppb)
{
	uint64_t adj;
	uint32_t diff, mult, flags;
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
	spin_lock_irqsave(&clk->lock, flags);
	timecounter_read(&clk->tc);
	clk->cc.mult = neg_adj ? mult - diff : mult + diff;
	spin_unlock_irqrestore(&clk->lock, flags);
	return 0;
}

static int qot_adjtime(struct qot_clock *clk, int64_t delta)
{
	uint32_t flags;
	spin_lock_irqsave(&clk->lock, flags);
	timecounter_adjtime(&clk->tc, delta);
	spin_unlock_irqrestore(&clk->lock, flags);
	return 0;
}

static int qot_gettime(struct qot_clock *clk, struct timespec64 *ts)
{
	uint64_t ns;
	uint32_t flags;
	spin_lock_irqsave(&clk->lock, flags);
	ns = timecounter_read(&clk->tc);
	spin_unlock_irqrestore(&clk->lock, flags);
	*ts = ns_to_timespec64(ns);
	return 0;
}

static int qot_settime(struct qot_clock *clk, const struct timespec64 *ts)
{
	uint64_t ns;
	uint32_t flags;
	ns = timespec64_to_ns(ts);	
	spin_lock_irqsave(&clk->lock, flags);
	timecounter_init(&clk->tc, &clk->cc, ns);
	spin_unlock_irqrestore(&clk->lock, flags);
	return 0;
}

static void qot_overflow_check(struct work_struct *work)
{
	struct timespec64 ts;
	struct qot_clock *clk = container_of(work, struct qot_clock, overflow_work.work);
	qot_gettime(clk, &ts)
	schedule_delayed_work(&clk->overflow_work, QOT_OVERFLOW_PERIOD);
}

static cycle_t qot_systim_read(const struct cyclecounter *cc)
{
	struct qot_clock *clk = container_of(cc, struct qot_clock, cc);

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
	struct qot_clock *clk = container_of(pc, struct qot_clock, clock);
	struct qot_metric metric;
	switch (cmd)
	{
	case QOT_GET_ACTUAL_METRIC:
		if (copy_to_user((struct qot_metric*)arg, &clk->actual, sizeof(struct qot_metric)))
			return -EFAULT;
		break;
	case QOT_GET_TARGET_METRIC:
		metric.acc = list_entry(clk->head_acc.next, struct qot_binding, acc_list)->request.acc;
		metric.res = list_entry(clk->head_res.next, struct qot_binding, res_list)->request.res;
		if (copy_to_user((struct qot_metric*)arg, &metric, sizeof(struct qot_metric)))
			return -EFAULT;
		break;
	case QOT_GET_UUID:
		if (copy_to_user((char*)arg, clk->uuid, QOT_MAX_UUIDLEN*sizeof(char)))
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
	struct qot_clock *clk = container_of(pc, struct qot_clock, clock);
	ida_simple_remove(&qot_clocks_map, clk->index);
}

static int qot_clock_register(struct qot_clock* clk)
{
	// Initialize the spin tlock
	spin_lock_init(&clk->lock);

	// Get the clock index
	clk->index = ida_simple_get(&qot_clocks_map, 0, MINORMASK + 1, GFP_KERNEL);
	if (clk->index < 0)
		return -1;

	// Create the cycle and time counter
	clk->cc.read  = qot_systim_read;		// Tick counter
	clk->cc.mask  = CLOCKSOURCE_MASK(32);	// 32 bit counter
	clk->cc_mult  = mult;					// Param for 24MHz
	clk->cc.mult  = mult;					// Param for 24MHz
	clk->cc.shift = shift;					// Param for 24MHz
	spin_lock_irqsave(&clk->lock, flags);
	timecounter_init(&clk->tc, &clk->cc, ktime_to_ns(ktime_get_real()));
	spin_unlock_irqrestore(&clk->lock, flags);

	// Overflow management
	INIT_DELAYED_WORK(&clk->overflow_work, qot_overflow_check);
	schedule_delayed_work(&clk->overflow_work, QOT_OVERFLOW_PERIOD);

	// Set the clock
	clk->clock.ops = qot_clock_ops;
	clk->clock.release = qot_clock_delete;	
	clk->devid = MKDEV(MAJOR(qot_clock_devt), clk->index);

	// Create a new device in our class
	clk->dev = device_create(qot_clock_class, NULL, clk->devid, clk, "clk%d", clk->index);
	if (IS_ERR(clk->dev))
		return -2;

	// Set the driver data
	dev_set_drvdata(clk->dev, clk);

	// Create the POSIX clock
	if (posix_clock_register(&clk->clock, clk->devid))
		return -3;

	// Success
	return 0;
}

int qot_clock_unregister(struct qot_clock* clk)
{
	// Release clock resources
	device_destroy(cl, clk->devid);

	// Unregister clock
	posix_clock_unregister(&clk->clock);

	return 0;
}

int32_t qot_clock_init(void)
{
	int32_t ret;
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

void qot_clock_cleanup(void)
{
    // Remove all posix clocks 
    class_destroy(qot_clock_class);
	unregister_chrdev_region(qot_clock_devt, MINORMASK + 1);
	ida_destroy(&qot_clocks_map);
}