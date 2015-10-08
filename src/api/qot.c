/*
 * @file qot.c
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

// This library include
#include "qot.h"

// Standard includes
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

// We include the ioctl header here, so that user apps don't need to know about it
#include "../module/qot_ioctl.h"

// Maintain a map of binding ids to clock ids
static clockid_t bid2cid[QOT_MAX_BINDINGS+1];

// Private: open the ioctl 
static int32_t qot_open(void)
{
	return open("/dev/qot", O_RDWR);
}

// Private: close the ioctl
static int32_t qot_close(int32_t fd)
{
	if (fd < 0)
		return NO_SCHEDULER_CHDEV;
	close(fd);
	return SUCCESS;
}

// Bind to a timeline with a given resolution and accuracy
int32_t qot_bind_timeline(const char *uuid, uint64_t accuracy, uint64_t resolution)
{
    char device[256];

	// Open a channel to the scheduler
	int32_t fd = qot_open();
	if (fd < 0)
		return NO_SCHEDULER_CHDEV;

	// Check to make sure the UUID is valid
	if (strlen(uuid) > QOT_MAX_UUIDLEN)
		return INVALID_UUID;

	// Package up a request	
	qot_message msg;
	msg.bid = 0;
	msg.tid = 0;
	msg.acc = accuracy;
	msg.res = resolution;
	strncpy(msg.uuid, uuid, QOT_MAX_UUIDLEN);

	// Default return code
	int32_t ret = IOCTL_ERROR;

	// Add this clock to the qot clock list through scheduler
	if (ioctl(fd, QOT_BIND, &msg) == SUCCESS)
	{
		// Special case: problematic binding id
		if (msg.bid < 0)
			return INVALID_BINDING_ID;

		// Construct the file handle tot he poix clock /dev/timeline/timelineX
	    sprintf(device, "%s%u", QOT_TIMELINE_PREFIX, msg.tid);
		
		// Open the clock
		int pd = open(device, O_RDWR);
		if (pd < 0)
			return INVALID_CLOCK;

		// Obtain the clock ID from the POSIX clock
		bid2cid[msg.bid] = ((~(clockid_t) (pd) << 3) | 3);

		// Close the POSIX clock
		close(pd);

		// Return the binding ID
		ret = msg.bid;
	}

	// Close communication with the scheduler
	qot_close(fd);

	// Could not communicate with scheduler
	return ret;
}

// Unbind from a timeline
int32_t qot_unbind_timeline(int32_t bid)
{
	// Open a channel to the scheduler
	int32_t fd = qot_open();
	if (fd < 0)
		return NO_SCHEDULER_CHDEV;

	// Package up a rewuest
	qot_message msg;
	msg.bid = bid;

	// Default return code
	int32_t ret = IOCTL_ERROR;

	// Delete this clock from the qot clock list
	if (ioctl(fd, QOT_UNBIND, &msg) == SUCCESS)
	{
		// Invalidate the binding
		bid2cid[bid] = -1;

		// Success
		ret = SUCCESS;
	}

	// Close communication with the scheduler
	qot_close(fd);

	// Return success code
	return ret;
}

int32_t qot_set_accuracy(int32_t bid, uint64_t accuracy)
{
	// Open a channel to the scheduler
	int32_t fd = qot_open();
	if (fd < 0)
		return NO_SCHEDULER_CHDEV;

	// Package up a rewuest
	qot_message msg;
	msg.acc = accuracy;
	msg.bid = bid;
	
	// Default return code
	int32_t ret = IOCTL_ERROR;

	// update this clock
	if (ioctl(fd, QOT_SET_ACCURACY, &msg) == SUCCESS)
		ret = SUCCESS;

	// Close communication with the scheduler
	qot_close(fd);

	// Return success code
	return ret;
}

int32_t qot_set_resolution(int32_t bid, uint64_t resolution)
{
	// Open a channel to the scheduler
	int32_t fd = qot_open();
	if (fd < 0)
		return NO_SCHEDULER_CHDEV;

	// Package up a rewuest
	qot_message msg;
	msg.res = resolution;
	msg.bid = bid;
	
	// Default return code
	int32_t ret = IOCTL_ERROR;

	// update this clock
	if (ioctl(fd, QOT_SET_RESOLUTION, &msg) == SUCCESS)
		ret = SUCCESS;

	// Close communication with the scheduler
	qot_close(fd);

	// Return success code
	return ret;
}

int32_t qot_get_achieved(int32_t bid, uint64_t *accuracy, uint64_t *resolution)
{
	// Open a channel to the scheduler
	int32_t fd = qot_open();
	if (fd < 0)
		return NO_SCHEDULER_CHDEV;
	if (!accuracy && !resolution)
		return SUCCESS;

	// Package up a rewuest
	qot_message msg;
	msg.bid = bid;
	
	// Default return code
	int32_t ret = IOCTL_ERROR;

	// update this clock
	if (ioctl(fd, QOT_GET_ACHIEVED, &msg) == SUCCESS)
	{
		// Only copy accuray if the poitner is valid
		if (accuracy)
			*accuracy = msg.acc;
		
		// Only copy resolution if the poitner is valid
		if (resolution)
			*resolution = msg.res;
		
		ret = SUCCESS;
	}

	// Close communication with the scheduler
	qot_close(fd);

	// Return success code
	return ret;
}

int32_t qot_gettime(int32_t bid, struct timespec *ts, uint64_t *accuracy, uint64_t *resolution)
{
	// Basic checks
	if (bid < QOT_MAX_BINDINGS)
		return INVALID_BINDING_ID;
	if (bid2cid[bid] < 0)
		return INVALID_CLOCK;

	// Get the time 
	int32_t ret = clock_gettime(bid2cid[bid], ts);

	// Get the stats
	if (accuracy || resolution)
		return qot_get_achieved(bid, accuracy, resolution);

	// Return success
	return ret;
}

int32_t qot_wait_until(int32_t bid, struct timespec *ts)
{
	// Open a channel to the scheduler
	int32_t fd = qot_open();
	if (fd < 0)
		return NO_SCHEDULER_CHDEV;

	// Package up a rewuest
	qot_message msg;
	msg.bid = bid;
	memcpy(&msg.event, ts, sizeof(struct timespec));
	
	// Default return code
	int32_t ret = IOCTL_ERROR;

	// update this clock
	if (ioctl(fd, QOT_WAIT_UNTIL, &msg) == SUCCESS)
		ret = SUCCESS;

	// Close communication with the scheduler
	qot_close(fd);

	// Return success code
	return ret;
}
