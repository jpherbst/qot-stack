/*
 * @file qot.cpp
 * @brief Userspace C++ API to manage QoT timelines
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
#include "qot.hpp"

// C++ includes
#include <exception>
#include <sstream>

// Private functionality
extern "C"
{
	// Userspeace ioctl
	#include <sys/ioctl.h>

	// Our module communication protocol
	#include "../module/qot.h"
}

using namespace qot;

Timeline::Timeline(const char* uuid, uint64_t acc, uint64_t res)
{
	// Open the QoT scheduler
	this->fd_qot = open("/dev/qot", O_RDWR);
	if (this->fd_qot < 0)
		throw CannotCommunicateWithCoreException();

	// Check to make sure the UUID is valid
	if (strlen(uuid) > QOT_MAX_UUIDLEN)
		throw ProblematicUUIDException();

	// Package up a request	to send over ioctl
	struct qot_message msg;
	msg.bid = 0;
	msg.tid = 0;
	msg.request.acc = acc;
	msg.request.res = res;
	strncpy(msg.uuid, uuid, QOT_MAX_UUIDLEN);

	// Add this clock to the qot clock list through scheduler
	if (ioctl(this->fd_qot, QOT_BIND_TIMELINE, &msg) == 0)
	{
		// Special case: problematic binding id
		if (msg.bid < 0)
			throw CannotBindToTimelineException();

		// Save the binding id
		this->bid = msg.bid;

		// Construct the file handle tot he poix clock /dev/timelineX
		std::ostringstream oss("/dev/");
		oss << QOT_IOCTL_TIMELINE;
		oss << msg.tid;
		
		// Open the clock
		this->fd_clk = open(oss.str().c_str(), O_RDWR);
		if (this->fd_clk < 0)
			throw CannotOpenPOSIXClockException();

		// Convert the file descriptor to a clock handle
		this->clk = ((~(clockid_t) (this->fd_clk) << 3) | 3);
	}
}

Timeline::~Timeline()
{
	// Close the clock and timeline
	if (this->fd_clk) close(fd_clk);
	if (this->fd_qot) close(fd_qot);
}

int Timeline::SetAccuracy(uint64_t acc)
{
	// Package up a rewuest
	struct qot_message msg;
	msg.request.acc = acc;
	msg.bid = this->bid;
	
	// Update this clock
	if (ioctl(this->fd_qot, QOT_SET_ACCURACY, &msg) == 0)
		return 0;

	// We only get here on failure
	throw CommunicationWithCoreFailedException();
}

int Timeline::SetResolution(uint64_t res)
{
	// Package up a rewuest
	struct qot_message msg;
	msg.request.res = res;
	msg.bid = this->bid;
	
	// Update this clock
	if (ioctl(this->fd_qot, QOT_SET_RESOLUTION, &msg) == 0)
		return 0;

	// We only get here on failure
	throw CommunicationWithCoreFailedException();
}

int64_t Timeline::GetTime()
{	
	struct timespec ts;
	int ret = clock_gettime(this->clk, &ts);
	return (int64_t) ts.tv_sec * 1000000000ULL + (int64_t) ts.tv_nsec;
}

int Timeline::RequestCapture(const char* pname, uint8_t enable,
	void (callback)(const char *pname, int64_t val))
{
	return 0;
}

int Timeline::RequestCompare(const char* pname, uint8_t enable, 
	int64_t start, uint64_t high, uint64_t low, uint64_t limit)
{
	return 0;
}

int Timeline::WaitUntil(uint64_t val)
{
	return 0;
}