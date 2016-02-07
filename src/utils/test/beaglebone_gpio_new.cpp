/*
 * @file helloworld_timeline.cpp
 * @brief Simple C++ example showing how to register to listen for capture events
 * @author Andrew Symington and Fatima Anwar 
 * 
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


// C++ includes
#include <iostream>

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h> 

// Include the QoT API
#include "qot.hpp"
#include "beaglebone_gpio.h"

// Basic onfiguration
#define TIMELINE_UUID    "my_test_timeline"
#define APPLICATION_NAME "default"
#define WAIT_TIME_SECS   1
#define NUM_ITERATIONS   100
#define PERIOD_MSEC      1500

// This function is called by the QoT API when a capture event occus
void callback(const std::string &name, uint8_t event)
{
	switch(event)
	{
		case qot::TIMELINE_EVENT_CREATE : 
			std::cout << "Timeline created"; 
			break;
		case qot::TIMELINE_EVENT_DESTROY : 
			std::cout << "Timeline destroyed"; 
			break;
		case qot::TIMELINE_EVENT_JOIN : 
			std::cout << "Peer joined timeline"; 
			break;
		case qot::TIMELINE_EVENT_LEAVE : 
			std::cout << "Peer left timeline"; 
			break;
		case qot::TIMELINE_EVENT_SYNC : 
			std::cout << "Timeline synchronization update"; 
			break;
		case qot::TIMELINE_EVENT_CAPTURE : 
			std::cout << "Capture event on this timeline"; 
			break;
		case qot::TIMELINE_EVENT_UDPATE  : 
			std::cout << "Local timeline parameters updated"; 
			break;
	}
}

// Main entry point of application
int main(int argc, char *argv[])
{
	// GPIO Stuff
	volatile void *gpio_addr = NULL;
    volatile unsigned int *gpio_oe_addr = NULL;
    volatile unsigned int *gpio_setdataout_addr = NULL;
    volatile unsigned int *gpio_cleardataout_addr = NULL;
    unsigned int reg;
    int fd = open("/dev/mem", O_RDWR);

    printf("Mapping %X - %X (size: %X)\n", GPIO1_START_ADDR,  GPIO1_END_ADDR, GPIO1_SIZE);

    gpio_addr = mmap(0, GPIO1_SIZE, PROT_READ | PROT_WRITE,  MAP_SHARED, fd, GPIO1_START_ADDR);

    gpio_oe_addr = gpio_addr + GPIO_OE;
    gpio_setdataout_addr = gpio_addr + GPIO_SETDATAOUT;
    gpio_cleardataout_addr = gpio_addr + GPIO_CLEARDATAOUT;

    if(gpio_addr == MAP_FAILED) {
        printf("Unable to map GPIO\n");
        exit(1);
    }
    printf("GPIO mapped to %p\n", gpio_addr);
    printf("GPIO OE mapped to %p\n", gpio_oe_addr);
    printf("GPIO SETDATAOUTADDR mapped to %p\n", gpio_setdataout_addr);
    printf("GPIO CLEARDATAOUT mapped to %p\n", gpio_cleardataout_addr);

    reg = *gpio_oe_addr;
    printf("GPIO1 configuration: %X\n", reg);
    reg = reg & (0xFFFFFFFF - PIN19);
    *gpio_oe_addr = reg;
    printf("GPIO1 configuration: %X\n", reg);
	
	// Allow this to go on for a while
	int period_msec = PERIOD_MSEC;
	if (argc > 1)
		period_msec = atoi(argv[1]);

	int n = NUM_ITERATIONS;
	if (argc > 2)
		n = atoi(argv[2]);

	// Grab the timeline
	const char *u = TIMELINE_UUID;
	if (argc > 3)
		u = argv[3];

	// Grab the timeline
	const char *m = APPLICATION_NAME;
	if (argc > 4)
		m = argv[4];
    std::cout << "Starting Helloworld  WaitUntilNextPeriod with GPIO pins going off\n"; 
	// Bind to a timeline
	try
	{
		
		qot::Timeline timeline(u, 1, 1);	
		
		timeline.SetEventCallback(callback);
		timeline.SetName(m);
		
		for (int i = 0; i < n; i++)
		{
			//int64_t tval = timeline.GetTime();
			//std::cout << "[Iteration " << (i+1) << "] " << tval << std::endl;
			//std::cout << "WAITING UNTIL NEXT PERIOD, PERIOD = " << PERIOD_MSEC << "ms AT " << tval/1000000000LL << " seconds "<< tval % 1000000000LL << " ns" << std::endl;
			timeline.WaitUntilNextPeriod((int64_t)period_msec * 1e6, 0);
			//tval = timeline.GetTime();
			//std::cout << "RESUMED AT " << tval/1000000000LL << " seconds "<< tval % 1000000000LL << "ns" << std::endl;
			*gpio_setdataout_addr= PIN19;

			//tval = timeline.GetTime();
			//std::cout << "[Iteration " << (i+1) << "] " << tval << std::endl;
			//std::cout << "WAITING UNTIL NEXT PERIOD, PERIOD = " << PERIOD_MSEC << "ms AT " << tval/1000000000LL << " seconds "<< tval % 1000000000LL << " ns" << std::endl;
			timeline.WaitUntilNextPeriod((int64_t)period_msec * 1e6, 0);
			//tval = timeline.GetTime();
			//std::cout << "RESUMED AT " << tval/1000000000LL << " seconds "<< tval % 1000000000LL << "ns" << std::endl;
			*gpio_cleardataout_addr = PIN19;
		}
	}
	catch (std::exception &e)
	{
		std::cout << "Exiting Helloworld\n"; 
		std::cout << "EXCEPTION: " << e.what() << std::endl;
	}

	// Unbind from timeline
	return 0;
}