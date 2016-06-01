/*
 * @file helloworld_compare.c
 * @brief Simple C example showing how to trigger output compare
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

// Include the BBB GPIO MMIO Configuration
#include "beaglebone_gpio.h"

// Basic onfiguration
#define TIMELINE_UUID    "my_test_timeline"
#define APPLICATION_NAME "default"
#define OFFSET_MSEC      1000

#define ANALYZE 1
char filename[100] = "/mnt/qot_gpio";
FILE *fp;

static int running = 1;

static void exit_handler(int s)
{
	printf("Exit requested \n");
  	running = 0;
}

int main(int argc, char *argv[])
{
	char file_timestamp[20];
    timeline_t *my_timeline;
	timelength_t resolution = { .sec = 0, .asec = 1e9 }; // 1nsec
	timeinterval_t accuracy = { .below.sec = 0, .below.asec = 1e12, .above.sec = 0, .above.asec = 1e12 }; // 1usec

    utimepoint_t wake_now;
    timepoint_t wake;
    timelength_t stepsize;

    qot_perout_t request;
    qot_callback_t callback;

    int step_size_ms = OFFSET_MSEC;

    // GPIO Registers
    volatile void *gpio_addr = NULL;
    volatile unsigned int *gpio_oe_addr = NULL;
    volatile unsigned int *gpio_setdataout_addr = NULL;
    volatile unsigned int *gpio_cleardataout_addr = NULL;
    unsigned int reg;

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
    TL_FROM_mSEC(stepsize, (int64_t) step_size_ms);

    // Configure GPIO using Memory Mapped IO
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
    reg = reg & (0xFFFFFFFF - PIN);
    *gpio_oe_addr = reg;
    printf("GPIO1 configuration: %X\n", reg);

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
        goto exit_point;
    }
    else
    {
        if(ANALYZE)
        {
          sprintf(file_timestamp, "%lld", wake_now.estimate.sec);
          strcat(filename, file_timestamp);
          fp = fopen(filename, "w");
        }
        wake = wake_now.estimate;
        timepoint_add(&wake, &stepsize);
        if(wake.sec % 2 != 0)
            wake.sec += 1;
        wake.asec = 0;
    }    

     // Configure Periodic request
    request.start.sec = wake.sec + 1;
    request.start.asec = wake.asec;

    // Initialize stepsize
    TL_FROM_mSEC(request.period, (int64_t) 2*step_size_ms);

    // Exit Handler on SIGINT
    signal(SIGINT, exit_handler);

    if(timeline_enable_output_compare(my_timeline, &request))
    {
        printf("Cannot request periodic output\n");
        goto exit_point;
    }
    else
    {
        printf("Output Compare Succesful\n");
    }

    printf("Start toggling GPIO PIN \n");
    while (running) {
        timepoint_add(&wake, &stepsize);
        wake_now.estimate = wake;
        timeline_waituntil(my_timeline, &wake_now);
        // Toggle Pins
        if(wake.sec % 2 == 0)
            *gpio_setdataout_addr= PIN;
        else
            *gpio_cleardataout_addr = PIN;
        if(ANALYZE)
            fprintf(fp, "%lld\t%llu\t%lld\t%llu\n", wake.sec, wake.asec, wake_now.estimate.sec, wake_now.estimate.asec);

    }

    // Disable Output compare
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

    if(ANALYZE)
        fclose(fp);

    // Close the MMIO file
    close(fd);
	/* Success */
	return 0;
}
