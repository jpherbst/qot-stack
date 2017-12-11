/**
 * @file NTP.hpp
 * @brief Provides ntp instance to the sync interface
 * @author Fatima Anwar
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

#ifndef NTP_HPP
#define NTP_HPP

// Boost includes
#include <boost/asio.hpp>
#include <boost/thread.hpp> 
#include <boost/log/trivial.hpp>

#include "../Sync.hpp"
#include "../SyncUncertainty.hpp"

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

	// ntpv4 includes
	#include "ntpv4/ntpclient.h"

	// Include to share data from ntp sync to uncertainty calculation
	#include "uncertainty_data.h"
}

namespace qot
{
	class NTP : public Sync
	{

		// Constructor and destructor
		public: NTP(boost::asio::io_service *io, const std::string &iface, struct uncertainty_params config);
		public: ~NTP();

		// Control functions
		public: void Reset();
		public: void Start(bool master, int log_sync_interval, uint32_t sync_session, int timelineid, int *timelinesfd, uint16_t timelines_size);
		public: void Stop();

		// This thread performs rhe actual syncrhonization
		private: int SyncThread(int *timelinesfd, uint16_t timelines_size);

		// Boost ASIO
		private: boost::asio::io_service *asio;
		private: boost::thread thread;
		private: std::string baseiface;
		private: bool kill;

		// NTP settings
		private: NtpConfig NtpCfg; // configuration variable
		private: Response resp_list[NSTAGE];
		private: int total;
		private: int cursor = 0;

		// Sync Uncertainty Calculation Class
		private: SyncUncertainty sync_uncertainty;

		// Last Received Clock-Sync Skew Statistic Data Point
		private: qot_stat_t last_clocksync_data_point;

	};
}

#endif
