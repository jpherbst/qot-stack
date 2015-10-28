/*
 * @file qot_ioctl.h
 * @brief IOCTL interface for communication between the scheduler and API
 * @author Andrew Symington and Fatima Anwar 
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

#ifndef _QOT_H_
#define _QOT_H_

#include <linux/ioctl.h>

// Various system parameters
#define QOT_MAX_UUIDLEN 	(32)
#define QOT_MAX_NAMELEN 	(32)
#define QOT_HASHTABLE_BITS	(16)
#define QOT_IOCTL_CORE		"qot"
#define QOT_IOCTL_TIMELINE	"timeline"
#define QOT_POLL_TIMEOUT_MS  (1000)
#define QOT_ACTION_CAPTURE	POLLIN
#define QOT_ACTION_EVENT	POLLOUT

// Actions
enum qot_event {
	BIND   = 0,
	UNBIND = 1
};

// QoT message type 
struct qot_metric {
	uint64_t acc;	       				// Range of acceptable deviation from the reference timeline in nanosecond
	uint64_t res;	    				// Required clock resolution
};

// QoT capture information
struct qot_capture {
	char name[QOT_MAX_NAMELEN];			// Name of the pin, eg. timer1, timer2, etc...
	uint8_t enable;						// Turn on or off
	int64_t event;						// Event (in global time!)
	uint32_t seq;						// Prevents duplicate sequence number
};

// QoT compare information
struct qot_compare {
	char name[QOT_MAX_NAMELEN];			// Name of the pin, eg. timer1, timer2, etc...
	uint8_t enable;						// Turn on or off
	int64_t start;						// Global time of first rising edge
	uint32_t high;						// Duty cycle high time (in global time units)
	uint32_t low;						// Duty cycle low time (in global time units)
	uint32_t repeat;					// Number of repetitions (0 = repeat indefinitely)
};

// QoT message type 
struct qot_message {
	char uuid[QOT_MAX_UUIDLEN];			// UUID of reference timeline shared among all collaborating entities
	char name[QOT_MAX_UUIDLEN];			// Name of the entity 
	struct qot_metric request;			// Request metrics			
	int bid;				   			// Binding id 
	int tid;				   			// Timeline id (ie. X in /dev/timelineX)
	int event;							// 1 = bind, 1 = unbind
	struct qot_capture capture;			// Capture information
	struct qot_compare compare;			// Compare information
};

// Magic code specific to our ioctl code
#define MAGIC_CODE 0xEF

// IOCTL with /dev/qot
#define QOT_BIND_TIMELINE	    _IOWR(MAGIC_CODE,  0, struct qot_message*)
#define QOT_SET_ACCURACY 		 _IOW(MAGIC_CODE,  1, struct qot_message*)
#define QOT_SET_RESOLUTION 		 _IOW(MAGIC_CODE,  2, struct qot_message*)
#define QOT_UNBIND_TIMELINE		 _IOW(MAGIC_CODE,  3, struct qot_message*)
#define QOT_GET_EVENT 			 _IOR(MAGIC_CODE,  4, struct qot_message*)
#define QOT_GET_CAPTURE 		 _IOR(MAGIC_CODE,  5, struct qot_message*)
#define QOT_SET_COMPARE 		 _IOW(MAGIC_CODE,  6, struct qot_message*)

// IOCTL with /dev/timeline*
#define QOT_GET_ACTUAL_METRIC 	 _IOR(MAGIC_CODE,  7, struct qot_metric*)
#define QOT_SET_ACTUAL_METRIC 	 _IOW(MAGIC_CODE,  8, struct qot_metric*)
#define QOT_GET_TARGET_METRIC 	 _IOR(MAGIC_CODE,  9, struct qot_metric*)
#define QOT_GET_METADATA 	 	 _IOR(MAGIC_CODE, 10, struct qot_message*)

#endif
