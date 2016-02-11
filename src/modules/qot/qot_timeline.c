/*
 * @file qot_core.h
 * @brief Interfaces used by clocks to interact with core
 * @author Andrew Symington
 *
 * Copyright (c) Regents of the University of California, 2015.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <linux/idr.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/posix-clock.h>
#include <linux/pps_kernel.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/spinlock.h>

#include "qot_internal.h"

#define DEVICE_NAME "timeline"

/* PRIVATE */

static dev_t timeline_devt;
static struct class *timeline_class;

static DEFINE_IDR(qot_timelines_map);

static spinlock_t qot_timelines_lock;

/* Private data for this timeline, not visible outside this code   */
typedef struct timeline_impl {
    struct rb_node node;        /* Red-black tree indexes by name  */
    int index;                  /* IDR index for timeline          */
    dev_t devid;                /* Device ID                       */
    struct device *dev;         /* Device                          */
    struct posix_clock clock;   /* POSIX clock for this timeline   */
} timeline_impl_t;

/* File operations for a given timeline */
static struct posix_clock_operations qot_clock_ops = {
	.owner		    = THIS_MODULE,
	.clock_adjtime	= qot_timeline_clock_adjtime,
	.clock_gettime	= qot_timeline_clock_gettime,
	.clock_getres	= qot_timeline_clock_getres,
	.clock_settime	= qot_timeline_clock_settime,
	.ioctl		    = qot_timeline_chdev_ioctl,
	.open		    = qot_timeline_chdev_open,
    .release        = qot_timeline_chdev_release,
	.poll		    = qot_timeline_chdev_poll,
	.read		    = qot_timeline_chdev_read,
};

static void qot_timeline_delete(struct posix_clock *pc) {
    timeline_impl_t *timeline_impl = container_of(pc, timeline_impl_t, clock);
    idr_remove(&qot_timelines_map, timeline_impl->index);
    kfree(timeline_impl);
}

/* PUBLIC */

/* Create the timeline and return an index that identifies it uniquely */
qot_return_t qot_timeline_register(qot_timeline_t *timeline) {
    timeline_impl_t *timeline_impl;
    int major = MAJOR(timeline_devt);   /* Allocate a major number for device */
    if (!timeline)
        return QOT_RETURN_TYPE_ERR;

    /* Allocate some memory for the timeline implementation data */
    timeline_impl = kzalloc(sizeof(timeline_impl_t), GFP_KERNEL);
    if (!timeline_impl) {
        pr_err("qot_timeline: cannot allocate memory for timeline_impl");
        goto fail_memoryalloc;
    }

    /* Draw the next integer X for /dev/timelineX */
    idr_preload(GFP_KERNEL);
    spin_lock(&qot_timelines_lock);
    timeline_impl->index = idr_alloc(&qot_timelines_map,
        (void*) timeline_impl, 0, 0, GFP_NOWAIT);
    spin_unlock(&qot_timelines_lock);
    idr_preload_end();
	if (timeline_impl->index < 0) {
        pr_err("qot_timeline: cannot get next integer");
        goto fail_idasimpleget;
    }

    /* Assign an ID and create the character device for this timeline */
    timeline_impl->devid = MKDEV(major, timeline_impl->index);
    timeline_impl->dev = device_create(timeline_class, NULL,
        timeline_impl->devid, timeline_impl, "timeline%d", timeline_impl->index);

    /* Setup the PTP clock for this timeline */
    timeline_impl->clock.ops = qot_clock_ops;
    timeline_impl->clock.release = qot_timeline_delete;
    if (posix_clock_register(&timeline_impl->clock, timeline_impl->devid)) {
        pr_err("qot_timeline: cannot register POSIX clock");
        goto fail_posixclock;
    }

    /* Update the index before returning OK */
    timeline->index = timeline_impl->index;

    return QOT_RETURN_TYPE_OK;

fail_posixclock:
    idr_remove(&qot_timelines_map, timeline_impl->index);
fail_idasimpleget:
    kfree(timeline_impl);
fail_memoryalloc:
    return QOT_RETURN_TYPE_ERR;
}

/* Destroy the timeline based on an index */
qot_return_t qot_timeline_unregister(int index) {
    timeline_impl_t *timeline_impl = idr_find(&qot_timelines_map, index);
    if (!timeline_impl)
        return QOT_RETURN_TYPE_ERR;
    /* Remove the posix clock */
    posix_clock_unregister(&timeline_impl->clock);
    /* Delete the character device - also deletes IDR */
    device_destroy(timeline_class, timeline_impl->devid);
    return QOT_RETURN_TYPE_OK;
}

/* Cleanup the timeline system */
void qot_timeline_cleanup(struct class *qot_class) {
    /* Remove the character device region */
	unregister_chrdev_region(timeline_devt, MINORMASK + 1);
    /* Shut down the IDR subsystem */
    idr_destroy(&qot_timelines_map);
}

/* Initialize the timeline system */
qot_return_t qot_timeline_init(struct class *qot_class) {
    int err;
    /* Copy the device class to help with future chdev allocations */
    if (!qot_class)
        return QOT_RETURN_TYPE_ERR;
    timeline_class = qot_class;
    /* Try and allocate a character device region to hold all /dev/timelines */
    err = alloc_chrdev_region(&timeline_devt, 0, MINORMASK + 1, DEVICE_NAME);
    if (err < 0) {
        pr_err("ptp: failed to allocate device region\n");
    }
    spin_lock_init(&qot_timelines_lock);
    return QOT_RETURN_TYPE_OK;
}

MODULE_LICENSE("GPL");
