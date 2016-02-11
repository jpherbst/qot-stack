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

#include "qot_internal.h"

static DEFINE_IDA(qot_timelines_map);

static struct posix_clock_operations ptp_clock_ops = {
	.owner		    = THIS_MODULE,
	.clock_adjtime	= qot_timeline_clock_adjtime,
	.clock_gettime	= qot_timeline_clock_gettime,
	.clock_getres	= qot_timeline_clock_getres,
	.clock_settime	= qot_timeline_clock_settime,
	.ioctl		    = qot_timeline_chdev_ioctl,
	.open		    = qot_timeline_chdev_open,
    .close          = qot_timeline_chdev_close,
	.poll		    = qot_timeline_chdev_poll,
	.read		    = qot_timeline_chdev_read,
};

/* public interface */

qot_return_t qot_timeline_register(qot_timeline_t *timeline) {
    timeline->index =
        ida_simple_get(&qot_timelines_map, 0, MINORMASK + 1, GFP_KERNEL);
	if (index < 0)
        return QOT_RETURN_TYPE_ERR;
    return QOT_RETURN_TYPE_OK;
}

qot_return_t qot_timeline_unregister(qot_timeline_t *timeline) {
    /* Todo */
}

/* Cleanup the timeline system */
void qot_timeline_cleanup(struct class *qot_class) {
	ida_destroy(&qot_timelines_map);
}

/* Initialize the timeline system */
qot_return_t qot_timeline_init(struct class *qot_class) {
    /* TODO */
}

MODULE_LICENSE("GPL");
