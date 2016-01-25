/*
 * PTP 1588 clock support - User space test program
 *
 * Copyright (C) 2010 OMICRON electronics GmbH
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#define _GNU_SOURCE
#define __SANE_USERSPACE_TYPES__        /* For PPC64, to get LL64 types */
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/timex.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <linux/ptp_clock.h>

#define NSEC_PER_MSEC ((long)1000000)

#ifndef ADJ_SETOFFSET
#define ADJ_SETOFFSET 0x0100
#endif

#ifndef CLOCK_INVALID
#define CLOCK_INVALID -1
#endif

#define FD_TO_CLOCKID(fd) ((~(clockid_t) (fd) << 3) | 3)

/* From: http://web.mit.edu/~tcoffee/Public/rss/common/timespec.c */
static void timespec_addns(struct ptp_clock_time *ts, long ns)
{
	int sec = ns / 1000000000;
	ns = ns - sec * 1000000000;
	ts->nsec += ns;
	ts->sec += ts->nsec / 1000000000 + sec;
	ts->nsec = ts->nsec % 1000000000;
}

static int running = 1;

static void exit_handler(int s) {
	printf("Exit requested \n");
  	running = 0;
}

static void usage(char *progname)
{
	fprintf(stderr,
		"usage: %s [options]\n"
		" -m name    master device to open (typically system PHC)\n"
		" -d name    slave device to open (typically interface PHC)\n"
		" -M val     channel index for master (perout)\n"
		" -D val     channel index for slave (extts)\n"
		" -p val     sync period (msec)\n"
		" -e val     error tolerance (nsec)\n"
		" -h         print help\n",
		progname);
}

int main(int argc, char *argv[])
{
	/* Default Parameters */
	char *progname;								/* This binary name */
	char *device_m = "/dev/ptp1";				/* PTP device (master) */
	char *device_s = "/dev/ptp0";				/* PTP device (slave) */
	int index_m = 0;							/* Channel index (master) */
	int index_s = 0;							/* Channel index (slave) */
	long period = NSEC_PER_MSEC*1000;			/* Synchronization period (ms) */
	long error = 1000;							/* Timestamp tolerance (nsec) */

	/* Internal variables */
	int c, cnt;									/* Used by getopt() */
	int fd_m;									/* Master file descriptor */
	int fd_s;									/* Slave file descriptor */
	struct ptp_clock_caps caps;					/* Clock capabilities */
	struct ptp_pin_desc desc;					/* Pin configuration */
	struct ptp_extts_event event;				/* PTP event */
	struct ptp_extts_request extts_request;		/* External timestamp req */
	struct ptp_perout_request perout_request;	/* Programmable interrupt */
	clockid_t clkid;							/* Clock ID */
	struct timespec ts;							/* Time storage */

	/* Argument parsing */
	progname = strrchr(argv[0], '/');
	progname = progname ? 1 + progname : argv[0];
	while (EOF != (c = getopt(argc, argv, "h:e:m:s:p:M:S"))) {
		switch (c) {
		case 'm':
			device_m = optarg;
			break;
		case 's':
			device_s = optarg;
			break;
		case 'e':
			error = atoi(optarg);
			break;
		case 'p':
			period = NSEC_PER_MSEC*atoi(optarg);
			break;
		case 'M':
			index_m = atoi(optarg);
			break;
		case 'S':
			index_s = atoi(optarg);
			break;
		case 'h':
			usage(progname);
			return 0;
		case '?':
		default:
			usage(progname);
			return -1;
		}
	}

	/* Open master character device */
	fd_m = open(device_m, O_RDWR);
	if (fd_m < 0) {
		fprintf(stderr, "opening master %s: %s\n", device_m, strerror(errno));
		return -1;
	}
	memset(&desc, 0, sizeof(desc));
	desc.index = index_m;
	desc.func = 2;
	desc.chan = index_m;
	if (ioctl(fd_m, PTP_PIN_SETFUNC, &desc)) {
		perror("master: PTP_PIN_SETFUNC");
	}

	/* Open slave character device */
	fd_s = open(device_s, O_RDWR);
	if (fd_s < 0) {
		fprintf(stderr, "opening slave %s: %s\n", device_s, strerror(errno));
		return -1;
	}
	memset(&desc, 0, sizeof(desc));
	desc.index = index_s;
	desc.func = 1;
	desc.chan = index_s;
	if (ioctl(fd_s, PTP_PIN_SETFUNC, &desc)) {
		perror("slave: PTP_PIN_SETFUNC");
	}

	/* Query the current time */
	clkid = FD_TO_CLOCKID(fd_m);
	if (CLOCK_INVALID == clkid) {
		fprintf(stderr, "failed to read master clock id\n");
		return -1;
	}
	if (clock_gettime(clkid, &ts)) {
		perror("master: clock_gettime");
	}

	/* Configure master pulse per second to start at deterministic point in future */
	memset(&perout_request, 0, sizeof(perout_request));
	perout_request.index = index_m;
	perout_request.start.sec = ceil(ts.tv_sec + 1);
	perout_request.start.nsec = 0;
	perout_request.period.sec = 0;
	perout_request.period.nsec = 0;
	timespec_addns(&perout_request.period, period);
	if (ioctl(fd_m, PTP_PEROUT_REQUEST, &perout_request)) {
		perror("PTP_PEROUT_REQUEST");
	} else {
		puts("periodic output request okay");
	}

	/* Request timestamps from the slave */
	memset(&extts_request, 0, sizeof(extts_request));
	extts_request.index = index_s;
	extts_request.flags = PTP_ENABLE_FEATURE;
	if (ioctl(fd_s, PTP_EXTTS_REQUEST, &extts_request)) {
		perror("PTP_EXTTS_REQUEST");
	}

	/* Keep checking for time stamps */
	signal(SIGINT, exit_handler);
	while (running) {
		cnt = read(fd_s, &event, sizeof(event));
		if (cnt != sizeof(event)) {
			perror("read");
			break;
		}
		timespec_addns(&perout_request.start, period);
		printf("EVENT CAPTURED\n");
		printf("- SYS PHC at %lld.%09u\n", perout_request.start.sec, perout_request.start.nsec);
		printf("- NIC PHC at %lld.%09u\n", event.t.sec, event.t.nsec);
		fflush(stdout);
	}

	/* Disable the feature again. */
	extts_request.flags = 0;
	if (ioctl(fd_s, PTP_EXTTS_REQUEST, &extts_request)) {
		perror("PTP_EXTTS_REQUEST");
	}

	/* Disable the master pin */ 	
	memset(&desc, 0, sizeof(desc));
	desc.index = index_m;
	desc.func = 0;
	desc.chan = index_m;
	if (ioctl(fd_m, PTP_PIN_SETFUNC, &desc)) {
		perror("master: PTP_PIN_SETFUNC");
	}

	/* Disable the slave pin */
	memset(&desc, 0, sizeof(desc));
	desc.index = index_s;
	desc.func = 0;
	desc.chan = index_s;
	if (ioctl(fd_s, PTP_PIN_SETFUNC, &desc)) {
		perror("slave: PTP_PIN_SETFUNC");
	}

	/* Close the character devices */
	close(fd_s);
	close(fd_m);
	
	/* Success */
	return 0;
}