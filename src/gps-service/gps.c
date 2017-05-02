/*
 * @file qot.h
 * @brief Header file for the GPS module
 * @author Fatima Anwar
 *
 * Copyright (c) Regents of the University of California, 2015.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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

#include "gps.h"

// Basic onfiguration
#define TIMELINE_UUID    "my_test_timeline"
#define APPLICATION_NAME "default"
#define OFFSET_MSEC      2000
#define DUTY_CYCLE       50

/**
 * launch output compare functionality
 **/
qot_return_t gps_compare(struct timespec start, int period_msec, int duty, qot_trigger_t edge, struct timespec stop){

	timeline_t *my_timeline;
    timelength_t resolution = { .sec = 0, .asec = 1e9 }; // 1nsec
    timeinterval_t accuracy = { .below.sec = 0, .below.asec = 1e12, .above.sec = 0, .above.asec = 1e12 }; // 1usec

    utimepoint_t wake_now;
    timepoint_t wake;

    qot_perout_t request;
    qot_callback_t callback;
    qot_event_t event;

    int i;
    int64_t start_sec;
    uint64_t start_nsec;

    const char *u = TIMELINE_UUID;
    const char *m = APPLICATION_NAME;

    // Period: minimum period is 1 millisecond
    TL_FROM_mSEC(request.period, (int64_t) period_msec);

    // Duty Cycle
    request.duty_cycle = duty;

    // Starting Edge -> Should be 1 for Rising, 2 for Falling
    request.edge = edge;
 

    /** CREATE TIMELINE **/
    my_timeline = timeline_t_create();
    if(!my_timeline)
    {
        printf("Unable to create the timeline data structure\n");
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
        //timepoint_add(&wake, &request.period);
        //wake.asec = 0;
    }    

    // Start time
    start_sec = (int64_t) start.tv_sec + wake.sec;
    start_nsec = (uint64_t) start.tv_nsec + wake.asec;

    request.start.sec  = start_sec / nSEC_PER_SEC;
	request.start.asec = (start_nsec % nSEC_PER_SEC ) * ASEC_PER_NSEC;

    if(timeline_enable_output_compare(my_timeline, &request))
    {
        printf("Cannot request periodic output\n");
        // Unbind from timeline
    	if(timeline_unbind(my_timeline))
    	{
        	printf("Failed to unbind from timeline  %s\n", u);
        	timeline_t_destroy(my_timeline);
        	return QOT_RETURN_TYPE_ERR;
    	}
    	printf("Unbound from timeline  %s\n", u);

    	// Free the timeline data structure
    	timeline_t_destroy(my_timeline);
    	return QOT_RETURN_TYPE_ERR;
    }
    else
    {
        printf("Output Compare Succesful\n");
    }

    do {
        if(timeline_read_events(my_timeline, &event))
        {
            printf("Could not read events\n");
        }
        printf("Event detected at %lld %llu with event code %d\n", event.timestamp.estimate.sec, event.timestamp.estimate.asec, event.type);

    } while (event.timestamp.estimate.sec < stop.tv_sec && event.timestamp.estimate.asec << (stop.tv_nsec * ASEC_PER_NSEC));

    if(timeline_disable_output_compare(my_timeline, &request))
    {
        printf("Cannot disable periodic output\n");
    }
    printf("Output Compare disabled\n");

    /** DESTROY TIMELINE **/

    // Unbind from timeline
    if(timeline_unbind(my_timeline))
    {
        printf("Failed to unbind from timeline  %s\n", u);
        timeline_t_destroy(my_timeline);
        return QOT_RETURN_TYPE_ERR;
    }
    printf("Unbound from timeline  %s\n", u);

    // Free the timeline data structure
    timeline_t_destroy(my_timeline);

    /* Success */
    return QOT_RETURN_TYPE_OK;
}