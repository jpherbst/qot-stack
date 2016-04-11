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

// Include the QoT API
#include "../../api/c/qot.h"

// Basic onfiguration
#define TIMELINE_UUID    "my_test_timeline"
#define APPLICATION_NAME "default"

static int running = 1;

static void exit_handler(int s)
{
	printf("Exit requested \n");
  	running = 0;
}

int main(int argc, char *argv[])
{
	timeline_t *my_timeline1;
	timelength_t resolution = { .sec = 0, .asec = 1e9 }; // 1nsec
	timeinterval_t accuracy = { .below.sec = 0, .below.asec = 1e12, .above.sec = 0, .above.asec = 1e12 }; // 1usec

	// Allow this to go on for a while
	int n = 0;
	int i;

	if (argc > 1)
		n = atoi(argv[1]);

	// Grab the timeline
	const char *u = TIMELINE_UUID;
	if (argc > 2)
		u = argv[2];

	// Grab the timeline
	const char *m = APPLICATION_NAME;
	if (argc > 3)
		m = argv[3];

/** CREATE TIMELINE ----------------------**/

	my_timeline1 = timeline_t_create();
	if(!my_timeline1)
	{
		printf("Unable to create the timeline1 data structure\n");
		QOT_RETURN_TYPE_ERR;
	}

	// Bind to a timeline
	printf("Binding to timeline 1 %s ........\n", u);
	if(timeline_bind(my_timeline1, u, m, resolution, accuracy))
	{
		printf("Failed to bind to timeline 1 %s\n", u);
		timeline_t_destroy(my_timeline1);
		return QOT_RETURN_TYPE_ERR;
	}

	char *device_m = "/dev/ptp1";               /* PTP device */
    int index_m = 1;                            /* Channel index, '1' corresponds to 'TIMER6' */
    int fd_m;                                   /* device file descriptor */

    int c, cnt;        
    struct ptp_clock_caps caps;                 /* Clock capabilities */
    struct ptp_pin_desc desc;                   /* Pin configuration */
    struct ptp_extts_event event;               /* PTP event */
    struct ptp_extts_request extts_request;     /* External timestamp req */
    timepoint_t tp;

    /* Open the character device */
    fd_m = open(device_m, O_RDWR);
    if (fd_m < 0) {
        //fprintf(stderr, "opening device %s: %s\n", device_m, strerror(errno));
        printf("Failed to open devide %s\n", device_m);
        return QOT_RETURN_TYPE_ERR;
    }
    printf("Device opened %d\n", fd_m);
    memset(&desc, 0, sizeof(desc));
    desc.index = index_m;
    desc.func = 1;              // '1' corresponds to external timestamp
    desc.chan = index_m;
    if (ioctl(fd_m, PTP_PIN_SETFUNC, &desc)) {
        //perror("PTP_PIN_SETFUNC Enable");
        printf("Set pin func failed for %d\n", fd_m);
        return QOT_RETURN_TYPE_ERR;
    }
    printf("Set pin func succees for %d\n", fd_m);
    
    // Request timestamps from the pin 
    memset(&extts_request, 0, sizeof(extts_request));
    extts_request.index = index_m;
    extts_request.flags = PTP_ENABLE_FEATURE;
    if (ioctl(fd_m, PTP_EXTTS_REQUEST, &extts_request)) {
        //perror("cannot request external timestamp");
        printf("Requesting timestamps failed for %d\n", fd_m);
        return QOT_RETURN_TYPE_ERR;
    }
    printf("Requesting timestamps success for %d\n", fd_m);
    
    // Keep checking for time stamps
    
    signal(SIGINT, exit_handler);
    while (running) {
        /* Read events coming in */
        printf("Trying to read events %d\n", running);
        cnt = read(fd_m, &event, sizeof(event));
        if (cnt != sizeof(event)) {
            //perror("cannot read event");
            printf("Cannot read event");
            break;
        }
        printf("Core - %lld.%09u\n", event.t.sec, event.t.nsec);

        tp.sec  = event.t.sec;
        tp.asec = event.t.nsec*nSEC_PER_SEC;

        // Get the core time
        timeline_core2rem(my_timeline1, &tp);

        printf("TML -%lld.%llu\n", tp.sec, (tp.asec / nSEC_PER_SEC));
    }

    /* Disable the pin */
    memset(&desc, 0, sizeof(desc));
    desc.index = index_m;
    desc.func = 0;              // '0' corresponds to no function 
    desc.chan = index_m;
    if (ioctl(fd_m, PTP_PIN_SETFUNC, &desc)) {
        perror("PTP_PIN_SETFUNC Disable");
    }
    
    /* Close the character device */
    close(fd_m);

/** DESTROY TIMELINE ----------------------**/

	// Unbind from timeline
	if(timeline_unbind(my_timeline1))
	{
		printf("Failed to unbind from timeline 1 %s\n", u);
		timeline_t_destroy(my_timeline1);
		return QOT_RETURN_TYPE_ERR;
	}
	printf("Unbound from timeline 1 %s\n", u);

	// Free the timeline data structure
	timeline_t_destroy(my_timeline1);

	/* Success */
	return 0;
}
