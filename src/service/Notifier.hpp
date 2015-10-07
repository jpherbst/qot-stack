/**
 * @file Notifier.hpp
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
#ifndef NOTIFIER_HPP
#define NOTIFIER_HPP

// System includes
#include <string>
#include <map>

/* System dependencies */
extern "C"
{
	#include <stdio.h>
	#include <stdlib.h>
	#include <errno.h>
	#include <poll.h>
	#include <dirent.h>
	#include <sys/types.h>
	#include <sys/inotify.h>
}

// Boost includes
#include <boost/asio.hpp>
#include <boost/thread.hpp> 

// Project includes
#include "Timeline.hpp"

/* Convenience declarations */
#define EVENT_SIZE  	(sizeof(struct inotify_event))
#define EVENT_BUF_LEN   (1024*(EVENT_SIZE + 16))
#define EVENT_MAXLEN 	(512)
#define	EVENT_TIMEOUT	(1000)

namespace qot
{
	class Notifier
	{

	// Constructor and destructor
	public: Notifier(boost::asio::io_service *io, const std::string &dir);
	public: ~Notifier();

	// Private methods
	private: void add(const std::string &path);
	private: void del(const std::string &path);
	private: void watch(const char* dir);

	// Private variables
	private: boost::asio::io_service *asio;
	private: boost::thread thread;
	private: std::map<std::string,Timeline> timelines;
	};
}

#endif