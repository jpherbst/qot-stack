/**
 * @file Syncrhonization.hpp
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
 *
 * The code in this file is an adaptation of ptp4l  
 *
 */

#ifndef SYNCHRONIZATION_HPP
#define SYNCHRONIZATION_HPP

// Boost includes
#include <boost/asio.hpp>
#include <boost/thread.hpp> 

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

/*
struct config settings = {
		
	.interfaces = STAILQ_HEAD_INITIALIZER(settings.interfaces),

	.dds = {
		.dds = {
			.flags = DDS_TWO_STEP_FLAG,
			.priority1 = 128,
			.clockQuality.clockClass = 248,
			.clockQuality.clockAccuracy = 0xfe,
			.clockQuality.offsetScaledLogVariance = 0xffff,
			.priority2 = 128,
			.domainNumber = 0,
		},
		.free_running = 0,
		.freq_est_interval = 1,
		.grand_master_capable = 1,
		.stats_interval = 0,
		.kernel_leap = 1,
		.sanity_freq_limit = 200000000,
		.time_source = INTERNAL_OSCILLATOR,
		.clock_desc = {
			.productDescription = {
				.max_symbols = 64,
				.text = ";;",
				.length = 2,
			},
			.revisionData = {
				.max_symbols = 32,
				.text = ";;",
				.length = 2,
			},
			.userDescription      = { .max_symbols = 128 },
			.manufacturerIdentity = { 0, 0, 0 },
		},
		.delay_filter = FILTER_MOVING_MEDIAN,
		.delay_filter_length = 10,
		.boundary_clock_jbod = 0,
	},

	.pod = {
		.logAnnounceInterval = 1,
		.logSyncInterval = 0,
		.logMinDelayReqInterval = 0,
		.logMinPdelayReqInterval = 0,
		.announceReceiptTimeout = 3,
		.syncReceiptTimeout = 0,
		.transportSpecific = 0,
		.announce_span = 1,
		.path_trace_enabled = 0,
		.follow_up_info = 0,
		.freq_est_interval = 1,
		.neighborPropDelayThresh = 20000000,
		.min_neighbor_prop_delay = -20000000,
		.tx_timestamp_offset = 0,
		.rx_timestamp_offset = 0,
	},

	.timestamping = TS_HARDWARE,
	.dm = DM_E2E,
	.transport = TRANS_UDP_IPV4,

	.assume_two_step = &assume_two_step,
	.tx_timestamp_timeout = &sk_tx_timeout,
	.check_fup_sync = &sk_check_fupsync,

	.clock_servo = CLOCK_SERVO_PI,

	.step_threshold = &servo_step_threshold,
	.first_step_threshold = &servo_first_step_threshold,
	.max_frequency = &servo_max_frequency,

	.pi_proportional_const = &configured_pi_kp,
	.pi_integral_const = &configured_pi_ki,
	.pi_proportional_scale = &configured_pi_kp_scale,
	.pi_proportional_exponent = &configured_pi_kp_exponent,
	.pi_proportional_norm_max = &configured_pi_kp_norm_max,
	.pi_integral_scale = &configured_pi_ki_scale,
	.pi_integral_exponent = &configured_pi_ki_exponent,
	.pi_integral_norm_max = &configured_pi_ki_norm_max,
	.ntpshm_segment = &ntpshm_segment,

	.ptp_dst_mac = ptp_dst_mac,
	.p2p_dst_mac = p2p_dst_mac,
	.udp6_scope = &udp6_scope,
	.uds_address = uds_path,

	.print_level = LOG_INFO,
	.use_syslog = 1,
	.verbose = 0,

	.cfg_ignore = 0,
};
*/

namespace qot
{
	class Synchronization
	{

	// Constructor and destructor
	public: Synchronization(boost::asio::io_service *io);
	public: ~Synchronization();

	// Private variables
	private: boost::asio::io_service *asio;
	
	// PTP settings
	//private: struct config settings;

	};
}

#endif