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

// This file's include
#include "Synchronization.hpp"

using namespace qot;

Synchronization::Synchronization(boost::asio::io_service *io)
	: asio(io)
{
}

Synchronization::~Synchronization()
{

}

/*


static void usage(char *progname)
{
	fprintf(stderr,
		"\nusage: %s [options]\n\n"
		" Delay Mechanism\n\n"
		" -A        Auto, starting with E2E\n"
		" -E        E2E, delay request-response (default)\n"
		" -P        P2P, peer delay mechanism\n\n"
		" Network Transport\n\n"
		" -2        IEEE 802.3\n"
		" -4        UDP IPV4 (default)\n"
		" -6        UDP IPV6\n\n"
		" Time Stamping\n\n"
		" -H        HARDWARE (default)\n"
		" -S        SOFTWARE\n"
		" -L        LEGACY HW\n\n"
		" Other Options\n\n"
		" -f [file] read configuration from 'file'\n"
		" -i [dev]  interface device to use, for example 'eth0'\n"
		"           (may be specified multiple times)\n"
		" -p [dev]  PTP hardware clock device to use, default auto\n"
		"           (ignored for SOFTWARE/LEGACY HW time stamping)\n"
		" -s        slave only mode (overrides configuration file)\n"
		" -l [num]  set the logging level to 'num'\n"
		" -m        print messages to stdout\n"
		" -q        do not print messages to the syslog\n"
		" -v        prints the software version and exits\n"
		" -h        prints this message and exits\n"
		"\n",
		progname);
}

int main(int argc, char *argv[])
{
	char *config = NULL, *req_phc = NULL, *progname;
	int c, i;
	struct interface *iface;
	int *cfg_ignore = &cfg_settings.cfg_ignore;
	enum delay_mechanism *dm = &cfg_settings.dm;
	enum transport_type *transport = &cfg_settings.transport;
	enum timestamp_type *timestamping = &cfg_settings.timestamping;
	struct clock *clock;
	struct defaultDS *ds = &cfg_settings.dds.dds;
	int phc_index = -1, required_modes = 0;

	if (handle_term_signals())
		return -1;

	// Set fault timeouts to a default value 
	for (i = 0; i < FT_CNT; i++) {
		cfg_settings.pod.flt_interval_pertype[i].type = FTMO_LOG2_SECONDS;
		cfg_settings.pod.flt_interval_pertype[i].val = 4;
	}

	// Process the command line arguments.
	progname = strrchr(argv[0], '/');
	progname = progname ? 1+progname : argv[0];
	while (EOF != (c = getopt(argc, argv, "AEP246HSLf:i:p:sl:mqvh"))) {
		switch (c) {
		case 'A':
			*dm = DM_AUTO;
			*cfg_ignore |= CFG_IGNORE_DM;
			break;
		case 'E':
			*dm = DM_E2E;
			*cfg_ignore |= CFG_IGNORE_DM;
			break;
		case 'P':
			*dm = DM_P2P;
			*cfg_ignore |= CFG_IGNORE_DM;
			break;
		case '2':
			*transport = TRANS_IEEE_802_3;
			*cfg_ignore |= CFG_IGNORE_TRANSPORT;
			break;
		case '4':
			*transport = TRANS_UDP_IPV4;
			*cfg_ignore |= CFG_IGNORE_TRANSPORT;
			break;
		case '6':
			*transport = TRANS_UDP_IPV6;
			*cfg_ignore |= CFG_IGNORE_TRANSPORT;
			break;
		case 'H':
			*timestamping = TS_HARDWARE;
			*cfg_ignore |= CFG_IGNORE_TIMESTAMPING;
			break;
		case 'S':
			*timestamping = TS_SOFTWARE;
			*cfg_ignore |= CFG_IGNORE_TIMESTAMPING;
			break;
		case 'L':
			*timestamping = TS_LEGACY_HW;
			*cfg_ignore |= CFG_IGNORE_TIMESTAMPING;
			break;
		case 'f':
			config = optarg;
			break;
		case 'i':
			if (!config_create_interface(optarg, &cfg_settings))
				return -1;
			break;
		case 'p':
			req_phc = optarg;
			break;
		case 's':
			ds->flags |= DDS_SLAVE_ONLY;
			*cfg_ignore |= CFG_IGNORE_SLAVEONLY;
			break;
		case 'l':
			if (get_arg_val_i(c, optarg, &cfg_settings.print_level,
					  PRINT_LEVEL_MIN, PRINT_LEVEL_MAX))
				return -1;
			*cfg_ignore |= CFG_IGNORE_PRINT_LEVEL;
			break;
		case 'm':
			cfg_settings.verbose = 1;
			*cfg_ignore |= CFG_IGNORE_VERBOSE;
			break;
		case 'q':
			cfg_settings.use_syslog = 0;
			*cfg_ignore |= CFG_IGNORE_USE_SYSLOG;
			break;
		case 'v':
			version_show(stdout);
			return 0;
		case 'h':
			usage(progname);
			return 0;
		case '?':
			usage(progname);
			return -1;
		default:
			usage(progname);
			return -1;
		}
	}

	if (config && (c = config_read(config, &cfg_settings))) {
		return c;
	}
	if (!cfg_settings.dds.grand_master_capable &&
	    ds->flags & DDS_SLAVE_ONLY) {
		fprintf(stderr,
			"Cannot mix 1588 slaveOnly with 802.1AS !gmCapable.\n");
		return -1;
	}
	if (!cfg_settings.dds.grand_master_capable ||
	    ds->flags & DDS_SLAVE_ONLY) {
		ds->clockQuality.clockClass = 255;
	}
	if (cfg_settings.clock_servo == CLOCK_SERVO_NTPSHM) {
		cfg_settings.dds.kernel_leap = 0;
		cfg_settings.dds.sanity_freq_limit = 0;
	}

	print_set_progname(progname);
	print_set_verbose(cfg_settings.verbose);
	print_set_syslog(cfg_settings.use_syslog);
	print_set_level(cfg_settings.print_level);

	if (STAILQ_EMPTY(&cfg_settings.interfaces)) {
		fprintf(stderr, "no interface specified\n");
		usage(progname);
		return -1;
	}

	if (!(ds->flags & DDS_TWO_STEP_FLAG)) {
		switch (*timestamping) {
		case TS_SOFTWARE:
		case TS_LEGACY_HW:
			fprintf(stderr, "one step is only possible "
				"with hardware time stamping\n");
			return -1;
		case TS_HARDWARE:
			*timestamping = TS_ONESTEP;
			break;
		case TS_ONESTEP:
			break;
		}
	}

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

	// Init interface configs and check whether timestamping mode is supported
	STAILQ_FOREACH(iface, &cfg_settings.interfaces, list) {
		config_init_interface(iface, &cfg_settings);
		if (iface->ts_info.valid &&
		    ((iface->ts_info.so_timestamping & required_modes) != required_modes)) {
			fprintf(stderr, "interface '%s' does not support "
				        "requested timestamping mode.\n",
				iface->name);
			return -1;
		}
	}

	// determine PHC Clock index 
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

	if (generate_clock_identity(&ds->clockIdentity, iface->name)) {
		fprintf(stderr, "failed to generate a clock identity\n");
		return -1;
	}

	clock = clock_create(phc_index, &cfg_settings.interfaces,
			     *timestamping, &cfg_settings.dds,
			     cfg_settings.clock_servo);
	if (!clock) {
		fprintf(stderr, "failed to create a clock\n");
		return -1;
	}

	while (is_running()) {
		if (clock_poll(clock))
			break;
	}

	clock_destroy(clock);
	config_destroy(&cfg_settings);
	return 0;
}

*/