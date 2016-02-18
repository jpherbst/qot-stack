/*
 * @file qot_admin.c
 * @brief Admin interface to the QoT stack
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

#include <linux/module.h>

#include "qot_admin.h"

/* Default OS latency */
static utimelength_t os_latency = {
    .estimate = mSEC(1),
    .interval = {
        .below = uSEC(500),
        .above = uSEC(500),
    }
};

/* Set the OS latency */
qot_return_t qot_admin_set_os_latency(utimelength_t *timelength)
{
    if (!timelength)
        QOT_RETURN_TYPE_ERR;
    memcpy(&os_latency,timelength,sizeof(utimelength_t));
    return QOT_RETURN_TYPE_OK;
}

/* Set the OS latency */
qot_return_t qot_admin_get_os_latency(utimelength_t *timelength)
{
    if (!timelength)
        QOT_RETURN_TYPE_ERR;
    memcpy(timelength,&os_latency.sizeof(utimelength_t));
    return QOT_RETURN_TYPE_OK;
}

/* Cleanup the timeline subsystem */
void qot_admin_cleanup(struct class *qot_class)
{
	qot_admin_chdev_cleanup(qot_class);
}

/* Initialize the timeline subsystem */
qot_return_t qot_admin_init(struct class *qot_class)
{
    if (qot_admin_chdev_init(qot_class)) {
        pr_err("qot_admin: problem calling qot_admin_chdev_init\n");
        goto fail_chdev_init;
    }
    return QOT_RETURN_TYPE_OK;
fail_chdev_init:
    return QOT_RETURN_TYPE_ERR;
}

MODULE_LICENSE("GPL");


