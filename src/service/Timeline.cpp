/*
 * @file Timeline.cpp
 * @brief Library to manage Quality of Time POSIX clocks
 * @author Andrew Symington
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

/* This file header */
#include "Timeline.hpp"

/* Include our QOT API */
extern "C"
{
	#include "../module/qot.h"
}

using namespace qot;

Timeline::Timeline(boost::asio::io_service *io, const std::string &file)
	: 	coordinator(io), lk(this->m), kill(false), thread(std::bind(&Timeline::MonitorThread, this))
{
	// Try and open the file
	BOOST_LOG_TRIVIAL(info) << "Opening IOCTL to timeline" << file;
	fd = open(file.c_str(), O_RDWR);
	if (fd < 0)
	{
		BOOST_LOG_TRIVIAL(error) << "Could not open the timeline " << file;
		return;
	}	

	// Extract the timeline information
	this->Update();

	// We can now start polling, because the timeline is setup
	this->cv.notify_one();
}

Timeline::~Timeline()
{
	// Kill the thread
	this->kill = true;

	// Threads must now exit
    this->thread.join();

	// Close ioctl
	if (fd > 0)
		close(fd);
}

void Timeline::Update()
{
	BOOST_LOG_TRIVIAL(info) << "Extracting new accuracy/resolution parameters";
	struct qot_message msg;
	if (ioctl(this->fd, QOT_GET_INFORMATION, &msg))
	{
		BOOST_LOG_TRIVIAL(error) << "Could not extract information from timeline";
		return;
	}
}

void Timeline::MonitorThread()
{
	// Wait until the main thread sets up the binding and posix clock
    while (this->fd < 0) 
    	this->cv.wait(this->lk);

    BOOST_LOG_TRIVIAL(info) << "Polling for activity";

    // Start polling
    while (!this->kill)
    {
    	// Initialize the polling struct
		struct pollfd pfd[1];
		memset(pfd,0,sizeof(pfd));
		pfd[0].fd = this->fd;
		pfd[0].events = QOT_ACTION_TIMELINE;

		// Wait until an asynchronous data push from the kernel module
		if (poll(pfd,1, QOT_POLL_TIMEOUT_MS))
		{
			BOOST_LOG_TRIVIAL(info) << "Updated accuracy/resolution parameters available";
			if (pfd[0].revents & QOT_ACTION_TIMELINE)
				this->Update();
		}
	}
}
