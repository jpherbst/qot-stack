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

#ifndef QOT_IOCTL_H
#define QOT_IOCTL_H

#include <linux/ioctl.h>

// Maximum length of the identifier
#define MAX_UUIDLEN 		32
#define QOT_TIMELINE_DIR	"/dev/timeline/"

/** clock source structure **/
typedef struct qot_message_t {
	char   	 uuid[MAX_UUIDLEN];			// UUID of reference timeline shared among all collaborating entities
	uint64_t acc;	       				// Range of acceptable deviation from the reference timeline in nanosecond
	uint64_t res;	    				// Required clock resolution
	int      bid;				   		// id returned once the clock is iniatialized
} qot_message;

/** unique code for ioctl **/
#define MAGIC_CODE 0xAB

/** read / write clock and schedule parameters **/
#define QOT_BIND  			    _IOWR(MAGIC_CODE, 1, qot_message*)
#define QOT_GET_ACCURACY		_IOWR(MAGIC_CODE, 5, qot_message*)
#define QOT_GET_RESOLUTION		_IOWR(MAGIC_CODE, 6, qot_message*)
#define QOT_SET_ACCURACY 		 _IOW(MAGIC_CODE, 3, qot_message*)
#define QOT_SET_RESOLUTION 		 _IOW(MAGIC_CODE, 4, qot_message*)
#define QOT_UNBIND 				 _IOW(MAGIC_CODE, 2, int32_t*)
	
#endif
