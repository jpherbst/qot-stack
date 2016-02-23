/**
 * @file NTP.cpp
 * @brief Provides ptp instance to the sync interface
 * @author Fatima Anwar
 * 
 * Copyright (c) Regents of the University of California, 2015. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *      1. Redistributions of source code must retain the above copyright notice, 
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
 *
 * The code in this file is an adaptation of ptp4l  
 *
 */

#include "NTP.hpp"

using namespace qot;

#define DEBUG true
#define TEST  true

NTP::NTP(boost::asio::io_service *io,		// ASIO handle
		const std::string &iface)			// interface			
	: asio(io), baseiface(iface)
{	
	this->Reset();	
}

NTP::~NTP()
{
	this->Stop();
}

void NTP::Reset() 
{}

void NTP::Start(bool master, int log_sync_interval, uint32_t sync_session, int *timelinesfd, uint16_t timelines_size)
{
	// First stop any sync that is currently underway
	this->Stop();

	// Restart sync
	BOOST_LOG_TRIVIAL(info) << "Starting NTP synchronization";
	kill = false;

	// TODO: How to see if /dev/ptp0 is always the ethernet controller clock?
	int phc_index = 0;

	thread = boost::thread(boost::bind(&NTP::SyncThread, this, phc_index, timelinesfd, timelines_size));
}

void NTP::Stop()
{
	BOOST_LOG_TRIVIAL(info) << "Stopping NTP synchronization ";
	kill = true;
	thread.join();
}


int NTP::SyncThread(int phc_index, int *timelinesfd, uint16_t timelines_size)
{
	BOOST_LOG_TRIVIAL(info) << "Sync thread started ";
}
