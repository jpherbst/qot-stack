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

#include "qot_core.h"

/* Stores a binding to a timeline */

typedef struct qot_binding {
    u32 placeholder;
} qot_binding_t;

/* For qot_core */

qot_return_t qot_core_timeline_get_info(qot_timeline_t *timeline);

qot_return_t qot_core_timeline_del_binding(qot_binding_t *binding);

qot_return_t qot_core_timeline_add_binding(qot_timeline_t *timeline,
    qot_binding_t *binding);

qot_return_t qot_core_clock_get_info(qot_clock_t *clk);

qot_return_t qot_core_clock_sleep(qot_clock_t *clk);

qot_return_t qot_core_clock_wake(qot_clock_t *clk);

qot_return_t qot_core_clock_switch(qot_clock_t *clk);

/* For qot_chardev_adm */

qot_return_t qot_chardev_adm_init(void);

qot_return_t qot_chardev_adm_cleanup(void);

qot_return_t qot_chardev_adm_clock_update(qot_clock_t *clk);

qot_return_t qot_chardev_adm_clock_add(qot_clock_t *clk);

qot_return_t qot_chardev_adm_clock_del(qot_clock_t *clk);

/* For qot_chardev_adm */

qot_return_t qot_chardev_usr_init(void);

qot_return_t qot_chardev_usr_cleanup(void);

qot_return_t qot_chardev_adm_timeline_update(qot_timeline_t *clk);

qot_return_t qot_chardev_adm_timeline_add(qot_timeline_t *clk);

qot_return_t qot_chardev_adm_timeline_del(qot_timeline_t *clk);


/* For qot_clock_sysfs */

// int qot_clock_sysfs_init(void);

// void qot_clock_sysfs_cleanup(void);

/* For qot_scheduler */

// int interface_update(struct rb_root *timeline_root);

//int interface_nanosleep(struct timespec *exp_time, struct qot_timeline *timeline);


#endif