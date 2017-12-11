/*
 * @file quad-robot_coordination1.cpp
 * @brief Quad Robot-Arm Coordination Demo Master
 * @author Sandeep D'souza 
 * 
 * Copyright (c) Carnegie Mellon University, 2017.
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
#include <iostream>
#include <sstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

// Include the QoT CPP API
#include "../../api/cpp/qot.hpp"

// Basic onfiguration
#define TIMELINE_UUID    "my_test_timeline"
#define APPLICATION_NAME "Coordinator"
#define OFFSET_MSEC      3000

#define DEBUG 1

// Coordination Running Variables
static int running = 1;

static void exit_handler(int s)
{
	printf("Exit requested \n");
  	running = 0;
}

// Main entry point of application
int main(int argc, char *argv[])
{
	// Variable declarations
	timeline_t *my_timeline;
	qot_return_t retval;
	qot_message_t message;

	timelength_t resolution;
	timeinterval_t accuracy;
	utimepoint_t start_time;
	timepoint_t start;
	timelength_t step_size;

	// Accuracy and resolution values
	resolution.sec = 0;
	resolution.asec = 1e9;

	accuracy.below.sec = 0;
	accuracy.below.asec = 1e12;
	accuracy.above.sec = 0;
	accuracy.above.asec = 1e12;

	// Set the Cluster Nodes which will take part in the coordination
	std::vector<std::string> Nodes = {"Coordinator","ControlArm1", "ControlArm2", "ControlArm3", "ControlArm4"};

	// Grab the timeline
	const char *u = TIMELINE_UUID;
	if (argc > 1)
		u = argv[1];

	// Grab the application name
	const char *m = APPLICATION_NAME;
	if (argc > 2)
		m = argv[2];


	if(DEBUG)
		printf("Coordination Master starting.... process id %i\n", getpid());

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

	// Define Cluster
	if(timeline_define_cluster(my_timeline, Nodes, NULL))
	{
		printf("Failed to define cluster\n");
		goto exit_point;
	}

	// Wait for Peers to Join
	printf("Waiting for peers to join ...\n");
	if(timeline_wait_for_peers(my_timeline))
	{
		printf("Failed to wait for peers\n");
		goto exit_point;
	}
	
	// Install the signal handler
	signal(SIGINT, exit_handler);

	sleep(5);

	// Initialize stepsize
	TL_FROM_mSEC(step_size, OFFSET_MSEC);

	// Read Initial Time
    if(timeline_gettime(my_timeline, &start_time))
	{
		printf("Could not read timeline reference time\n");
		goto exit_point;
	}

	// Create the coordination start timestamp
	start = start_time.estimate;
	timepoint_add(&start, &step_size);
    start.asec = 0;
    start_time.estimate = start;

    std::cout << "Sending Coordination Start Message" << std::endl;
    // Publish the start Message
    strcpy(message.name, m);
	message.type = QOT_MSG_COORD_START;
	message.timestamp = start_time;
	timeline_publish_message(my_timeline, message);

	// Busy Loop until a SIGINT is received
	while (running)
		sleep(1);

	// Stop Coordination if this node terminates
	message.type = QOT_MSG_COORD_STOP;
	timeline_publish_message(my_timeline, message);


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
	return 0;
}