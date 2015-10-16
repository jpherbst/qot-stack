/*
 * @file qot_ioctl.h
 * @brief IOCTL interface for communication between the scheduler and API
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

#ifndef _QOT_H_
#define _QOT_H_

#include <linux/ioctl.h>

// Various system parameters
#define QOT_MAX_BINDINGS    (65536)
#define QOT_MAX_UUIDLEN 	(32)
#define QOT_HASHTABLE_BITS	(16)
#define QOT_IOCTL_CORE		"qot"
#define QOT_IOCTL_TIMELINE	"timeline"

// QoT message type 
struct qot_metric {
	uint64_t acc;	       				// Range of acceptable deviation from the reference timeline in nanosecond
	uint64_t res;	    				// Required clock resolution
};

// QoT message type 
struct qot_message {
	char uuid[QOT_MAX_UUIDLEN];			// UUID of reference timeline shared among all collaborating entities
	struct qot_metric request;			// Request metrics			
	int bid;				   			// Binding id 
	int tid;				   			// Timeline id (ie. X in /dev/timelineX)
	struct timespec event;				// Timestamp
};

// Magic code specific to our ioctl code
#define MAGIC_CODE 0xEF

// IOCTL with /dev/qot
#define QOT_BIND_TIMELINE	    _IOWR(MAGIC_CODE, 1, struct qot_message*)
#define QOT_SET_ACCURACY 		 _IOW(MAGIC_CODE, 2, struct qot_message*)
#define QOT_SET_RESOLUTION 		 _IOW(MAGIC_CODE, 3, struct qot_message*)
#define QOT_UNBIND_TIMELINE		 _IOW(MAGIC_CODE, 4, struct qot_message*)

// IOCTL with /dev/timeline*
#define QOT_GET_ACTUAL_METRIC 	 _IOR(MAGIC_CODE, 5, struct qot_metric*)
#define QOT_GET_TARGET_METRIC 	 _IOR(MAGIC_CODE, 6, struct qot_metric*)
#define QOT_GET_UUID 			 _IOR(MAGIC_CODE, 7, char*)

#endif
