#include "qot.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>

#include "../module/qot_ioctl.h"

// File desciptor
static int32_t fd = NO_SCHEDULER_CHDEV;

// Try and open
int32_t qot_init(void)
{
	if (fd < 0)
		fd = open("/dev/qot", O_RDWR);
	return fd;
}

// Try and open
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

	//
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

int32_t qot_set_desired_accuracy(int32_t bid, uint64_t accuracy)
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

int32_t qot_set_desired_resolution(int32_t bid, uint64_t resolution)
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

int32_t qot_get_target_accuracy(int32_t bid, uint64_t *accuracy)
{
	// Package up a rewuest
	qot_message msg;
	msg.bid = bid;
	
	// update this clock
	if (ioctl(fd, QOT_GET_ACCURACY, &msg) == 0)
	{
		(*accuracy) = msg.acc;
		return SUCCESS;
	}
	return IOCTL_ERROR;
}

int32_t qot_get_target_resolution(int32_t bid, uint64_t *resolution)
{
	// Package up a rewuest
	qot_message msg;
	msg.bid = bid;
	
	// update this clock
	if (ioctl(fd, QOT_GET_RESOLUTION, &msg) == 0)
	{
		(*resolution) = msg.res;
		return SUCCESS;
	}
	return IOCTL_ERROR;	
}
