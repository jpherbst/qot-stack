/**
 * @file Timeline.hpp
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
#ifndef TIMELINE_HPP
#define TIMELINE_HPP

// std includes
#include <thread>
#include <mutex>
#include <condition_variable>

// Boost includes
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/log/trivial.hpp>

// Network coordination
#include "Coordinator.hpp"

// Include the QOT api
extern "C"
{
	#include "../qot_types.h"
}

namespace qot
{
	class Timeline
	{
	// Constructor and destructor
	public: Timeline(boost::asio::io_service *io, const std::string &name,
		const std::string &iface, const std::string &addr, int id);
	public: ~Timeline();

	// Heartbeat timer
	private: void MonitorThread(); // Polling thread for params
	private: void Heartbeat(const boost::system::error_code& /*e*/);

	// Asynchronous service
	private: boost::asio::io_service *asio;
	private: boost::thread thread; // Worker thread for capture

	// Communication with local timeline
	private: int id_; // 'id' of timeline as in /dev/timeline[id_]
	private: int fd; // IOCTL link

	private: qot_binding_t msgb;  // Information about the timeline
	private: qot_timeline_t msgt; // Information about the timeline

	private: bool kill; // Kill flag
	private: std::string basename; // Name of service

	// Network (DDS) coordinator
	private: Coordinator coordinator;

	};
}

#endif
