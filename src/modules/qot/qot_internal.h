/*
 * @file qot_internal.h
 * @brief Internal functions shared between subcomponents of qot_core
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

#ifndef QOT_STACK_SRC_MODULES_QOT_QOT_INTERNAL_H
#define QOT_STACK_SRC_MODULES_QOT_QOT_INTERNAL_H

#include "qot_exported.h"

/* Internal event type */
typedef struct event {
    qot_event_t info;            /* The event type                      */
    struct list_head list;       /* The list of events head             */
} event_t;

/* qot_core: To support ioctl calls from /dev/qotusr */

qot_return_t qot_core_timeline_next(qot_timeline_t *timeline);

qot_return_t qot_core_timeline_get_info(qot_timeline_t *timeline);

qot_return_t qot_core_timeline_create(qot_timeline_t *timeline);

/* qot_core: To support ioctl calls from /dev/qotadm */

qot_return_t qot_core_clock_next(qot_clock_t *clk);

qot_return_t qot_core_clock_get_info(qot_clock_t *clk);

qot_return_t qot_core_clock_sleep(qot_clock_t *clk);

qot_return_t qot_core_clock_wake(qot_clock_t *clk);

qot_return_t qot_core_clock_switch(qot_clock_t *clk);

/* qot_chardev_adm: Function calls from qot_core */

qot_return_t qot_chardev_adm_init(void);

qot_return_t qot_chardev_adm_cleanup(void);

/* qot_chardev_usr: Function calls from qot_core */

qot_return_t qot_chardev_usr_init(void);

qot_return_t qot_chardev_usr_cleanup(void);

/* qot_chardev_usr: Function calls from qot_core */

qot_return_t qot_sysfs_init(void);

qot_return_t qot_sysfs_cleanup(void);

#endif