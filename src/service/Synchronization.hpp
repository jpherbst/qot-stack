/**
 * @file Synchronization.hpp
 * @brief C++ interface to a version of linuxptp that supports multiple timelines
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
#ifndef SYNCHRONIZATION_HPP
#define SYNCHRONIZATION_HPP

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

namespace qot
{
	class Synchronization
	{

	// Constructor and destructor
	public: Synchronization(boost::asio::io_service *io, const std::string &dir);
	public: ~Synchronization();
	
	// Private variables
	private: boost::asio::io_service *asio;
	
	};
}

#endif