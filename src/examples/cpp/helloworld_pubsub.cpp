/*
 * @file helloworld.c
 * @brief Simple C++ example showing how to use pub-sub capabilities
 * @author Sandeep D'souza 
 * 
 * Copyright (c) Carnegie Mellon University, 2018.
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
extern "C" {
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
}

#include <iostream>

// Include the QoT CPP API
#include "../../api/cpp/qot.hpp"

// Basic onfiguration
#define TIMELINE_UUID    "my_test_timeline"
#define APPLICATION_NAME "default"
#define OFFSET_MSEC      1000
#define DEFAULT_ROLE     "pub"

#define DEBUG 1

using namespace qot;

static int running = 1;

static void exit_handler(int s)
{
	printf("Exit requested \n");
  	running = 0;
}

static void messaging_handler(const qot_message_t *msg)
{
    printf("Message Received from %s\n", msg->name);
    printf("Message Type %d\n", msg->type);
}

// Main entry point of application
int main(int argc, char *argv[])
{
	// Variable declarations
	timeline_t *my_timeline;
	qot_return_t retval;
	utimepoint_t est_now;
	utimepoint_t wake_now;
	timepoint_t  wake;
	timelength_t step_size;
	qot_message_t message;

	// Application resolution and accuracy requirements
	timelength_t resolution;
	timeinterval_t accuracy;

	// Accuracy and resolution values
	resolution.sec = 0;
	resolution.asec = 1e9;
	accuracy.below.sec = 0;
	accuracy.below.asec = 1e12;
	accuracy.above.sec = 0;
	accuracy.above.asec = 1e12;

	// Publish Period
	int step_size_ms = OFFSET_MSEC;

	// Grab the timeline
	const char *u = TIMELINE_UUID;
	if (argc > 1)
		u = argv[1];

	// Grab the application name
	const char *m = APPLICATION_NAME;
	if (argc > 2)
		m = argv[2];

    // Run in Publish or Subscribe Mode
    const char *role = DEFAULT_ROLE;
    int pub_flag = 1;
    if (argc > 3)
    {
        if (strcmp (argv[3], "pub") == 0)
        {
        	role = argv[3];
        	pub_flag = 1;
        	std::cout << "Selected the publisher role\n";
        }
        else if (strcmp (argv[3], "sub") == 0)
        {
        	role = argv[3];
        	pub_flag = 0;
        	std::cout << "Selected the subscriber role\n";
        }
        else
        {
        	std::cout << "Invalid Role, enter either pub or sub\n";
        	return -1;
        }
    }

    // Set topic type
    int global_flag = 0; // Default local topic
    if (argc > 4)
    {
    	if (strcmp (argv[4], "global") == 0)
        {
        	global_flag = 1;
        	std::cout << "Global Topic selected\n";
        }
        else if (strcmp (argv[4], "local") == 0)
        {
        	pub_flag = 0;
        	std::cout << "Local Topic Selected\n";
        }
        else if (strcmp (argv[4], "gopt") == 0)
        {
        	pub_flag = 2;
        	std::cout << "Global Optimized Topic Selected\n";
        }
        else
        {
        	std::cout << "Invalid Role, enter either global, local or gopt\n";
        	return -1;
        }
    }

    signal(SIGINT, exit_handler);

	// Initialize stepsize
	TL_FROM_mSEC(step_size, step_size_ms);

	if(DEBUG)
		printf("Helloworld starting.... process id %i\n", getpid());

	// Create Timeline Data Structure
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
		goto exit_point;
	}
	else
	{
		wake = wake_now.estimate;
		timepoint_add(&wake, &step_size);
        wake.asec = 0;
	}

	if(pub_flag == 1)
	{
		/* Run a Periodic Publisher */
		// Setup a publisher -> passing the topic name, topic type, node name and timeline uuid
		Publisher publisher("test", (qot::TopicType) global_flag, std::string(m), std::string(u));

		// Populate the messaging structure
		strcpy(message.name, m);
		message.type = QOT_MSG_DATA;
		message.timestamp = wake_now;
		strcpy(message.data, "Test Message !");

		// Run Periodic Publisher
		while(running)
		{
			if(timeline_gettime(my_timeline, &est_now))
			{
				printf("Could not read timeline reference time\n");
				goto exit_point;
			}
			else
			{
				std::cout << "Publishing Message\n";
				publisher.Publish(message);
			}
			timepoint_add(&wake, &step_size);
			wake_now.estimate = wake;
			timeline_waituntil(my_timeline, &wake_now);
		}
	}
	else
	{
		/* Run an asynchronous Subscriber */
		// Setup a publisher -> passing the topic name, topic type, timeline uuid and messaging handler
		Subscriber subscriber("test", (qot::TopicType) global_flag, std::string(u), messaging_handler);
		
		// Busy Loop till termination
		while (running)
			sleep(1);
	}

/** DESTROY TIMELINE **/
exit_point:
	// Unbind from timeline
	if(timeline_unbind(my_timeline))
	{
		printf("Failed to unbind from timeline  %s\n", u);
		timeline_t_destroy(my_timeline);
		return QOT_RETURN_TYPE_ERR;
	}
	printf("Unbound from timeline %s\n", u);

	// Free the timeline data structure
	timeline_t_destroy(my_timeline);	
	return 0;
}