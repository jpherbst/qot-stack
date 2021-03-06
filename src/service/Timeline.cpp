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

#include <string>

/* So that we might expose a meaningful name through PTP interface */
#define QOT_IOCTL_BASE          "/dev"
#define QOT_IOCTL_TIMELINE      "timeline"
#define QOT_IOCTL_FORMAT        "%8s%d"
#define QOT_MAX_NAMELEN         64

using namespace qot;

Timeline::Timeline(boost::asio::io_service *io, const std::string &name,
	const std::string &iface, const std::string &addr, int id)
	: id_(id), kill(false), coordinator(io, name, iface, addr)
{
	/* First, save the id to the message data structure. Having this present
	   in the data structure will cause us to bind without affecting metrics
	   Second, bind to the timeline to get the base requirements */
	
	std::string tml_filename = "";
	tml_filename = tml_filename + QOT_IOCTL_BASE + "/" + QOT_IOCTL_TIMELINE + std::to_string(id_);
	BOOST_LOG_TRIVIAL(info) << "opening timeline chdev: " << tml_filename;
	this->fd = open(tml_filename.c_str(), O_RDWR);
	if (fd < 0)
	{
		BOOST_LOG_TRIVIAL(error) << "Could not open ioctl to " << tml_filename;
		return;
	}
	if (ioctl(fd, TIMELINE_GET_INFO, &msgt))
	{
		BOOST_LOG_TRIVIAL(warning) << "Timeline " << id_ << " have no info.";
		return;
	}
	if (ioctl(fd, TIMELINE_GET_BINDING_INFO, &msgb))
	{
		BOOST_LOG_TRIVIAL(warning) << "Timeline " << id_ << " have binding problems";
		return;
	}
	BOOST_LOG_TRIVIAL(info) << "Timeline information received successfully";

	// Initialize the coordinator with the given PHC id, UUID, accuracy and resolution
	coordinator.Start(id_, this->fd, msgt.name, msgb.demand.accuracy, msgb.demand.resolution);

	// We can now start polling, because the timeline is setup
	//thread = boost::thread(boost::bind(&Timeline::MonitorThread, this));
}

Timeline::~Timeline()
{
	// Initialize the coordinator
	coordinator.Stop();

	// Kill the thread
	this->kill = true;
	//this->thread.join();

	// Close ioctl
	if (fd > 0)
		close(fd);
}

void Timeline::MonitorThread()
{
	// Wait until the main thread sets up the binding and posix clock
	BOOST_LOG_TRIVIAL(info) << "Polling for activity";

	// Start polling
	//while (!this->kill)
	// {

	// Initialize the polling struct
		/*struct pollfd pfd[1];
		memset(pfd,0,sizeof(pfd));
		pfd[0].fd = this->fd;
		pfd[0].events = POLLIN;*/

		//TODO: change according to new core
		// Wait until an asynchronous data push from the kernel module
		/*if (poll(pfd,1, QOT_POLL_TIMEOUT_MS) && !kill)
		{
			// Some event just occured on the timeline
			if (pfd[0].revents & POLLIN)
			{
				BOOST_LOG_TRIVIAL(info) << "Timeline Event...";
				
				while (ioctl(this->fd, QOT_GET_EVENT, &msg) == 0)
				{
					// Special callback for capture events to
					//TODO: change according to new core
					if (msg.event.type == EVENT_UPDATE)
					{
						BOOST_LOG_TRIVIAL(info) << "Timeline metrics updated...";
						//TODO: change according to new core
						if (ioctl(this->fd, QOT_GET_TARGET, &msg) == 0)
							coordinator.Update(msg.request.acc, msg.request.res);
					}
				}
			}
		}
		*/
	//}
}
