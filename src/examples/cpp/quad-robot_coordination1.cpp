/*
 * @file quad-robot_coordination1.cpp
 * @brief Quad Robot-Arm Coordination Demo Type 1 Controller Node
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
#define APPLICATION_NAME "default"
#define OFFSET_MSEC      3000

#define DEBUG 1

#define BUFSIZE 1024
#define SERVER_IP "192.168.1.110"
#define SERVER_PORT 1200

// Socket Communication Variables
int sockfd, portno;
int serverlen;
struct sockaddr_in serveraddr;
struct hostent *server;
char buf[BUFSIZE];

// Coordination Running Variables
int running = -1;
utimepoint_t start_time;

static void exit_handler(int s)
{
	printf("Exit requested \n");
  	running = 0;
}

static void send_message(std::string message)
{
	int n;
	/* get a message from the user */
    bzero(buf, BUFSIZE);
    sprintf(buf, "%s", message.c_str());

    std::cout << "Sending Message " << message << std::endl;
    /* send the message to the server */
    serverlen = sizeof(serveraddr);
    n = sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *)&serveraddr, serverlen);
    if (n < 0) 
      printf("ERROR in sendto\n");
    
    /* print the server's reply */
    n = recvfrom(sockfd, buf, strlen(buf), 0, (struct sockaddr *)&serveraddr, (unsigned int*)&serverlen);
    if (n < 0) 
      printf("ERROR in recvfrom\n");
    printf("Echo from server: %s\n", buf);
}

static void messaging_handler(const qot_message_t *msg)
{
    if (msg->type == QOT_MSG_COORD_STOP)
    {
    	// If stop message received then nodes will terminate
    	running = 0;
    	std::cout << "Received Coordination Stop Message" << std::endl;
    }
    else if (msg->type == QOT_MSG_COORD_START)
    {
    	// If start message received nodes will start coordination
    	running = 1;
    	start_time = msg->timestamp;
    	std::cout << "Received Coordination Start Message " << start_time.estimate.sec << " " 
    			                                           << start_time.estimate.asec << std::endl;
    }
}

// Main entry point of application
int main(int argc, char *argv[])
{
	// Variable declarations
	timeline_t *my_timeline;
	qot_return_t retval;
	utimepoint_t wake_now;
	timepoint_t  wake;
	timelength_t step_size;
	qot_message_t message;

	timelength_t resolution;
	timeinterval_t accuracy;

	// Accuracy and resolution values
	resolution.sec = 0;
	resolution.asec = 1e9;

	accuracy.below.sec = 0;
	accuracy.below.asec = 1e12;
	accuracy.above.sec = 0;
	accuracy.above.asec = 1e12;

	// Set the Cluster Nodes which will take part in the coordination
	std::vector<std::string> Nodes = {"Coordinator","ControlArm1", "ControlArm2", "ControlArm3", "ControlArm4"};

	// Add Commands to Command Vector
	std::vector<std::string> command;
    int command_vect_size = 0;
	command.push_back("ArmGripPos");
	command.push_back("Arm45");
	command.push_back("ArmUp");
	command.push_back("ElbowBendFront");
	command.push_back("ArmBendFront");
	command.push_back("ArmUp");
	command.push_back("ArmHome");
	command.push_back("ArmSpinBack");
	command_vect_size = command.size();

	// Set the Message types the node wants to subscribe to
	std::set<qot_msg_type_t> MsgTypes;
	MsgTypes.insert(QOT_MSG_COORD_START);
	MsgTypes.insert(QOT_MSG_SENSOR_VAL);
	//MsgTypes.insert(QOT_MSG_COORD_STOP);

	int i = 0;

	// Grab the timeline
	const char *u = TIMELINE_UUID;
	if (argc > 1)
		u = argv[1];

	// Grab the application name
	const char *m = APPLICATION_NAME;
	if (argc > 2)
		m = argv[2];

    // Grab Arm Server IP
    const char *hostname = SERVER_IP;
    if (argc > 3) 
        hostname = argv[3];
    
    // Grab Server Port
    portno = SERVER_PORT;
    if (argc > 4) 
    	portno = atoi(argv[4]);

    // Periodic command interval
    int step_size_ms = OFFSET_MSEC;
    if (argc > 5) 
    	step_size_ms = atoi(argv[5]);

    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        printf("ERROR opening socket\n");
        exit(0);
    }

    /* gethostbyname: get the server's DNS entry */
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        exit(0);
    }

    /* build the server's Internet address */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
	  (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portno);

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
	// Define Cluster
	if(timeline_define_cluster(my_timeline, Nodes, NULL))
	{
		printf("Failed to define cluster\n");
		goto exit_point;
	}

	// Define Message types to subscribe to
	if(timeline_subscribe_message(my_timeline, MsgTypes, messaging_handler))
	{
		printf("Failed to subscribe to message\n");
		goto exit_point;
	}

	// Wait for Peers to Join
	printf("Waiting for peers to join ...\n");
	if(timeline_wait_for_peers(my_timeline))
	{
		printf("Failed to wait for peers\n");
		goto exit_point;
	}

	// Wait for Coordination start message
	while (running == -1)
		sleep(1);

	// Install the signal handler
	signal(SIGINT, exit_handler);

	std::cout << "Entered the Running State" << std::endl;

	wake_now = start_time;
	wake = wake_now.estimate;
	timeline_waituntil(my_timeline, &wake_now);

	// Periodic Sleep Till the program is terminated
	while(running)
	{
		// Send Message to Robot Arm
		send_message(command[i]);

		// Loop over Command Vector
		i = (i + 1) % command_vect_size;

		// Set Next Wakeup Point
		timepoint_add(&wake, &step_size);
        wake.asec = 0;
        wake_now.estimate = wake;
        timeline_waituntil(my_timeline, &wake_now);
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
	printf("Unbound from timeline  %s\n", u);

	// Free the timeline data structure
	timeline_t_destroy(my_timeline);	
	return 0;
}