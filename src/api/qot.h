/*
 * @file qot.h
 * @brief Userspace API to manage QoT timelines
 * @author Fatima Anwar 
 * 
 * Copyright (c) Regents of the University of California, 2015. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 * 	1. Redistributions of source code must retain the above copyright notice, 
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, 
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef QOT_H
#define QOT_H

#include <stdint.h>
#include <time.h>

/**
 * @brief Error codes for functions
 */
typedef enum
{
	SUCCESS            =  0,
	NO_SCHEDULER_CHDEV = -1,	 /**< No scheduler character device */
	INVALID_BINDING_ID = -2,	 /**< Invalid binding ID specified */
	INVALID_FD  	   = -3,	 /**< Invalid binding ID specified */
	INVALID_UUID       = -4,	 /**< Invalid timeline UUID specified */
	INVALID_CLOCK      = -5,	 /**< IOCTL returned error */
	IOCTL_ERROR        = -6		 /**< IOCTL returned error */
} 
qot_error;

/**
 * @brief Bind to a timeline
 * @param uuid A unique identifier for the timeline, eg. "sample_timeline"
 * @param accuracy The desired accuracy in nanoseconds
 * @param resolution The desired resolution in nanoseconds
 * @return A non-negative binding id for the binding. Negative: error
 *
 **/
int32_t qot_bind_timeline(const char *uuid, uint64_t accuracy, uint64_t resolution);

/**
 * @brief Unbind from a timeline
 * @param bid The binding id
 * @return Positive: success Negative: error
 **/
int32_t qot_unbind_timeline(int32_t bid);

/**
 * @brief Update the requested binding accuracy
 * @param bid The binding id
 * @param accuracy The new accuracy
 * @return Positive: success Negative: error
 **/
int32_t qot_set_accuracy(int32_t bid, uint64_t accuracy);

/**
 * @brief Update the requested binding resolution
 * @param bid The binding id
 * @param accuracy The new resolution
 * @return Positive: success Negative: error
 **/
int32_t qot_set_resolution(int32_t bid, uint64_t resolution);

/**
 * @brief Get the current global timeline
 * @param bid The binding id
 * @param ts Pointer to timespec struct 
 * @param accuracy Attained accuracy
 * @param resolution Attained resoltion
 * @return Positive: success Negative: error
 **/
int32_t qot_gettime(int32_t bid, struct timespec *ts);

/**
 * @brief Wait until a specific time
 * @param bid The binding id
 * @param ts Pointer to timespec struct representing event
 * @return Positive: success Negative: error
 **/
int32_t qot_wait_until(int32_t bid, struct timespec *ts);

#endif
