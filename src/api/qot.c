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
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>

// We include the ioctl header here, so that user apps don't need to know about it
#include "../module/qot_ioctl.h"

// File desciptor
static int32_t fd = NO_SCHEDULER_CHDEV;

int32_t qot_init(void)
{
	if (fd < 0)
		fd = open("/dev/qot", O_RDWR);
	return fd;
}

int32_t qot_free(void)
{
	if (fd < 0)
		return NO_SCHEDULER_CHDEV;
	close(fd);

	fd = NO_SCHEDULER_CHDEV;
	
	return SUCCESS;
}

int32_t qot_bind(const char *uuid, uint64_t accuracy, uint64_t resolution)
{
	// Check for scheduler presence, and inititalize if necessary
	if (qot_init() < 0) 
		return NO_SCHEDULER_CHDEV;

	// Check to make sure the UUID is valid
	if (strlen(uuid) > MAX_UUIDLEN)
		return INVALID_UUID;

	// Package up a request	
	qot_message msg;
	msg.bid = 0;
	msg.acc = accuracy;
	msg.res = resolution;
	strncpy(msg.uuid, uuid, MAX_UUIDLEN);
	
	// Add this clock to the qot clock list through scheduler
	if (ioctl(fd, QOT_BIND, &msg) == 0)
		return msg.bid;
	return IOCTL_ERROR;
}

int32_t qot_getclkid(int32_t bid, clockid_t *cid)
{	
    char device[256];
    strcpy(device, QOT_TIMELINE_DIR);
    strcat(device, "temp");	
	int pd = open(device, O_RDWR);
	if (pd < 0)
		return INVALID_CLOCK;
	*cid = ((~(clockid_t) (pd) << 3) | 3);
	close(pd);
	return SUCCESS;
}

int32_t qot_unbind(int32_t bid)
{
	// Check for scheduler presence, and inititalize if necessary
	if (qot_init() < 0) 
		return NO_SCHEDULER_CHDEV;

	// delete this clock from the qot clock list
	if (ioctl(fd, QOT_UNBIND, &bid) == 0)
		return SUCCESS;
	return IOCTL_ERROR;
}

int32_t qot_set_accuracy(int32_t bid, uint64_t accuracy)
{
	// Package up a rewuest
	qot_message msg;
	msg.acc = accuracy;
	msg.bid = bid;
	
	// update this clock
	if (ioctl(fd, QOT_SET_ACCURACY, &msg) == 0)
		return SUCCESS;
	return IOCTL_ERROR;
}

int32_t qot_set_resolution(int32_t bid, uint64_t resolution)
{
	// Package up a rewuest
	qot_message msg;
	msg.res = resolution;
	msg.bid = bid;
	
	// update this clock
	if (ioctl(fd, QOT_SET_RESOLUTION, &msg) == 0)
		return SUCCESS;
	return IOCTL_ERROR;
}
