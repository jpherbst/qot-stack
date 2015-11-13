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

#ifndef QOT_H
#define QOT_H

// Cater for different C compilation pipelines
#ifdef __KERNEL__
	#include <linux/ioctl.h>
#else
	#include <time.h>
	#include <stdint.h>
	#include <sys/ioctl.h>
#endif

// Various system parameters -- name length is bounded by PTP
#define QOT_MAX_NAMELEN 	(32)
#define QOT_MAX_DATALEN 	(128)
#define QOT_IOCTL_BASE		"/dev"
#define QOT_IOCTL_QOT		"qot"
#define QOT_IOCTL_TIMELINE	"ptp"
#define QOT_IOCTL_FORMAT	"%3s%d" 
#define QOT_POLL_TIMEOUT_MS (1000)

// Timeline events
#define EVENT_CREATE		(0)
#define EVENT_DESTROY		(1)
#define EVENT_JOIN			(2)
#define EVENT_LEAVE			(3)
#define EVENT_SYNC			(4)
#define EVENT_CAPTURE		(5)
#define EVENT_UPDATE		(6)

// QoT message type 
typedef struct qot_metric {
	uint64_t acc;	       				// Range of acceptable deviation from the reference timeline in nanosecond
	uint64_t res;	    				// Required clock resolution
} qot_metric_t;

// QoT capture information
typedef struct qot_event {
	char info[QOT_MAX_DATALEN];			// Event data
	uint8_t type;						// Event type code
} qot_event_t;

// QoT capture information
typedef struct qot_capture {
	char name[QOT_MAX_NAMELEN];			// Name of the pin, eg. timer1, timer2, etc...
	int64_t edge;						// Edge time in global coordinates
} qot_capture_t;

// QoT compare information
typedef struct qot_compare {
	char name[QOT_MAX_NAMELEN];			// Name of the pin, eg. timer1, timer2, etc...
	uint8_t enable;						// Turn on or off
	int64_t start;						// Global time of first rising edge
	uint64_t high;						// Duty cycle high time (in global time units)
	uint64_t low;						// Duty cycle low time (in global time units)
	uint64_t repeat;					// Number of repetitions (0 = repeat indefinitely)
} qot_compare_t;

// QoT message type 
typedef struct qot_message {
	char uuid[QOT_MAX_NAMELEN];			// UUID of reference timeline shared among all collaborating entities
	struct qot_metric request;			// Request metrics			
	int tid;				   			// Timeline id (ie. X in /dev/timelineX)
	struct qot_capture capture;			// Capture information
	struct qot_compare compare;			// Compare information
	struct qot_event event;				// Event information
} qot_message_t;

// Magic code specific to our ioctl code
#define MAGIC_CODE 0xEF

// Used by the QoT API
#define QOT_BIND_TIMELINE	    _IOWR(MAGIC_CODE,  0, struct qot_message*)
#define QOT_SET_ACCURACY 		 _IOW(MAGIC_CODE,  1, struct qot_message*)
#define QOT_SET_RESOLUTION 		 _IOW(MAGIC_CODE,  2, struct qot_message*)
#define QOT_UNBIND_TIMELINE		 _IOW(MAGIC_CODE,  3, struct qot_message*)
#define QOT_SET_COMPARE 		 _IOW(MAGIC_CODE,  4, struct qot_message*)
#define QOT_GET_CAPTURE 		 _IOR(MAGIC_CODE,  5, struct qot_message*)
#define QOT_GET_EVENT 			 _IOR(MAGIC_CODE,  6, struct qot_message*)

// Used by the QoT daemon (all indexed on id)
#define QOT_GET_TARGET 			 _IOR(MAGIC_CODE,  7, struct qot_message*)
#define QOT_SET_ACTUAL 			 _IOW(MAGIC_CODE,  8, struct qot_message*)
#define QOT_PUSH_EVENT 			 _IOW(MAGIC_CODE,  9, struct qot_message*)

// Used by linuxptp 
#define QOT_PROJECT_TIME 		_IOWR(MAGIC_CODE, 10, int64_t*)

#endif
