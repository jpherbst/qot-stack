/*
 * @file qot_scheduler.h
 * @brief Interface to the QoT Scheduler
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

#ifndef QOT_STACK_SRC_MODULES_QOT_QOT_SCHEDULER_H
#define QOT_STACK_SRC_MODULES_QOT_QOT_SCHEDULER_H

#include "qot_timeline.h"

/* Puts task into a blocking sleep */
int qot_attosleep(utimepoint_t *expiry_time, struct qot_timeline *timeline);

/* Update tasks that are blocking when the notion of time changes */
void qot_scheduler_update(void);

/* Cleanup the timeline subsystem */
void qot_scheduler_cleanup(struct class *qot_class);

/* Initialize the timeline subsystem */
qot_return_t qot_scheduler_init(struct class *qot_class);

#endif

