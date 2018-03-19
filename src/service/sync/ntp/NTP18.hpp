/**
 * @file NTP18.hpp
 * @brief Provides header for ntp instance based on Chrony to the sync interface
 * @author Sandeep D'souza
 * 
 * Copyright (c) Carnegie Mellon University, 2018. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *      1. Redistributions of source code must retain the above copyright notice, 
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, 
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND f
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
 * Reference: Based on the chrony implementation of NTP
 * 1. chrony: https://chrony.tuxfamily.org/
 */

#ifndef NTP18_HPP
#define NTP18_HPP

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

	// chrony includes
	#include "chrony-3.2/config.h"
	#include "chrony-3.2/sysincl.h"
	#include "chrony-3.2/main.h"
	#include "chrony-3.2/sched.h"
	#include "chrony-3.2/local.h"
	#include "chrony-3.2/sys.h"
	#include "chrony-3.2/ntp_io.h"
	#include "chrony-3.2/ntp_signd.h"
	#include "chrony-3.2/ntp_sources.h"
	#include "chrony-3.2/ntp_core.h"
	#include "chrony-3.2/sources.h"
	#include "chrony-3.2/sourcestats.h"
	#include "chrony-3.2/reference.h"
	#include "chrony-3.2/logging.h"
	#include "chrony-3.2/conf.h"
	#include "chrony-3.2/cmdmon.h"
	#include "chrony-3.2/keys.h"
	#include "chrony-3.2/manual.h"
	#include "chrony-3.2/rtc.h"
	#include "chrony-3.2/refclock.h"
	#include "chrony-3.2/clientlog.h"
	#include "chrony-3.2/nameserv.h"
	#include "chrony-3.2/privops.h"
	#include "chrony-3.2/smooth.h"
	#include "chrony-3.2/tempcomp.h"
	#include "chrony-3.2/util.h"

	// Include to share data from ntp sync to uncertainty calculation
	#include "uncertainty_data.h"
}

namespace qot
{
	class NTP18 : public Sync
	{

		// Constructor and destructor
		public: NTP18(boost::asio::io_service *io, const std::string &iface, struct uncertainty_params config);
		public: ~NTP18();

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
