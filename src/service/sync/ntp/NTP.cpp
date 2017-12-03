/**
 * @file NTP.cpp
 * @brief Provides ntp instance to the sync interface
 * @author Fatima Anwar, Zhou Fang
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
 * Reference: 
 * 1. NTPv4: https://www.eecis.udel.edu/~mills/database/reports/ntp4/ntp4.pdf
 * 2. Use some source code from: http://blog.csdn.net/rich_baba/article/details/6052863
 *
 */

#include "NTP.hpp"

using namespace qot;

NTP::NTP(boost::asio::io_service *io, // ASIO handle
	const std::string &iface,     // interface	
	struct uncertainty_params config // uncertainty calculation configuration		
	) : asio(io), baseiface(iface), sync_uncertainty(config)
{	
	this->Reset();
}

NTP::~NTP()
{
	this->Stop();
}

void NTP::Reset()
{
	load_default_cfg(&NtpCfg); // load configuration file
	reset_clock();
}

void NTP::Start(
	bool master,
	int log_sync_interval,
	uint32_t sync_session,
	int timelineid,
	int *timelinesfd,
	uint16_t timelines_size)
{
	// First stop any sync that is currently underway
	this->Stop();

	// Restart sync
	BOOST_LOG_TRIVIAL(info) << "Starting NTP synchronization";
	kill = false;

	// Initialize Local Tracking Variable for Clock-Skew Statistics (Checks Staleness)
	last_clocksync_data_point.offset  = 0;
	last_clocksync_data_point.drift   = 0;
	last_clocksync_data_point.data_id = 0;

	// Initialize Global Variable for Clock-Skew Statistics 
	ntp_clocksync_data_point.offset  = 0;
	ntp_clocksync_data_point.drift   = 0;
	ntp_clocksync_data_point.data_id = 0;

	thread = boost::thread(boost::bind(&NTP::SyncThread, this, timelinesfd, timelines_size));
}

void NTP::Stop()
{
	BOOST_LOG_TRIVIAL(info) << "Stopping NTP synchronization ";
	kill = true;
	thread.join();
}


int NTP::SyncThread(int *timelinesfd, uint16_t timelines_size)
{
	BOOST_LOG_TRIVIAL(info) << "Sync thread started";

	int sock[NR_REMOTE];
	int ret;
	Response resp;
	int addr_len;

	// connect to ntp server
	int i = 0, j = 0;

	for (j = 0; j < NR_REMOTE; j++) {
		sock[j] = ntp_conn_server(NtpCfg.servaddr[j], NtpCfg.port); // connect to ntp server
	}

	//#ifdef DEBUG
	printf("\n");
	//#endif
	
	while (true) {
		i++;
		total = NR_REMOTE;
		// poll all servers
		for (j = 0; j < NR_REMOTE; j++) {
			send_packet(sock[j], timelinesfd[0]); // send ntp package
			if (get_server_time(sock[j], &resp, timelinesfd[0]) == false) {
				total --;
				if (total < MIN_NTP_SAMPLE) {
					printf("Quit this NTP sync\n");
					break; // quit this sync
				}
			} else {
				// process here:
				resp_list[cursor++] = resp;
			}

			//#ifdef DEBUG
			printf("cursor: %d, total: %d\n\n", cursor, total);
			//#endif

			if (cursor == total) {
				cursor = 0;
				ntp_process(timelinesfd[0], resp_list, total); // process responses
				// Check if a new skew statistic data point has been added
				if(last_clocksync_data_point.data_id < ntp_clocksync_data_point.data_id)
				{
					// New statistic received -> Replace old value
					last_clocksync_data_point = ntp_clocksync_data_point;

					std::cout << "Estimated Drift = " << last_clocksync_data_point.drift << " " << ((double)last_clocksync_data_point.drift)/1000000000LL
					          << " offset = " << last_clocksync_data_point.offset 
					          << " data_id =" << last_clocksync_data_point.data_id << "\n";

					// Add Synchronization Uncertainty Sample
					sync_uncertainty.CalculateBounds(last_clocksync_data_point.offset, ((double)last_clocksync_data_point.drift)/1000000000LL, timelinesfd[0]);
				}
			}
		}

		sleep(NtpCfg.psec); // ntp check interval
	}

	for (j = 0; j < NR_REMOTE; j++) {
		close(sock[j]);
	}
}
