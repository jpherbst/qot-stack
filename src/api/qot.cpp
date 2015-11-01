/*
 * @file qot.cpp
 * @brief Userspace C++ API to manage QoT timelines
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

// This library include
#include "qot.hpp"

// C++ includes
#include <exception>
#include <sstream>

// Private functionality
extern "C"
{
	// Userspeace ioctl
	#include <unistd.h>
	#include <fcntl.h>
	#include <stdio.h>
	#include <string.h>
	#include <stdint.h>
	#include <time.h>
	#include <sys/ioctl.h>
	#include <sys/poll.h>

	// Our module communication protocol
	#include "../module/qot.h"
}

using namespace qot;

// Bind to a timeline
Timeline::Timeline(const std::string &uuid, uint64_t acc, uint64_t res)
	: name(RandomString(32)), fd_qot(-1), lk(this->m), 
		thread(std::bind(&Timeline::CaptureThread, this))
{
	// Open the QoT scheduler
	this->fd_qot = open("/dev/qot", O_RDWR);
	if (this->fd_qot < 0)
		throw CannotCommunicateWithCoreException();

	// Check to make sure the UUID is valid
	if (uuid.length() > QOT_MAX_NAMELEN)
		throw ProblematicUUIDException();

	// Package up a request	to send over ioctl
	struct qot_message msg;
	msg.bid = 0;
	msg.tid = 0;
	msg.request.acc = acc;
	msg.request.res = res;
	strncpy(msg.uuid, uuid.c_str(), QOT_MAX_NAMELEN);

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

		// We can now start polling, because the timeline is setup
    	this->cv.notify_one();
	}
}

// Unbind from a timeline
Timeline::~Timeline()
{
	// Join the thread
    thread.join();

	// Close the clock and timeline
	if (this->fd_clk) close(fd_clk);
	if (this->fd_qot) close(fd_qot);
}

// Set the binding accuracy
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

// Set the binding resolution
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

// Get the current time
int64_t Timeline::GetTime()
{	
	struct timespec ts;
	int ret = clock_gettime(this->clk, &ts);
	return (int64_t) ts.tv_sec * 1000000000ULL + (int64_t) ts.tv_nsec;
}

// Get the achieved accuracy
uint64_t Timeline::GetAccuracy()
{
	// Package up a rewuest
	struct qot_metric msg;
	if (ioctl(this->fd_clk, QOT_GET_ACTUAL_METRIC, &msg) == 0)
		return msg.acc;
	throw CommunicationWithPOSIXClockException();
}

// Get the achieved resolutions
uint64_t Timeline::GetResolution()
{
	// Package up a rewuest
	struct qot_metric msg;
	if (ioctl(this->fd_clk, QOT_GET_ACTUAL_METRIC, &msg) == 0)
		return msg.res;
	throw CommunicationWithPOSIXClockException();
}

// Get the name of the application
std::string Timeline::GetName()
{
	return this->name;
}

// Set the name of the application
void Timeline::SetName(const std::string &name)
{
	this->name = name;
}

// Generate an interrupt on a given timer pin
int Timeline::GenerateInterrupt(const std::string& pname, uint8_t enable, 
	int64_t start, uint32_t high, uint32_t low, uint32_t repeat)
{
	// Check to make sure the pin name is valid
	if (pname.length() > QOT_MAX_NAMELEN)
		throw ProblematicPinNameException();

	// Package up a request
	struct qot_message msg;
	msg.bid = this->bid;
	msg.compare.enable = enable;
	msg.compare.start = start;
	msg.compare.high = high;
	msg.compare.low = low;
	msg.compare.repeat = repeat;
	strcpy(msg.compare.name,pname.c_str());
	
	// Update this clock
	if (ioctl(this->fd_qot, QOT_SET_COMPARE, &msg) == 0)
		return 0;

	// We only get here on failure
	throw CommunicationWithCoreFailedException();
}

// Wait until some global time
int64_t Timeline::WaitUntil(int64_t val)
{
	// TODO:
	// Make a nanosleep-like system call to the QoT code, which passes a binding
	// ID and  the absolute wake-up time to the scheduler. The scheduler can then
	// perfom a busy wait until the global time is reached. To do this the 
	// scheduler periodically back-projects the global time to a local time. When
	// the current local time is sufficiently close to the desired wake up time, 
	// then the scheduler starts a high resolution timer to fire at the event. 
	// eg. qot_absolute_nanosleep(this->bid, val);
	return 0;
}

// Listen for an interrupt on a pin
void Timeline::SetCaptureCallback(CaptureCallbackType callback)
{
	this->cb_capture = callback;
}

void Timeline::SetEventCallback(EventCallbackType callback)
{
	this->cb_event = callback;
}

// A thread to manage file descriptor polling (should introduce a mutex)
void Timeline::CaptureThread()
{
	// Wait until the main thread sets up the binding and posix clock
    while (fd_qot < 0) 
    	this->cv.wait(this->lk);

    // Start polling
    while (this->fd_qot > 0)
    {
    	// Initialize the polling struct
		struct pollfd pfd[1];
		memset(pfd,0,sizeof(pfd));
		pfd[0].fd = this->fd_qot;
		pfd[0].events = POLLIN;

		// Wait until an asynchronous data push from the kernel module
		if (poll(pfd,1,QOT_POLL_TIMEOUT_MS))
		{
			// We have received notification of a capture event
			if (pfd[0].revents & QOT_ACTION_CAPTURE)
			{
				// Keep querying for capture events until there are none left
				qot_message msg;
				msg.bid = this->bid;
				while (ioctl(this->fd_qot, QOT_GET_CAPTURE, &msg) == 0)
					if (cb_capture) 
						this->cb_capture(msg.capture.name, msg.capture.edge);
			}

			// We have received notification of a device bind/unbind
			if (pfd[0].revents & QOT_ACTION_EVENT)
			{
				// Keep querying for capture events until there are none left
				qot_message msg;
				msg.bid = this->bid;
				while (ioctl(this->fd_qot, QOT_GET_EVENT, &msg) == 0)
					if (cb_event) 
						this->cb_event(msg.event.name, msg.event.type);
			}
		}
	}
}

// Random string generator
std::string Timeline::RandomString(uint32_t length)
{
    auto randchar = []() -> char
    {
        const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        const size_t max_index = (sizeof(charset) - 1);
        return charset[ rand() % max_index ];
    };
    std::string str(length,0);
    std::generate_n( str.begin(), length, randchar );
    return str;
}