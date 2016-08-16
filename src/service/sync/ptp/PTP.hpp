/**
 * @file PTP.hpp
 * @brief Provides ptp instance to the sync interface
 * @author Andrew Symingon, Fatima Anwar
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
 *
 *
 */

#ifndef PTP_HPP
#define PTP_HPP

// Parent class include
#include "../Sync.hpp"

// Boost includes
#include <boost/asio.hpp>
#include <boost/thread.hpp> 
#include <boost/log/trivial.hpp>

/* Linuxptp includes */
extern "C"
{
	// Standard includes
	#include <limits.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <unistd.h>
	#include <linux/net_tstamp.h>

	// Linuxptp includes
	#include "linuxptp/clock.h"
	#include "linuxptp/config.h"
	#include "linuxptp/ntpshm.h"
	#include "linuxptp/pi.h"
	#include "linuxptp/print.h"
	#include "linuxptp/raw.h"
	#include "linuxptp/sk.h"
	#include "linuxptp/transport.h"
	#include "linuxptp/udp6.h"
	#include "linuxptp/uds.h"
	#include "linuxptp/util.h"
	#include "linuxptp/version.h"
}

namespace qot
{
	class PTP  : public Sync
	{

		// Constructor and destructor
		public: PTP(boost::asio::io_service *io, const std::string &iface);
		public: ~PTP();

		// Control functions
		public: void Reset();
		public: void Start(bool master, int log_sync_interval,
			uint32_t sync_session, int timelineid, int *timelinesfd, uint16_t timelines_size);
		public: void Stop();						// Stop

		// This thread performs rhe actual syncrhonization
		private: int SyncThread(int timelineid, int *timelinesfd, uint16_t timelines_size);

		// Boost ASIO
		private: boost::asio::io_service *asio;
		private: boost::thread thread;
		private: std::string baseiface;
		private: bool kill;

		// PTP settings
		private: struct config cfg_settings;

	};
}

#endif
