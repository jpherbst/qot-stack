/*
 * @file helloworld_control_pwm.c
 * @brief Simple C example showing how to trigger output compare (PWM) with varying duty cycles
 * @author Sandeep D'souza
 * 
 * Copyright (c) Carnegie Mellon University, 2016.
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
#define TIMELINE_UUID         "my_test_timeline"
#define APPLICATION_NAME      "default"
#define OFFSET_MSEC           2000
#define DUTY_CYCLE            50
#define PWM_CHANGE_PERIOD_MS  10000

static int running = 1;

static void exit_handler(int s)
{
	printf("Exit requested \n");
  	running = 0;
}

int main(int argc, char *argv[])
{
    timeline_t *my_timeline;
	timelength_t resolution = { .sec = 0, .asec = 1e9 }; // 1nsec
	timeinterval_t accuracy = { .below.sec = 0, .below.asec = 1e12, .above.sec = 0, .above.asec = 1e12 }; // 1usec

    utimepoint_t wake_now;
    timepoint_t wake;

    qot_perout_t request;
    qot_callback_t callback;
    qot_event_t event;
    timelength_t pwm_change_period;

    // Initialize change period
    TL_FROM_mSEC(pwm_change_period, PWM_CHANGE_PERIOD_MS);

	// Grab the timeline
	const char *u = TIMELINE_UUID;
	if (argc > 1)
		u = argv[1];

	// Grab the application name
	const char *m = APPLICATION_NAME;
	if (argc > 2)
		m = argv[2];

    // Loop Interval
    int step_size_ms = OFFSET_MSEC;
    if (argc > 3)
        step_size_ms = atoi(argv[3]);

    // Duty Cycle
    request.duty_cycle = DUTY_CYCLE;
    if (argc > 4)
        request.duty_cycle = atoi(argv[4]);

    // Starting Edge -> Should be 1 for Rising, 2 for Falling
    request.edge = QOT_TRIGGER_RISING;
    if (argc > 5)
        request.edge = atoi(argv[5]);

    // Initialize stepsize
    TL_FROM_mSEC(request.period, (int64_t) step_size_ms);

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
        timepoint_add(&wake, &request.period);
        wake.asec = 0;
    }    

    signal(SIGINT, exit_handler);

    while (running) {
        // Configure Periodic request
        request.start.sec = wake.sec + 1;
        request.start.asec = wake.asec;
        if(timeline_enable_output_compare(my_timeline, &request))
        {
            printf("Cannot request periodic output\n");
            goto terminate;
        }
        else
        {
            printf("Output Compare Succesful\n");
        }
        if(timeline_read_events(my_timeline, &event))
        {
            printf("Could not read events\n");
            //goto exit_point;
        }
        printf("Event detected at %lld %llu with event code %d\n", event.timestamp.estimate.sec, event.timestamp.estimate.asec, event.type);

        timepoint_add(&wake, &pwm_change_period);
        wake_now.estimate = wake;
        timeline_waituntil(my_timeline, &wake_now);
        request.duty_cycle = request.duty_cycle*6/7;
    }

terminate:
    if(timeline_disable_output_compare(my_timeline, &request))
    {
        printf("Cannot disable periodic output\n");
    }
    printf("Output Compare disabled\n");

    /** DESTROY TIMELINE **/
exit_point:
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
	return 0;
}
