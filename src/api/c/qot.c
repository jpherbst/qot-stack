/*
 * @file qot.h
 * @brief A simple C application programmer interface to the QoT stack
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

/* System includes */
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

/* This file includes */
#include "qot.h"

/* Timeline implementation */
typedef struct timeline {
    qot_timeline_t info;    /* Basic binding information */
    int fd;                 /* File descriptor to ioctl  */
    pthread_t thread;       /* Thread to poll on fd */
} timeline_t;

/* Is the given timeline a valid one */
qot_return_t timeline_check_fd(timeline_t *timeline) {
    if (fcntl(timeline->fd, F_GETFD)==-1)
        return QOT_RETURN_TYPE_ERR;
    return QOT_RETURN_TYPE_OK;
}

/* Exported methods */

qot_return_t timeline_bind(timeline_t *timeline, const char *uuid,
    const char *name, timelength_t res, timeinterval_t acc) {
    return QOT_RETURN_TYPE_ERR;
}

qot_return_t timeline_unbind(timeline_t *timeline) {
    return QOT_RETURN_TYPE_ERR;
}

qot_return_t timeline_get_accuracy(timeline_t *timeline, timeinterval_t *acc) {
    return QOT_RETURN_TYPE_ERR;
}

qot_return_t timeline_get_resolution(timeline_t *timeline, timelength_t *res) {
    return QOT_RETURN_TYPE_ERR;
}

qot_return_t timeline_get_name(timeline_t *timeline, const char *name) {
    return QOT_RETURN_TYPE_ERR;
}

qot_return_t timeline_get_uuid(timeline_t *timeline, const char *uuid) {
    return QOT_RETURN_TYPE_ERR;
}

qot_return_t timeline_set_accuracy(timeline_t *timeline, timeinterval_t *acc) {
    return QOT_RETURN_TYPE_ERR;
}

qot_return_t timeline_set_resolution(timeline_t *timeline, timelength_t *res) {
    return QOT_RETURN_TYPE_ERR;
}

qot_return_t timeline_gettime(timeline_t *timeline, utimepoint_t *est) {
    return QOT_RETURN_TYPE_ERR;
}

qot_return_t timeline_config_pin_interrupt(timeline_t *timeline,
    qot_perout_t *request, qot_callback_t callback) {
    return QOT_RETURN_TYPE_ERR;
}

qot_return_t timeline_config_pin_timestamp(timeline_t *timeline,
    qot_extts_t *request, qot_callback_t callback) {
    return QOT_RETURN_TYPE_ERR;
}

qot_return_t timeline_config_events(timeline_t *timeline, uint8_t enable,
    qot_callback_t callback) {
    return QOT_RETURN_TYPE_ERR;
}

qot_return_t timeline_waituntil(timeline_t *timeline, utimepoint_t *utp) {
    return QOT_RETURN_TYPE_ERR;
}

qot_return_t timeline_sleep(timeline_t *timeline, utimelength_t *utl) {
    return QOT_RETURN_TYPE_ERR;
}

qot_return_t timeline_timer_create(timeline_t *timeline, utimepoint_t *start,
    utimelength_t *period, int cnt, qot_callback_t callback, timer_t *timer) {
    return QOT_RETURN_TYPE_ERR;
}

qot_return_t timeline_timer_cancel(timeline_t *timeline, timer_t *timer) {
    return QOT_RETURN_TYPE_ERR;
}
