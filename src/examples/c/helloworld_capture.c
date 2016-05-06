/*
 * @file helloworld_capture.c
 * @brief Simple C example showing how to register to listen for capture events
 * @author Fatima Anwar and Andrew Symington
 * 
 * Copyright (c) Regents of the University of California, 2015. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright notice, 
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
#define DEBUG 0

static int running = 1;

static void exit_handler(int s)
{
	printf("Exit requested \n");
  	running = 0;
}

int main(int argc, char *argv[])
{
    FILE *fp1, *fp2, *fp3, *fp4, *fp5, *fp6;
	timeline_t *my_timeline1, *my_timeline2;
	timelength_t resolution = { .sec = 0, .asec = 1e9 }; // 1nsec
	timeinterval_t accuracy = { .below.sec = 0, .below.asec = 1e12, .above.sec = 0, .above.asec = 1e12 }; // 1usec

	// Grab the app name
	const char *cosec = "/mnt/capturetime.log";
	if (argc > 1)
		cosec = argv[1];

	const char *consec = "capturetime_nsec.log";
    if (argc > 2)
        consec = argv[2];

    const char *u_sec = "u_errors_sec.log";
    if (argc > 3)
        u_sec = argv[3];

    const char *u_nsec = "u_errors_nsec.log";
    if (argc > 4)
        u_nsec = argv[4];

    const char *l_sec = "l_errors_sec.log";
    if (argc > 5)
        l_sec = argv[5];

    const char *l_nsec = "l_errors_nsec.log";
    if (argc > 6)
        l_nsec = argv[6];

    if (argc > 7)
        accuracy.above.asec = atof(argv[7]);

    // Grab the timelines
    const char *t = TIMELINE_UUID;
    const char *t2 = "TML2";
    if (argc > 8)
        t = argv[8];

    // Grab the app name
    const char *m = APPLICATION_NAME;
    if (argc > 9)
        m = argv[9];

    // Grab the number of timelines to create
    int i = 0;
    if (argc > 10)
        i = atoi(argv[10]);

    /* open the files to write Core time, and errors */
      fp1 = fopen(cosec, "w");
      if (fp1 == NULL) {
         printf("Couldn't open coretime.log for writing.\n");
         exit(0);
      }
          /* open the file to write Core time */
      fp2 = fopen(consec, "w");
      if (fp2 == NULL) {
         printf("Couldn't open coretime_nsec.log for writing.\n");
         exit(0);
      }
          /* open the file to write Core time */
      fp3 = fopen(u_sec, "w");
      if (fp3 == NULL) {
         printf("Couldn't open u_errors_sec.log for writing.\n");
         exit(0);
      }

	  fp4 = fopen(u_nsec, "w");
      if (fp4 == NULL) {
         printf("Couldn't open u_errors_nsec.log for writing.\n");
         exit(0);
      }

      fp5 = fopen(l_sec, "w");
      if (fp4 == NULL) {
         printf("Couldn't open l_errors_sec.log for writing.\n");
         exit(0);
      }

      fp6 = fopen(l_nsec, "w");
      if (fp4 == NULL) {
         printf("Couldn't open l_errors_nsec.log for writing.\n");
         exit(0);
      }

/** CREATE TIMELINE ----------------------**/

	my_timeline1 = timeline_t_create();
	if(!my_timeline1)
	{
		printf("Unable to create the timeline1 data structure\n");
		QOT_RETURN_TYPE_ERR;
	}

	// Bind to a timeline
	printf("Binding to timeline 1 %s ........\n", t);
	if(timeline_bind(my_timeline1, t, m, resolution, accuracy))
	{
		printf("Failed to bind to timeline 1 %s\n", t);
		timeline_t_destroy(my_timeline1);
		return QOT_RETURN_TYPE_ERR;
	}

    if(i > 0){
        //Create 2nd timeline
        my_timeline2 = timeline_t_create();
        if(!my_timeline2)
        {
            printf("Unable to create the timeline2 data structure\n");
            QOT_RETURN_TYPE_ERR;
        }

        // Bind to a timeline
        printf("Binding to timeline 2 %s ........\n", t2);
        if(timeline_bind(my_timeline2, t2, m, resolution, accuracy))
        {
            printf("Failed to bind to timeline 2 %s\n", t2);
            timeline_t_destroy(my_timeline2);
            return QOT_RETURN_TYPE_ERR;
        }
    }

	char *device_m = "/dev/ptp1";               /* PTP device */
    int index_m = 1;                            /* Channel index, '1' corresponds to 'TIMER6' */
    int fd_m;                                   /* device file descriptor */

    int c, cnt;        
    struct ptp_clock_caps caps;                 /* Clock capabilities */
    struct ptp_pin_desc desc;                   /* Pin configuration */
    struct ptp_extts_event event;               /* PTP event */
    struct ptp_extts_request extts_request;     /* External timestamp req */
    stimepoint_t utp, utp2;

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
        if(DEBUG)
            printf("Trying to read events %d\n", running++);

        cnt = read(fd_m, &event, sizeof(event));
        if (cnt != sizeof(event)) {
            //perror("cannot read event");
            printf("Cannot read event");
            break;
        }
        if(DEBUG)
            printf("Core - %lld.%09u\n", event.t.sec, event.t.nsec);

        utp.estimate.sec  = event.t.sec;
        utp.estimate.asec = event.t.nsec*nSEC_PER_SEC;
        // Get the core time
        timeline_core2rem(my_timeline1, &utp);
        
        if(i > 0){
            utp2.estimate.sec  = event.t.sec;
            utp2.estimate.asec = event.t.nsec*nSEC_PER_SEC;
            timeline_core2rem(my_timeline2, &utp2);
        }

        if(DEBUG){
            printf("TML1 - %lld.%llu\n", utp.estimate.sec, (utp.estimate.asec / nSEC_PER_SEC));
            //printf("UERR - %llu.%llu\n", utp.u_estimate.sec, (utp.u_estimate.asec / nSEC_PER_SEC));
            //printf("LERR - %llu.%llu\n", utp.l_estimate.sec, (utp.l_estimate.asec / nSEC_PER_SEC));
        } else{
            printf("TML1 - %lld.%llu\n", utp.estimate.sec, (utp.estimate.asec / nSEC_PER_SEC));
            printf("UERR - %llu.%llu\n", utp.u_estimate.sec, (utp.u_estimate.asec / nSEC_PER_SEC));
            printf("LERR - %llu.%llu\n", utp.l_estimate.sec, (utp.l_estimate.asec / nSEC_PER_SEC));
            fprintf(fp1, "%lld,\n", utp.estimate.sec);
            fprintf(fp2, "%llu,\n", (utp.estimate.asec / nSEC_PER_SEC));
            fprintf(fp3, "%llu,\n", utp.u_estimate.sec);
            fprintf(fp4, "%llu,\n", (utp.u_estimate.asec / nSEC_PER_SEC));
            fprintf(fp5, "%llu,\n", utp.l_estimate.sec);
            fprintf(fp6, "%llu,\n", (utp.l_estimate.asec / nSEC_PER_SEC));

            if(i > 0){
                printf("TML2 - %lld.%llu\n", utp2.estimate.sec, (utp2.estimate.asec / nSEC_PER_SEC));
                fprintf(fp3, "%lld,\n", utp2.estimate.sec);
                fprintf(fp4, "%llu,\n", (utp2.estimate.asec / nSEC_PER_SEC));
            }
            
        }
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
		printf("Failed to unbind from timeline 1 %s\n", t2);
		timeline_t_destroy(my_timeline1);
		return QOT_RETURN_TYPE_ERR;
	}
	printf("Unbound from timeline 1 %s\n", t);

    // Free the timeline data structure
    timeline_t_destroy(my_timeline1);

    if(i > 0){
        // Unbind from timeline 2
        if(timeline_unbind(my_timeline2))
        {
          printf("Failed to unbind from timeline 2 %s\n", t2);
          timeline_t_destroy(my_timeline2);
           return QOT_RETURN_TYPE_ERR;
        }
        printf("Unbound from timeline 2 %s\n", t2);
        // Free the timeline data structure
        timeline_t_destroy(my_timeline2);
    }



    /* close the files */
      fclose(fp1);
      fclose(fp2);
      fclose(fp3);
      fclose(fp4);
      fclose(fp5);
      fclose(fp6);

	/* Success */
	return 0;
}
