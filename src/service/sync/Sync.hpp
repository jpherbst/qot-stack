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

// System includes
#include <cstdint>

// Required for logging and shared pointers
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/log/trivial.hpp>

// System dependencies
extern "C"
{
	#include <string.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <dirent.h>

	#include "../../qot_types.h"
}

namespace qot
{
	// Algorithm for synchronization
	enum SyncType {
		SYNC_NTP,
		SYNC_PTP,
		SYNC_PULSESYNC,
		SYNC_FTSP
	};

	// Interface type
	enum SyncInterface {
		ETH,
		WLAN,
		WPAN
	};

	// Mode of synchronization
	enum SyncMode
	{
		MASTER,
		SLAVE
	};

	// Base functionality
	class Sync {
		// Reset the synchronization algorithm
		public: virtual void Reset() = 0;

		// Start synchronization
		public: virtual void Start(bool master,
			int log_sync_interval,
			uint32_t sync_session,
			int timelineid,
			int *timelinesfd,
			uint16_t timelines_size) = 0;

		// Stop synchronization
		public: virtual void Stop() = 0;

		// Factory method to produce a handle to a sync algorithm
		public: static boost::shared_ptr<Sync> Factory(
			boost::asio::io_service *io,		// IO service
			const std::string &address,		// address of the Master
			const std::string &iface		// interface for synchronization
		);

		// Helper functions to check if the IP address is in private / public subnet
		private: static bool IsIPprivate(const std::string ip);
		private: static uint32_t IPtoUint(const std::string ip);
	};
}

#endif
