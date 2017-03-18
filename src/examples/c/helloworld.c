/*
 * @file helloworld.c
 * @brief Simple C example showing how to use wait_until 
 * @author Andrew Symington, Sandeep D'souza and Fatima Anwar 
 * 
 * Copyright (c) Regents of the University of California, 2015. All rights reserved.
 * Copyright (c) Carnegie Mellon University, 2016.
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
 */


// C includes
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


// Include the QoT API
#include "../../api/c/qot.h"

// Basic onfiguration
#define TIMELINE_UUID    "my_test_timeline"
#define APPLICATION_NAME "default"
#define OFFSET_MSEC      1000

#define DEBUG 1

static int running = 1;

static void exit_handler(int s)
{
	printf("Exit requested \n");
  	running = 0;
}

// Main entry point of application
int main(int argc, char *argv[])
{
	// Variable declaration
	timeline_t *my_timeline;
	qot_return_t retval;
	utimepoint_t est_now;
	utimepoint_t wake_now;
	timepoint_t  wake;
	timelength_t step_size;

	timelength_t resolution = { .sec = 0, .asec = 1e9 }; // 1nsec
	timeinterval_t accuracy = { .below.sec = 0, .below.asec = 1e12, .above.sec = 0, .above.asec = 1e12 }; // 1usec

	int i;

	int step_size_ms = OFFSET_MSEC;

	// Grab the timeline
	const char *u = TIMELINE_UUID;
	if (argc > 1)
		u = argv[1];

	// Grab the application name
	const char *m = APPLICATION_NAME;
	if (argc > 2)
		m = argv[2];

        // Loop Interval
        if (argc > 3)
                step_size_ms = atoi(argv[3]);

	// Initialize stepsize
	TL_FROM_mSEC(step_size, step_size_ms);

	if(DEBUG)
		printf("Helloworld starting.... process id %i\n", getpid());

	my_timeline = timeline_t_create();
	if(!my_timeline)
	{
		printf("Unable to create the timeline_t data structure\n");
		QOT_RETURN_TYPE_ERR;
	}

	// Bind to a timeline
	if(DEBUG)
		printf("Binding to timeline %s ........\n", u);
	if(timeline_bind(my_timeline, u, m, resolution, accuracy))
	{
		printf("Failed to bind to timeline %s\n", u);
		timeline_t_destroy(my_timeline);
		return QOT_RETURN_TYPE_ERR;
	}
	// Read Initial Time
    if(timeline_gettime(my_timeline, &wake_now))
	{
		printf("Could not read timeline reference time\n");
		// Unbind from timeline
		if(timeline_unbind(my_timeline))
		{
			printf("Failed to unbind from timeline %s\n", u);
			timeline_t_destroy(my_timeline);
			return QOT_RETURN_TYPE_ERR;
		}
		timeline_t_destroy(my_timeline);
		return QOT_RETURN_TYPE_ERR;
	}
	else
	{
		wake = wake_now.estimate;
		timepoint_add(&wake, &step_size);
        wake.asec = 0;
	}

	signal(SIGINT, exit_handler);


	while(running)
	{
		if(timeline_gettime(my_timeline, &est_now))
		{
			printf("Could not read timeline reference time\n");
			// Unbind from timeline
			if(timeline_unbind(my_timeline))
			{
				printf("Failed to unbind from timeline %s\n", u);
				timeline_t_destroy(my_timeline);
				return QOT_RETURN_TYPE_ERR;
			}
			timeline_t_destroy(my_timeline);
			return QOT_RETURN_TYPE_ERR;
		}
		else if (DEBUG)
		{
			printf("[Iteration %d ]: core time =>\n", i++);
			printf("Scheduled wake up          %lld %llu\n", wake_now.estimate.sec, wake_now.estimate.asec);
			printf("Time Estimate @ wake up    %lld %llu\n", est_now.estimate.sec, est_now.estimate.asec);
			printf("Uncertainity below         %llu %llu\n", est_now.interval.below.sec, est_now.interval.below.asec);
			printf("Uncertainity above         %llu %llu\n", est_now.interval.above.sec, est_now.interval.above.asec);
			printf("WAITING FOR %d ms\n", step_size_ms);

		}
		timepoint_add(&wake, &step_size);
		wake_now.estimate = wake;
		timeline_waituntil(my_timeline, &wake_now);
	}
	

	// Unbind from timeline
	if(timeline_unbind(my_timeline))
	{
		printf("Failed to unbind from timeline %s\n", u);
		timeline_t_destroy(my_timeline);
		return QOT_RETURN_TYPE_ERR;
	}
	if(DEBUG)
		printf("Unbound from timeline %s\n", u);

	// Free the timeline data structure
	timeline_t_destroy(my_timeline);
	return 0;
}
