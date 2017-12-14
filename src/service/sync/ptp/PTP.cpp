/**
 * @file PTP.cpp
 * @brief Provides ptp instance to the sync interface
 * @author Andrew Symingon, Fatima Anwar, Sandeep D'souza
 * 
 * Copyright (c) Regents of the University of California, 2015. All rights reserved.
 * Copyright (c) Regents of the Carnegie Mellon University, 2017. All rights reserved.
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

#include "PTP.hpp"

using namespace qot;

#define DEBUG true
#define TEST  true

PTP::PTP(boost::asio::io_service *io,	 // ASIO handle
		const std::string &iface,		 // interface
		struct uncertainty_params config // uncertainty calculation configuration
	) : /*asio(io),*/ baseiface(iface), sync_uncertainty(config)
{	
	this->Reset();	
}

PTP::~PTP()
{
	this->Stop();
}

void PTP::Reset() 
{
	// Default settings...
	cfg_settings.interfaces = STAILQ_HEAD_INITIALIZER(cfg_settings.interfaces);
	cfg_settings.dds.dds.flags = DDS_TWO_STEP_FLAG;
	cfg_settings.dds.dds.priority1 = 128;
	cfg_settings.dds.dds.clockQuality.clockClass = 100; // had been changed by Sandeep from 100
	cfg_settings.dds.dds.clockQuality.clockAccuracy = 0xfe;
	cfg_settings.dds.dds.clockQuality.offsetScaledLogVariance = 0xffff;
	cfg_settings.dds.dds.priority2 = 128;
	cfg_settings.dds.dds.domainNumber = 0;
	cfg_settings.dds.free_running = 0;
	cfg_settings.dds.freq_est_interval = 1;
	cfg_settings.dds.grand_master_capable = 1;
	cfg_settings.dds.stats_interval = 0;
	cfg_settings.dds.kernel_leap = 1;
	cfg_settings.dds.sanity_freq_limit = 200000000;
	cfg_settings.dds.time_source = INTERNAL_OSCILLATOR;
	cfg_settings.dds.clock_desc.productDescription.max_symbols = 64;
	strcpy((char *)cfg_settings.dds.clock_desc.productDescription.text,";;");
	cfg_settings.dds.clock_desc.productDescription.length = 2;
	cfg_settings.dds.clock_desc.revisionData.max_symbols = 32;
	strcpy((char *)cfg_settings.dds.clock_desc.revisionData.text,";;");
	cfg_settings.dds.clock_desc.revisionData.length = 2;
	cfg_settings.dds.clock_desc.userDescription.max_symbols = 128;
	cfg_settings.dds.clock_desc.manufacturerIdentity[0] = 0;
	cfg_settings.dds.clock_desc.manufacturerIdentity[1] = 0;
	cfg_settings.dds.clock_desc.manufacturerIdentity[2] = 0;
	cfg_settings.dds.delay_filter = FILTER_MOVING_MEDIAN;
	cfg_settings.dds.delay_filter_length = 10;
	cfg_settings.dds.boundary_clock_jbod = 0;
	cfg_settings.pod.logAnnounceInterval = 1;
	cfg_settings.pod.logSyncInterval = 0;
	cfg_settings.pod.logMinDelayReqInterval = 0;
	cfg_settings.pod.logMinPdelayReqInterval = 0;
	cfg_settings.pod.announceReceiptTimeout = 3;
	cfg_settings.pod.syncReceiptTimeout = 0;
	cfg_settings.pod.transportSpecific = 0;
	cfg_settings.pod.announce_span = 1;
	cfg_settings.pod.path_trace_enabled = 0;
	cfg_settings.pod.follow_up_info = 0;
	cfg_settings.pod.freq_est_interval = 1;
	cfg_settings.pod.neighborPropDelayThresh = 20000000;
	cfg_settings.pod.min_neighbor_prop_delay = -20000000;
	cfg_settings.pod.tx_timestamp_offset = 0;
	cfg_settings.pod.rx_timestamp_offset = 0;
	cfg_settings.timestamping = TS_HARDWARE;
	cfg_settings.dm = DM_E2E; //Changed by sandeep from e2e
	cfg_settings.transport = TRANS_IEEE_802_3;
	cfg_settings.assume_two_step = &assume_two_step;
	cfg_settings.tx_timestamp_timeout = &sk_tx_timeout;
	cfg_settings.check_fup_sync = &sk_check_fupsync;
	cfg_settings.clock_servo = CLOCK_SERVO_PI;//changed by Sandeep from CLOCK_SERVO_LINREGNEW, moving from feedforward to feedback; 
	cfg_settings.step_threshold = &servo_step_threshold;
	cfg_settings.first_step_threshold = &servo_first_step_threshold;
	cfg_settings.max_frequency = &servo_max_frequency;
	cfg_settings.pi_proportional_const = &configured_pi_kp;
	cfg_settings.pi_integral_const = &configured_pi_ki;
	cfg_settings.pi_proportional_scale = &configured_pi_kp_scale;
	cfg_settings.pi_proportional_exponent = &configured_pi_kp_exponent;
	cfg_settings.pi_proportional_norm_max = &configured_pi_kp_norm_max;
	cfg_settings.pi_integral_scale = &configured_pi_ki_scale;
	cfg_settings.pi_integral_exponent = &configured_pi_ki_exponent;
	cfg_settings.pi_integral_norm_max = &configured_pi_ki_norm_max;
	cfg_settings.ntpshm_segment = &ntpshm_segment;
	cfg_settings.ptp_dst_mac = ptp_dst_mac;
	cfg_settings.p2p_dst_mac = p2p_dst_mac;
	cfg_settings.udp6_scope = &udp6_scope;
	cfg_settings.uds_address = uds_path;
	cfg_settings.print_level = LOG_INFO;
	cfg_settings.use_syslog = 1;
	cfg_settings.verbose = 1;
	cfg_settings.cfg_ignore = 0;
}

void PTP::Start(bool master, int log_sync_interval, uint32_t sync_session,
	int timelineid, int *timelinesfd, uint16_t timelines_size)
{
	// First stop any sync that is currently underway
	this->Stop();

	// Restart sync
	BOOST_LOG_TRIVIAL(info) << "Starting PTP synchronization as " << (master ? "master" : "slave") 
		<< " on domain " << sync_session << " with synchronization interval " << log_sync_interval;
	//cfg_settings.dds.dds.domainNumber = sync_session;	
	cfg_settings.dds.dds.domainNumber = 0;
	cfg_settings.pod.laterlogSyncInterval = 0;//log_sync_interval; // Change this when you need to slow down sync later
	cfg_settings.pod.logSyncInterval = 0;//log_sync_interval; 

	if (master){
		cfg_settings.dds.dds.flags &= ~DDS_SLAVE_ONLY;
	}else{
		cfg_settings.dds.dds.flags |= DDS_SLAVE_ONLY;
		cfg_settings.cfg_ignore |= CFG_IGNORE_SLAVEONLY;
	}
	// Force to be a slave -> prev lines commented by Sandeep (comment prev lines out to force slave mode)
	// cfg_settings.dds.dds.flags |= DDS_SLAVE_ONLY;
	// cfg_settings.cfg_ignore |= CFG_IGNORE_SLAVEONLY;

	kill = false;

	// Initialize Local Tracking Variable for Clock-Skew Statistics (Checks Staleness)
	last_clocksync_data_point.offset  = 0;
	last_clocksync_data_point.drift   = 0;
	last_clocksync_data_point.data_id = 0;

	// Initialize Global Variable for Clock-Skew Statistics 
	ptp_clocksync_data_point.offset  = 0;
	ptp_clocksync_data_point.drift   = 0;
	ptp_clocksync_data_point.data_id = 0;

	thread = boost::thread(boost::bind(&PTP::SyncThread, this, timelineid, timelinesfd, timelines_size));
}

void PTP::Stop()
{
	BOOST_LOG_TRIVIAL(info) << "Stopping PTP synchronization ";
	kill = true;
	thread.join();
}


int PTP::SyncThread(int timelineid, int *timelinesfd, uint16_t timelines_size)
{
	BOOST_LOG_TRIVIAL(info) << "Sync thread started ";
	char *config = NULL, *req_phc = NULL;
	int c, i;
	struct interface *iface;
	int *cfg_ignore = &cfg_settings.cfg_ignore;
	enum delay_mechanism *dm = &cfg_settings.dm;
	enum transport_type *transport = &cfg_settings.transport;
	enum timestamp_type *timestamping = &cfg_settings.timestamping;
	struct clock *clock;
	struct defaultDS *ds = &cfg_settings.dds.dds;
	int phc_index = -1, required_modes = 0;

	// Set fault timeouts
	for (i = 0; i < FT_CNT; i++)
	{
		cfg_settings.pod.flt_interval_pertype[i].type = FTMO_LOG2_SECONDS;
		cfg_settings.pod.flt_interval_pertype[i].val = 4;
	}

	// Add the single eth0 interface
	char ifname[16];
	strncpy(ifname, baseiface.c_str(), 16);
	config_create_interface(ifname, &cfg_settings);

	// Cannot be a slave and a grand master
	if (!cfg_settings.dds.grand_master_capable && ds->flags & DDS_SLAVE_ONLY)
	{
		fprintf(stderr,"Cannot mix 1588 slaveOnly with 802.1AS !gmCapable.\n");
		return -1;
	}
	if (!cfg_settings.dds.grand_master_capable || ds->flags & DDS_SLAVE_ONLY)
		ds->clockQuality.clockClass = 255;
	
	// Set some servo values
	if (cfg_settings.clock_servo == CLOCK_SERVO_NTPSHM)
	{
		cfg_settings.dds.kernel_leap = 0;
		cfg_settings.dds.sanity_freq_limit = 0;
	}

	// Debug 
	print_set_verbose(cfg_settings.verbose);
	print_set_syslog(cfg_settings.use_syslog);
	print_set_level(cfg_settings.print_level);

	if (STAILQ_EMPTY(&cfg_settings.interfaces))
	{
		fprintf(stderr, "no interface specified\n");
		return -1;
	}

	// If we have requested one step, then we need to pick hardware timestamping
	if (!(ds->flags & DDS_TWO_STEP_FLAG))
	{
		switch (*timestamping)
		{
		case TS_SOFTWARE:
		case TS_LEGACY_HW:
			fprintf(stderr, "one step is only possible with hardware time stamping\n");
			return -1;
		case TS_HARDWARE:
			*timestamping = TS_ONESTEP;
			break;
		case TS_ONESTEP:
			break;
		}
	}

	// Determine the required iface stamping modes
	switch (*timestamping) {
	case TS_SOFTWARE:
		required_modes |= SOF_TIMESTAMPING_TX_SOFTWARE |
			SOF_TIMESTAMPING_RX_SOFTWARE |
			SOF_TIMESTAMPING_SOFTWARE;
		break;
	case TS_LEGACY_HW:
		required_modes |= SOF_TIMESTAMPING_TX_HARDWARE |
			SOF_TIMESTAMPING_RX_HARDWARE |
			SOF_TIMESTAMPING_SYS_HARDWARE;
		break;
	case TS_HARDWARE:
	case TS_ONESTEP:
		required_modes |= SOF_TIMESTAMPING_TX_HARDWARE |
			SOF_TIMESTAMPING_RX_HARDWARE |
			SOF_TIMESTAMPING_RAW_HARDWARE;
		break;
	}

	// Probe interfaces
	STAILQ_FOREACH(iface, &cfg_settings.interfaces, list)
	{
		fprintf(stderr, "Configuring interface '%s'\n", iface->name);	
		config_init_interface(iface, &cfg_settings);
		if (iface->ts_info.valid && ((iface->ts_info.so_timestamping & required_modes) != required_modes))
		{
			fprintf(stderr, "interface '%s' does not support requested timestamping mode.\n", iface->name);
			//return -1;
			*timestamping = TS_SOFTWARE; //QOT, switch to software timestamping if hardware is not supported
		}
	}

	/* determine PHC Clock index */
	iface = STAILQ_FIRST(&cfg_settings.interfaces);
	if (cfg_settings.dds.free_running) {
		phc_index = -1;
	} else if (*timestamping == TS_SOFTWARE || *timestamping == TS_LEGACY_HW) {
		phc_index = -1;
	} else if (req_phc) {
		if (1 != sscanf(req_phc, "/dev/ptp%d", &phc_index)) {
			fprintf(stderr, "bad ptp device string\n");
			return -1;
		}
	} else if (iface->ts_info.valid) {
		phc_index = iface->ts_info.phc_index;
	} else {
		fprintf(stderr, "ptp device not specified and\n"
		                "automatic determination is not\n"
		                "supported. please specify ptp device\n");
		return -1;
	}

	if (phc_index >= 0) {
		pr_info("selected /dev/ptp%d as PTP clock", phc_index);
	}

	// Generate the clock identity
	if (generate_clock_identity(&ds->clockIdentity, iface->name))
	{
		fprintf(stderr, "failed to generate a clock identity\n");
		return -1;
	}

	// Create the clock
	clock = clock_create(phc_index, (struct interfaces_head*)&cfg_settings.interfaces, 
		*timestamping, &cfg_settings.dds, cfg_settings.clock_servo,
		timelineid, timelinesfd, timelines_size);
	if (!clock)
	{
		fprintf(stderr, "failed to create a clock\n");
		return -1;
	}

	// Keep going until kill called or ctrl+c is pressed
	while (is_running() && !kill)
	{
		if (clock_poll(clock)) break;

		// Check if a new skew statistic data point has been added
		if(last_clocksync_data_point.data_id < ptp_clocksync_data_point.data_id)
		{
			// New statistic received -> Replace old value
			last_clocksync_data_point = ptp_clocksync_data_point;

			// Add Synchronization Uncertainty Sample
			sync_uncertainty.CalculateBounds(last_clocksync_data_point.offset, ((double)last_clocksync_data_point.drift)/1000000000LL, timelinesfd[0]);
		}
	}

	// Clean up
	clock_destroy(clock);
	config_destroy(&cfg_settings);
	return 0;
}
