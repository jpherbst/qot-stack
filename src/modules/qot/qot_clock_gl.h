/*
 * @file qot_clock_gl.h
 * @brief Interface to clock management for global timelines in the QoT stack
 * @author Sandeep D'souza
 *
 * Copyright (c) Carnegie Mellon University 2018.
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

#ifndef QOT_STACK_SRC_MODULES_QOT_QOT_CLOCK_GL_H
#define QOT_STACK_SRC_MODULES_QOT_QOT_CLOCK_GL_H

#include "qot_core.h"

/* Read the global time for global timelines */
qot_return_t qot_clock_gl_get_time(utimepoint_t *utp);

/* Read the raw CLOCK_REALTIME */
qot_return_t qot_clock_gl_get_time_raw(timepoint_t *tp);

/* Set the global core clock time*/
qot_return_t qot_clock_gl_settime(timepoint_t tp);

/* Adjust the global clock frequency */
qot_return_t qot_clock_gl_adjfreq(s32 ppb);

/* Adjust the global clock time */
qot_return_t qot_clock_gl_adjtime(s64 delta);

/* Set the global clock uncertainty */
qot_return_t qot_clock_gl_set_uncertainty(qot_bounds_t bounds);

/* Get the global timeline mapping parameters */
qot_return_t qot_clock_gl_get_params(tl_translation_t *params);

/* Project from global clock to global timeline */
qot_return_t qot_gl_loc2rem(int period, s64 *val);

/* Project from global timeline to global clock */
qot_return_t qot_gl_rem2loc(int period, s64 *val);

/* Cleanup the global core clock */
qot_return_t qot_clock_gl_cleanup(void);

/* Initialize the global clock */
qot_return_t qot_clock_gl_init(void);

#endif