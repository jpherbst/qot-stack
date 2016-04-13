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

#ifndef QOT_STACK_SRC_MODULES_QOT_QOT_CORE_H
#define QOT_STACK_SRC_MODULES_QOT_QOT_CORE_H

#include <linux/ptp_clock_kernel.h>

/* Public interface */
#include "../../qot_types.h"

/**
 * @brief Information about a platform clock
 **/
typedef struct qot_clock_impl {
    qot_clock_t info;                 /* Description of this clock      */
    struct ptp_clock_info ptpclk;     /* The PTP interface to the clock */
    timepoint_t (*read_time)(void);
    long (*program_interrupt)(timepoint_t expiry, int force, long (*callback)(void));
    long (*cancel_interrupt)(void);
    long (*enable_compare)(timepoint_t *core_start, timepoint_t *core_period, qot_perout_t *perout, s64 (*callback)(qot_perout_t *perout_ret), int on);
    long (*sleep)(void);
    long (*wake)(void);
} qot_clock_impl_t;

/**
 * @brief Register a clock with the QoT core
 * @param impl A pointer containing all the clock info
 * @return A status code indicating success (0) or other
 **/
qot_return_t qot_register(qot_clock_impl_t *impl);

/**
 * @brief Unregister a clock with the QoT core
 * @param impl A pointer containing all the clock info
 * @return A status code indicating success (0) or other
 **/
qot_return_t qot_unregister(qot_clock_impl_t *impl);

/**
 * @brief Prompt QoT core into updating the clock information
 * @param impl A pointer containing all the clock info
 * @return A status code indicating success (0) or other
 **/
qot_return_t qot_update(qot_clock_impl_t *impl);

#endif