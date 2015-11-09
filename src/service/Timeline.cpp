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

extern "C"
{
	#include <stdio.h>
	#include <sys/ioctl.h>
}

/* Trivial logging */
#include <boost/log/trivial.hpp>

using namespace qot;

Timeline::Timeline(boost::asio::io_service *io, const std::string &file)
	: asio(io), coordinator(io, "timeline")
{
	// Try and open the file
	fd = open(file.c_str(), O_RDWR);
	if (fd < 0)
	{
		BOOST_LOG_TRIVIAL(error) << "Could not open the timeline " << file;
		return;
	}	
}

Timeline::~Timeline()
{
	if (fd > 0)
		close(fd);
}