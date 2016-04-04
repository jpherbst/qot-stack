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

/* Add servo and clock adjustment code from LinuxPTP */
#include "sync/ptp/linuxptp/config.h"
#include "sync/ptp/linuxptp/servo.h"
#include "sync/ptp/linuxptp/clockadj.h"

/* Useful definitions */
#define NSEC_PER_SEC  ((int64_t)1000000000)
#define NSEC_PER_MSEC ((int64_t)1000000)
#define FD_TO_CLOCKID(fd) ((~(clockid_t) (fd) << 3) | 3)
#ifndef CLOCK_INVALID
#define CLOCK_INVALID -1
#endif

////////////////////////////////////////////////////////

/* From: http://web.mit.edu/~tcoffee/Public/rss/common/timespec.c */
static void ptp_clock_addns(struct ptp_clock_time *ts, int64_t ns)
{
	int64_t sec = ns / NSEC_PER_SEC;
	ns = ns - sec * NSEC_PER_SEC;
	ts->nsec += ns;
	ts->sec += ts->nsec / NSEC_PER_SEC + sec;
	ts->nsec = ts->nsec % NSEC_PER_SEC;
}

static int64_t ptp_clock_diff(struct ptp_clock_time *a, struct ptp_clock_time *b)
{
	int64_t tmp = ((int64_t) a->sec - (int64_t) b->sec) * NSEC_PER_SEC;
	tmp += ((int64_t) a->nsec - (int64_t) b->nsec);
	return tmp;
}

static uint64_t ptp_clock_u64(struct ptp_clock_time *ts)
{
	return ((uint64_t)ts->sec) * NSEC_PER_SEC + (uint64_t) ts->nsec;
}

static double ptp_clock_double(struct ptp_clock_time *ts)
{
	return ((double)ts->sec) + ((double) ts->nsec) / (double) NSEC_PER_SEC;
}

static int running = 1;

static void exit_handler(int s)
{
	printf("Exit requested \n");
  	running = 0;
}

static void usage(char *progname)
{
	fprintf(stderr,
		"usage: %s [options]\n"
		" -m name    master device to open (typically system PHC)\n"
		" -s name    slave device to open (typically interface PHC)\n"
		" -M val     channel index for master (perout)\n"
		" -S val     channel index for slave (extts)\n"
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
	int index_s = 1;							/* Channel index (slave) */
	int64_t period = NSEC_PER_MSEC*1000*6;		/* Synchronization period (ms) */
	int64_t error = 1000;						/* Timestamp tolerance (nsec) */

	/* Internal variables */
	int c, cnt;									/* Used by getopt() */
	int fd_m;									/* Master file descriptor */
	int fd_s;									/* Slave file descriptor */
	struct ptp_clock_caps caps;					/* Clock capabilities */
	struct ptp_pin_desc desc;					/* Pin configuration */
	struct ptp_extts_event event;				/* PTP event */
	struct ptp_extts_request extts_request;		/* External timestamp req */
	struct ptp_perout_request perout_request;	/* Programmable interrupt */
	clockid_t clkid_s, clkid_m;					/* Clock ids */
	struct timespec ts;							/* Time storage */

	/* How we plan to discipline the clock */
	int max_ppb;
	double ppb, weight;
	uint64_t local_ts;
	int64_t offset;
	struct servo *servo;
	//struct config *cfg = config_create();
	//if (!cfg)
	//	perror("cannot create config");

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
			period = NSEC_PER_MSEC * (int64_t) atoi(optarg);
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
	clkid_m = FD_TO_CLOCKID(fd_m);
	if (CLOCK_INVALID == clkid_m) {
		perror("master: failed to read clock id\n");
		return -1;
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
	clkid_s = FD_TO_CLOCKID(fd_s);
	if (CLOCK_INVALID == clkid_s) {
		perror("slave: failed to read clock id\n");
		return -1;
	}

	/* Get the master's time */	
	if (clock_gettime(clkid_m, &ts)) {
		perror("master: clock_gettime");
	}

	/* Configure master pulse per second to start at deterministic point in future */
	memset(&perout_request, 0, sizeof(perout_request));
	perout_request.index = index_m;
	perout_request.start.sec = ceil(ts.tv_sec + 1);
	perout_request.start.nsec = 0;
	perout_request.period.sec = 0;
	perout_request.period.nsec = 0;
	ptp_clock_addns(&perout_request.period, period);
	if (ioctl(fd_m, PTP_PEROUT_REQUEST, &perout_request)) {
		perror("master: cannot reequest periodic output");
	}

	/* Request timestamps from the slave */
	memset(&extts_request, 0, sizeof(extts_request));
	extts_request.index = index_s;
	extts_request.flags = PTP_ENABLE_FEATURE;
	if (ioctl(fd_s, PTP_EXTTS_REQUEST, &extts_request)) {
		perror("slave: cannot request external timestamp");
	}

	/* Determine slave max frequency adjustment */
	if (ioctl(fd_s, PTP_CLOCK_GETCAPS, &caps)) {
		perror("slave: cannot get capabilities");
	}
	max_ppb = caps.max_adj;
	if (!max_ppb) {
		perror("slave: clock is not adjustable");
	}

	/* Initialize clock discipline */
	clockadj_init(clkid_s);

	/* Get the current ppb error */
	ppb = clockadj_get_freq(clkid_s);
	
	/* Create a servo */
	servo = servo_create(
		//cfg,			/* Servo configuration */
		CLOCK_SERVO_PI,	/* Servo type */ 
		-ppb,			/* Current frequency adjustment */ 
		max_ppb, 		/* Max frequency adjustment */
		0				/* 0: hardware, 1: software */
	);

	/* Set the servo sync interval (in fractional seconds) */
	servo_sync_interval(
		servo, 		
		ptp_clock_double(&perout_request.period)
	);

	/* This will save the servo state */
	enum servo_state state;

	/* Keep checking for time stamps */
	signal(SIGINT, exit_handler);
	while (running) {
		/* Read events coming in */
		cnt = read(fd_s, &event, sizeof(event));
                if (cnt != sizeof(event)) {
                        perror("slave: cannot read event");
                        break;
                }
		/* Work out the predicted timestamp */
		ptp_clock_addns(&perout_request.start, period);

		/* Local timestamp and offset */
		local_ts = ptp_clock_u64(&perout_request.start);
		offset = ptp_clock_diff(&event.t, &perout_request.start);
		weight = 1.0;

		/* Update the clock */
		ppb = servo_sample(
			servo, 			/* Servo object */
			offset,			/* T(slave) - T(master) */
			local_ts, 		/* T(master) */
			//weight,			/* Weighting */
			&state 			/* Next state */
		);
		
		/* What we do depends on the servo state */
		switch (state) {
		case SERVO_UNLOCKED:
			break;
		case SERVO_JUMP:
			clockadj_set_freq(clkid_s, -ppb);
			clockadj_step(clkid_s, -offset);
		case SERVO_LOCKED:
			clockadj_set_freq(clkid_s, -ppb);
			break;
		}

		/* Print some debug info */
		printf("EVENT CAPTURED : [%d] offset %9lld freq %+7.0f\n", state, offset, ppb);
		printf("- SYS PHC at %lld.%09u\n", 
			perout_request.start.sec, perout_request.start.nsec);
		printf("- NIC PHC at %lld.%09u\n", 
			event.t.sec, event.t.nsec);
		fflush(stdout);
	}

	/* Destroy the servo */
	servo_destroy(servo);

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
