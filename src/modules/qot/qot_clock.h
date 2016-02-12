/*
 * @file qot_clock.h
 * @brief Interface to clock management in the QoT stack
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

#ifndef QOT_STACK_SRC_MODULES_QOT_QOT_CLOCK_H
#define QOT_STACK_SRC_MODULES_QOT_QOT_CLOCK_H

#include <linux/posix-clock.h>
#include <linux/poll.h>

#include "qot_internal.h"

/**
 * @brief Find the next clock in the system based on the specified name.
 * @param clk A pointer to the current clock with the name field populated
 * @return A status code indicating success (0) or other (no more clocks)
 **/
qot_return_t qot_clock_next(qot_clock_t *clk);

/**
 * @brief Get information about a clock with the specified name.
 * @param clk A pointer to the current clock with the name field populated
 * @return A status code indicating success (0) or other (no more clocks)
 **/
qot_return_t qot_clock_get_info(qot_clock_t *clk);

/**
 * @brief Remove a clock with the specified name.
 * @param clk A pointer to the current clock with the name field populated
 * @return A status code indicating success (0) or other (no more clocks)
 **/
qot_return_t qot_clock_remove(qot_clock_t *clk);

/**
 * @brief Ask a clock with a given name to sleep. Core will not sleep.
 * @param clk A pointer to the current clock with the name field populated
 * @return A status code indicating success (0) or other (no more clocks)
 **/
qot_return_t qot_clock_sleep(qot_clock_t *clk);

/**
 * @brief Wake up a clock with the given name
 * @param clk A pointer to the current clock with the name field populated
 * @return A status code indicating success (0) or other (no more clocks)
 **/
qot_return_t qot_clock_wake(qot_clock_t *clk);

/**
 * @brief Switch core to be driven by a clock with a given na,e
 * @param clk A pointer to the current clock with the name field populated
 * @return A status code indicating success (0) or other (no more clocks)
 **/
qot_return_t qot_clock_switch(qot_clock_t *clk);


/**
 * @brief Clean up the clock subsystem
 * @param qot_class Device class for all QoT devices
 **/
void qot_clock_cleanup(struct class *qot_class);

/**
 * @brief Initialize the clock subsystem
 * @param qot_class Device class for all QoT devices
 * @return A status code indicating success (0) or failure (!0)
 **/
qot_return_t qot_clock_init(struct class *qot_class);

#endif