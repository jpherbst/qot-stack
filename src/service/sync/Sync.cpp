/**
 * @file Sync.cpp
 * @brief Factory class prepares a synchronization session
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
 *
 */

#include "Sync.hpp"

// Subtypes
#include "ptp/PTP.hpp"

using namespace qot;

// All sync algorithms get parameter updates from the qot module
Sync::Sync() : flag(true)
{
	// Open an IOCTL handle
	/*const char *file_name = "/dev/timeline";
	fd = open(file_name, O_RDWR);
	if (fd == -1)
		std::cerr << "Sync::ERROR IOCTL open() problem" << std::endl;
	*/
	// Protocol to listen for time sync updates
	//thread = boost::thread(boost::bind(&Sync::worker,this));
}

Sync::~Sync()
{
	// Stop and join thread
	this->flag = false;
	//thread.join();

	// Close ioctl
	//close(fd);
}

// Worker thread to poll for new parameters
void Sync::worker()
{}


// Factory method
boost::shared_ptr<Sync> Sync::Factory(
			boost::asio::io_service *io,		// IO service
			const std::string &address,			// IP address of the node
			const std::string &iface)				// interface for synchronization
{
	// Instantiate a new sync algorithm
/*	switch(iface)
	{
		case ETH: 	return boost::shared_ptr<Sync>((Sync*) new PTP(io,iface));
	}
*/	return boost::shared_ptr<Sync>((Sync*) new PTP(io,iface));
}
