/**
 * @file PTP18.cpp
 * @brief Provides ptp instance to the sync interface (based on linuxptp-1.8)
 * @author Sandeep D'souza
 * 
 * Copyright (c) Regents of the Carnegie Mellon University, 2018. All rights reserved.
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

#include "PTP18.hpp"

using namespace qot;

#define DEBUG true
#define TEST  true

int assume_two_step = 0;

PTP18::PTP18(boost::asio::io_service *io, // ASIO handle
		const std::string &iface,		  // interface
		struct uncertainty_params config  // uncertainty calculation configuration
	) : baseiface(iface), sync_uncertainty(config), cfg(NULL)
{	
	this->Reset();	
}

PTP18::~PTP18()
{
	this->Stop();
}

void PTP18::Reset() 
{
	if (cfg)
		config_destroy(cfg);

	// Initialize Default Config
	cfg = config_create();
}

void PTP18::Start(bool master, int log_sync_interval, uint32_t sync_session,
	int timelineid, int *timelinesfd, uint16_t timelines_size)
{
	// First stop any sync that is currently underway
	this->Stop();

	// Initialize Default Config
	cfg = config_create();

	// Restart sync
	BOOST_LOG_TRIVIAL(info) << "Starting PTP synchronization as " << (master ? "master" : "slave") 
		<< " on domain " << sync_session << " with synchronization interval " << log_sync_interval;
	
	config_set_int(cfg, "logSyncInterval", 0);
	config_set_int(cfg, "domainNumber", sync_session); // Should (can also for multi-timeline?) be set to sync session

	if (master){
		config_set_int(cfg, "slaveOnly", 0);
	}

	// Force to be a slave -> prev lines commented by Sandeep (comment prev lines out to force slave mode)
	// if (config_set_int(cfg, "slaveOnly", 1)) {
	// 			goto out;

	kill = false;

	// Initialize Local Tracking Variable for Clock-Skew Statistics (Checks Staleness)
	last_clocksync_data_point.offset  = 0;
	last_clocksync_data_point.drift   = 0;
	last_clocksync_data_point.data_id = 0;

	// Initialize Global Variable for Clock-Skew Statistics 
	ptp_clocksync_data_point[timelineid].offset  = 0;
	ptp_clocksync_data_point[timelineid].drift   = 0;
	ptp_clocksync_data_point[timelineid].data_id = 0;

	thread = boost::thread(boost::bind(&PTP18::SyncThread, this, timelineid, timelinesfd, timelines_size));
}

void PTP18::Stop()
{
	BOOST_LOG_TRIVIAL(info) << "Stopping PTP synchronization ";
	kill = true;
	thread.join();
}


int PTP18::SyncThread(int timelineid, int *timelinesfd, uint16_t timelines_size)
{
	BOOST_LOG_TRIVIAL(info) << "PTP (linuxptp-1.8) Sync thread started ";
	char *config = NULL, *req_phc = NULL;
	int c, err = -1, print_level;
	struct clock *clock = NULL;
	int required_modes = 0;

	if (config_set_int(cfg, "delay_mechanism", DM_AUTO))
	{
		config_destroy(cfg);
		return err;
	}

	enum transport_type trans_type = TRANS_IEEE_802_3; // Can also be TRANS_UDP_IPV4 or TRANS_UDP_IPV6
	if (config_set_int(cfg, "network_transport", trans_type))
	{
		config_destroy(cfg);
		return err;
	}

	enum timestamp_type ts_type = TS_HARDWARE; // Can also be TS_SOFTWARE, TS_LEGACY_HW
	if (config_set_int(cfg, "time_stamping", ts_type))
	{
		config_destroy(cfg);
		return err;
	}

	// Add the interface
	char ifname[MAX_IFNAME_SIZE];
	strncpy(ifname, baseiface.c_str(), MAX_IFNAME_SIZE);

	if (!config_create_interface(ifname, cfg))
	{
		config_destroy(cfg);
		return err;
	}

	// Verbose printing for debug
	if (DEBUG)
		config_set_int(cfg, "verbose", 1);

	// Read config from a file -> can be removed later
	if (config && (c = config_read(config, cfg))) {
		return c;
	}

	print_set_verbose(config_get_int(cfg, NULL, "verbose"));
	print_set_syslog(config_get_int(cfg, NULL, "use_syslog"));
	print_set_level(config_get_int(cfg, NULL, "logging_level"));

	assume_two_step = config_get_int(cfg, NULL, "assume_two_step");
	sk_check_fupsync = config_get_int(cfg, NULL, "check_fup_sync");
	sk_tx_timeout = config_get_int(cfg, NULL, "tx_timestamp_timeout");

	if (config_get_int(cfg, NULL, "clock_servo") == CLOCK_SERVO_NTPSHM) {
		config_set_int(cfg, "kernel_leap", 0);
		config_set_int(cfg, "sanity_freq_limit", 0);
	}

	if (STAILQ_EMPTY(&cfg->interfaces)) {
		fprintf(stderr, "no interface specified\n");
		config_destroy(cfg);
		return err;
	}

	clock = clock_create(cfg->n_interfaces > 1 ? CLOCK_TYPE_BOUNDARY :
			     CLOCK_TYPE_ORDINARY, cfg, req_phc, timelineid, timelinesfd);
	if (!clock) {
		fprintf(stderr, "failed to create a clock\n");
		goto out;
	}

	err = 0;

	while (is_running() && !kill) {
		if (clock_poll(clock))
			break;

		// Check if a new skew statistic data point has been added
		if(last_clocksync_data_point.data_id < ptp_clocksync_data_point[timelineid].data_id)
		{
			// New statistic received -> Replace old value
			last_clocksync_data_point = ptp_clocksync_data_point[timelineid];

			// Add Synchronization Uncertainty Sample
			sync_uncertainty.CalculateBounds(last_clocksync_data_point.offset, ((double)last_clocksync_data_point.drift)/1000000000LL, timelinesfd[0]);
		}
	}
out:
	if (clock)
		clock_destroy(clock);
	config_destroy(cfg);
	return err;
}
