/*
 * @file qot_clock.c
 * @brief POSIX clock API for the QoT framework, based largely on Linux PTP clocks
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

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>

#include "qot_clock.h"

static dev_t qot_clock_devt;
static struct class *qot_clock_class;

static DEFINE_IDA(qot_clocks_map);

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
	return  0;
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

static void qot_delete_clock(struct posix_clock *pc)
{
	struct qot_clock *clock = container_of(pc, struct qot_clock, posix);
	ida_simple_remove(&qot_clocks_map, clock->index);
}

int qot_timeline_register(struct qot_clock* clock)
{
	// Get the clock index
	clock->index = ida_simple_get(&qot_clocks_map, 0, MINORMASK + 1, GFP_KERNEL);
	if (clock->index < 0)
		return -1;

	// Set the clock
	clock->posix.ops = qot_clock_ops;
	clock->posix.release = qot_delete_clock;	
	clock->devid = MKDEV(MAJOR(qot_clock_devt), clock->index);

	// Create a new device in our class
	clock->dev = device_create(qot_clock_class, NULL, clock->devid, clock, "timeline%d", clock->index);
	if (IS_ERR(clock->dev))
		return -2;

	// Set the driver data
	dev_set_drvdata(clock->dev, clock);

	// Create the POSIX clock
	if (posix_clock_register(&clock->posix, clock->devid))
		return -3;

	// Success
	return 0;
}

int qot_timeline_unregister(struct qot_clock* clock)
{
	// Release clock resources
	device_destroy(qot_clock_class, clock->devid);

	// Unregister clock
	posix_clock_unregister(&clock->posix);

	return 0;
}

void qot_timeline_exit(void)
{
	class_destroy(qot_clock_class);
	unregister_chrdev_region(qot_clock_devt, MINORMASK + 1);
	ida_destroy(&qot_clocks_map);
}

int qot_timeline_init(void)
{
	int err;
	qot_clock_class = class_create(THIS_MODULE, "qotclk");
	if (IS_ERR(qot_clock_class)) {
		pr_err("qotclk: failed to allocate class\n");
		return PTR_ERR(qot_clock_class);
	}

	err = alloc_chrdev_region(&qot_clock_devt, 0, MINORMASK + 1, "qotclk");
	if (err < 0) {
		pr_err("qotclk: failed to allocate device region\n");
		goto no_region;
	}

	pr_info("QoT clock support registered\n");
	return 0;

no_region:
	class_destroy(qot_clock_class);
	return err;
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andrew Symington <asymingt@ucla.edu>");
MODULE_DESCRIPTION("QoT ioctl driver");