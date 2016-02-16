/**
 * @file Sync.hpp
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

#ifndef SYNC_H
#define SYNC_H

// Standard integer types
#include <cstdint>

// Required for logging and shared pointers
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

namespace qot
{
        // Filter type
        enum SyncType
        {
        	SYNC_NTP,
        	SYNC_PTP,
			SYNC_PULSESYNC,
			SYNC_FTSP
        };

		// Interface type
        enum SyncInterface
        {
        	ETH,
        	WLAN,
			WPAN
        };

        enum SyncMode
        {
        	MASTER,
        	SLAVE
        };

        // Base functionality
        class Sync
        {
        	// Constructor and destructor
        	public: Sync();
        	public: ~Sync();

        	// Used by all sync algorithms
        	private: int fd;							// ioctl file descriptor
        	private: bool flag;							// Thread termination flag
        	private: boost::thread thread;				// Thread handle
        	private: void worker();						// Thread worker
		
			// Factory method to produce a handle to a sync algorithm
			public: static boost::shared_ptr<Sync> Factory(
				boost::asio::io_service *io,		// IO service
				const std::string &address,			// address of the Master
				const std::string &iface					// interface for synchronization
			);
        };
}

#endif
