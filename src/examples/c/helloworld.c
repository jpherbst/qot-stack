/*
 * @file helloworld.c
 * @brief Simple C example showing how to register to listen for capture events
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
#include <stdio.h>
#include <unistd.h>

// Include the QoT API
#include "../../api/c/qot.h"

// Basic onfiguration
#define TIMELINE_UUID    "my_test_timeline"
#define APPLICATION_NAME "default"
#define WAIT_TIME_SECS   1
#define NUM_ITERATIONS   10
#define OFFSET_MSEC      1000LL

// Main entry point of application
int main(int argc, char *argv[])
{
	// Variable declaration
	timeline_t *my_timeline;
	qot_return_t retval;
	timelength_t resolution;
	timeinterval_t accuracy;
	utimepoint_t est_now;
	utimepoint_t wake_now;
	timepoint_t  wake;
	timelength_t step_size;

	// Allow this to go on for a while
	int n = NUM_ITERATIONS;
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

	// Initialize stepsize
	TL_FROM_mSEC(step_size, OFFSET_MSEC);
	printf("Helloworld starting.... process id %i\n", getpid());
	//usleep(5000000);

	my_timeline = timeline_t_create();
	if(!my_timeline)
	{
		printf("Unable to create the timeline_t data structure\n");
		QOT_RETURN_TYPE_ERR;
	}

	// Bind to a timeline
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
	}

	for (i = 0; i < n; i++)
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
		else
		{
			printf("[Iteration %d ]: core time =>\n", i+1);
			printf("Actual wake up          %lld %llu\n", wake_now.estimate.sec, wake_now.estimate.asec);
			printf("Time Estimate @ wake up %lld %llu\n", est_now.estimate.sec, est_now.estimate.asec);
			printf("Uncertainity below %llu %llu\n", est_now.interval.below.sec, est_now.interval.below.asec);
			printf("Uncertainity above %llu %llu\n", est_now.interval.above.sec, est_now.interval.above.asec);

		}

		printf("WAITING FOR %lld ms\n", OFFSET_MSEC);
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
	printf("Unbound from timeline %s\n", u);

	// Free the timeline data structure
	timeline_t_destroy(my_timeline);
	return 0;
}