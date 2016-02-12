/*
 * @file qot_user.h
 * @brief user interface for the QoT stack
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

#ifndef QOT_STACK_SRC_MODULES_QOT_QOT_USER_H
#define QOT_STACK_SRC_MODULES_QOT_QOT_USER_H

#include "qot_core.h"

/* qot_admin_user.c */

/**
 * @brief Clean up the user subsystem
 * @param qot_class Device class for all QoT devices
 **/
void qot_user_cleanup(struct class *qot_class);

/**
 * @brief Initialize the user subsystem
 * @param qot_class Device class for all QoT devices
 * @return A status code indicating success (0) or failure (!0)
 **/
qot_return_t qot_user_init(struct class *qot_class);

/* qot_admin_chdev.c */

/**
 * @brief Clean up the user character device subsystem
 * @param qot_class Device class for all QoT devices
 **/
void qot_user_chdev_cleanup(struct class *qot_class);

/**
 * @brief Initialize the user character device subsystem
 * @param qot_class Device class for all QoT devices
 * @return A status code indicating success (0) or failure (!0)
 **/
qot_return_t qot_user_chdev_init(struct class *qot_class);


#endif


