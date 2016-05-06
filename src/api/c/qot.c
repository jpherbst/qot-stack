/*
 * @file qot.h
 * @brief A simple C application programmer interface to the QoT stack
 * @author Andrew Symington, Sandeep D'souza and Fatima Anwar
 *
 * Copyright (c) Regents of the University of California, 2015.
 * Copyright (c) Carnegie Mellon University, 2016.
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

/* System includes */
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

#include <linux/ptp_clock.h>

/* This file includes */
#include "qot.h"

#define DEBUG 0

/* Timeline implementation */
typedef struct timeline {
    qot_timeline_t info;    /* Basic timeline information               */
    qot_binding_t binding;  /* Basic binding info                       */
    int fd;                 /* File descriptor to /dev/timelineX ioctl  */
    int qotusr_fd;          /* File descriptor to /dev/qotusr ioctl     */
} timeline_t;

/* Is the given timeline a valid one */
qot_return_t timeline_check_fd(timeline_t *timeline) {
    if (fcntl(timeline->fd, F_GETFD)==-1)
        return QOT_RETURN_TYPE_ERR;
    return QOT_RETURN_TYPE_OK;
}


/* Exported methods */

timeline_t *timeline_t_create()
{
    timeline_t *timeline;
    timeline = (timeline_t*) malloc(sizeof(struct timeline));
    return timeline;
}

void timeline_t_destroy(timeline_t *timeline)
{
    free(timeline);
}

qot_return_t timeline_bind(timeline_t *timeline, const char *uuid, const char *name, timelength_t res, timeinterval_t acc) 
{
    char qot_timeline_filename[15];
    int usr_file;
    

    // Check to make sure the UUID is valid
    if (strlen(uuid) > QOT_MAX_NAMELEN)
        return QOT_RETURN_TYPE_ERR;

    // Open the QoT Core
    if (DEBUG) 
        printf("Opening IOCTL to qot_core\n");
    usr_file = open("/dev/qotusr", O_RDWR);
    if (DEBUG)
        printf("IOCTL to qot_core opened %d\n", usr_file);

    if (usr_file < 0)
    {
        printf("Error: Invalid file\n");
        return QOT_RETURN_TYPE_ERR;
    }

    timeline->qotusr_fd = usr_file;
    
    // Bind to the timeline
    if (DEBUG) 
        printf("Binding to timeline %s\n", uuid);

    strcpy(timeline->info.name, uuid);  

    // Try to create a new timeline if none exists
    if(ioctl(timeline->qotusr_fd, QOTUSR_CREATE_TIMELINE, &timeline->info) < 0)
    {
        return QOT_RETURN_TYPE_ERR;
    }
    // Construct the file handle to the posix clock /dev/timelineX
    sprintf(qot_timeline_filename, "/dev/timeline%d", timeline->info.index);

    // Open the clock
    if (DEBUG) 
        printf("Opening clock %s\n", qot_timeline_filename);
    timeline->fd = open(qot_timeline_filename, O_RDWR);
    if (!timeline->fd)
    {
        printf("Cant open /dev/timeline%d\n", timeline->info.index);
        return QOT_RETURN_TYPE_ERR;
    }

    
    
    if (DEBUG) 
        printf("Opened clock %s\n", qot_timeline_filename);
    // Populate Binding fields
    strcpy(timeline->binding.name, name);
    timeline->binding.demand.resolution = res;
    timeline->binding.demand.accuracy = acc;
    
    if (DEBUG) 
        printf("Binding to timeline %s\n", uuid);
    // Bind to the timeline
    if(ioctl(timeline->fd, TIMELINE_BIND_JOIN, &timeline->binding) < 0)
    {
        return QOT_RETURN_TYPE_ERR;
    }
    if (DEBUG) 
        printf("Bound to timeline %s\n", uuid);

    // This was there in the old api code
    // if (DEBUG) std::cout << "Start polling " << std::endl;

    // // We can now start polling, because the timeline is setup
    // this->cv.notify_one();

    return QOT_RETURN_TYPE_OK;
}

qot_return_t timeline_unbind(timeline_t *timeline) 
{
    if(!timeline)
        return QOT_RETURN_TYPE_ERR;

    // Unbind from the timeline
    if(ioctl(timeline->fd, TIMELINE_BIND_LEAVE, &timeline->binding) < 0)
    {
        return QOT_RETURN_TYPE_ERR;
    }

    // Close the timeline file
    if (timeline->fd)
        close(timeline->fd);

    // Try to destroy the timeline if possible (will destroy if no other bindings exist)
    if(ioctl(timeline->qotusr_fd, QOTUSR_DESTROY_TIMELINE, &timeline->info) == 0)
    {
       if(DEBUG)
          printf("Timeline %d destroyed\n", timeline->info.index);
    }
    else
    {
       if(DEBUG)
          printf("Timeline %d not destroyed\n", timeline->info.index);
    }

    if (timeline->qotusr_fd)
        close(timeline->qotusr_fd);
    
    return QOT_RETURN_TYPE_OK;
}

qot_return_t timeline_get_accuracy(timeline_t *timeline, timeinterval_t *acc) 
{
    if(!timeline)
        return QOT_RETURN_TYPE_ERR;
    *acc = timeline->binding.demand.accuracy;
    return QOT_RETURN_TYPE_OK;
}

qot_return_t timeline_get_resolution(timeline_t *timeline, timelength_t *res) 
{
    if(!timeline)
        return QOT_RETURN_TYPE_ERR;
    *res = timeline->binding.demand.resolution;
    return QOT_RETURN_TYPE_OK;
}

qot_return_t timeline_get_name(timeline_t *timeline, char *name) 
{
    if(!timeline)
        return QOT_RETURN_TYPE_ERR;
    strcpy(name, timeline->binding.name);
    return QOT_RETURN_TYPE_OK;
}

qot_return_t timeline_get_uuid(timeline_t *timeline, char *uuid) 
{
    if(!timeline)
        return QOT_RETURN_TYPE_ERR;
    strcpy(uuid, timeline->info.name);
    return QOT_RETURN_TYPE_OK;
}

qot_return_t timeline_set_accuracy(timeline_t *timeline, timeinterval_t *acc) 
{
    if(!timeline)
        return QOT_RETURN_TYPE_ERR;

    if (fcntl(timeline->fd, F_GETFD)==-1)
        return QOT_RETURN_TYPE_ERR;
    
    timeline->binding.demand.accuracy = *acc;
    // Update the binding
    if(ioctl(timeline->fd, TIMELINE_BIND_UPDATE, &timeline->binding) < 0)
    {
        return QOT_RETURN_TYPE_ERR;
    }
    *acc = timeline->binding.demand.accuracy;
    return QOT_RETURN_TYPE_OK;
}

qot_return_t timeline_set_resolution(timeline_t *timeline, timelength_t *res) 
{
    if(!timeline)
        return QOT_RETURN_TYPE_ERR;
    if (fcntl(timeline->fd, F_GETFD)==-1)
        return QOT_RETURN_TYPE_ERR;

    timeline->binding.demand.resolution = *res;
    // Update the binding
    if(ioctl(timeline->fd, TIMELINE_BIND_UPDATE, &timeline->binding) < 0)
    {
        return QOT_RETURN_TYPE_ERR;
    }
    *res = timeline->binding.demand.resolution;
    return QOT_RETURN_TYPE_OK;
}

qot_return_t timeline_getcoretime(timeline_t *timeline, utimepoint_t *core_now)
{
    if(!timeline)
        return QOT_RETURN_TYPE_ERR;
    if (fcntl(timeline->fd, F_GETFD)==-1)
        return QOT_RETURN_TYPE_ERR;

    // Get the core time
    if(ioctl(timeline->fd, TIMELINE_GET_CORE_TIME_NOW, core_now) < 0)
    {
        return QOT_RETURN_TYPE_ERR;
    }
    return QOT_RETURN_TYPE_OK;
}

qot_return_t timeline_gettime(timeline_t *timeline, utimepoint_t *est) 
{    
    if(!timeline)
        return QOT_RETURN_TYPE_ERR;
    if (fcntl(timeline->fd, F_GETFD)==-1)
        return QOT_RETURN_TYPE_ERR;
    
    // Get the timeline time
    if(ioctl(timeline->fd, TIMELINE_GET_TIME_NOW, est) < 0)
    {
        return QOT_RETURN_TYPE_ERR;
    }
    
    return QOT_RETURN_TYPE_OK;
}

qot_return_t timeline_enable_output_compare(timeline_t *timeline,
    qot_perout_t *request, qot_callback_t callback) {

    if(!timeline)
        return QOT_RETURN_TYPE_ERR;
    if (fcntl(timeline->fd, F_GETFD)==-1)
        return QOT_RETURN_TYPE_ERR;

    request->timeline = timeline->info;
    // Blocking wait on remote timeline time
    if(ioctl(timeline->qotusr_fd, QOTUSR_OUTPUT_COMPARE_ENABLE, request) < 0)
    {
        return QOT_RETURN_TYPE_ERR;
    }

    return QOT_RETURN_TYPE_OK;
}

qot_return_t timeline_disable_output_compare(timeline_t *timeline,
    qot_perout_t *request) {

    if(!timeline)
        return QOT_RETURN_TYPE_ERR;
    if (fcntl(timeline->fd, F_GETFD)==-1)
        return QOT_RETURN_TYPE_ERR;

    request->timeline = timeline->info;
    // Blocking wait on remote timeline time
    if(ioctl(timeline->qotusr_fd, QOTUSR_OUTPUT_COMPARE_DISABLE, request) < 0)
    {
        return QOT_RETURN_TYPE_ERR;
    }

    return QOT_RETURN_TYPE_OK;
}

qot_return_t timeline_config_pin_timestamp(timeline_t *timeline,
    qot_extts_t *request, qot_callback_t callback) {

    return QOT_RETURN_TYPE_OK;
}

qot_return_t timeline_config_events(timeline_t *timeline, uint8_t enable,
    qot_callback_t callback) {
    return QOT_RETURN_TYPE_ERR;
}

qot_return_t timeline_waituntil(timeline_t *timeline, utimepoint_t *utp) 
{
    qot_sleeper_t sleeper;
    if(!timeline)
        return QOT_RETURN_TYPE_ERR;
    if (fcntl(timeline->fd, F_GETFD)==-1)
        return QOT_RETURN_TYPE_ERR;

    sleeper.timeline = timeline->info;
    sleeper.wait_until_time = *utp;

    if(DEBUG)
        printf("Task invoked wait until secs %lld %llu\n", utp->estimate.sec, utp->estimate.asec);
    
    // Blocking wait on remote timeline time
    if(ioctl(timeline->qotusr_fd, QOTUSR_WAIT_UNTIL, &sleeper) < 0)
    {
        return QOT_RETURN_TYPE_ERR;
    }
    *utp = sleeper.wait_until_time;
    return QOT_RETURN_TYPE_OK;
}

qot_return_t timeline_sleep(timeline_t *timeline, utimelength_t *utl) 
{
    qot_sleeper_t sleeper;

    if(!timeline)
        return QOT_RETURN_TYPE_ERR;
    if (fcntl(timeline->fd, F_GETFD)==-1)
        return QOT_RETURN_TYPE_ERR;

    // Get the timeline time
    if(ioctl(timeline->fd, TIMELINE_GET_TIME_NOW, &sleeper.wait_until_time) < 0)
    {
        return QOT_RETURN_TYPE_ERR;
    }

    // Convert timelength to a timepoint
    sleeper.wait_until_time.interval =  utl->interval;
    timepoint_add(&sleeper.wait_until_time.estimate, &utl->estimate);
    
    // Blocking wait on remote timeline time
    if(ioctl(timeline->qotusr_fd, QOTUSR_WAIT_UNTIL, &sleeper) < 0)
    {
        return QOT_RETURN_TYPE_ERR;
    }
    
    return QOT_RETURN_TYPE_OK;
}

qot_return_t timeline_timer_create(timeline_t *timeline, utimepoint_t *start,
    utimelength_t *period, int cnt, qot_callback_t callback, timer_t *timer) {
    return QOT_RETURN_TYPE_ERR;
}

qot_return_t timeline_timer_cancel(timeline_t *timeline, timer_t *timer) {
    return QOT_RETURN_TYPE_ERR;
}

qot_return_t timeline_core2rem(timeline_t *timeline, stimepoint_t *est) 
{    
    if(!timeline)
        return QOT_RETURN_TYPE_ERR;
    if (fcntl(timeline->fd, F_GETFD)==-1)
        return QOT_RETURN_TYPE_ERR;
    
    // Get the timeline time
    if(ioctl(timeline->fd, TIMELINE_CORE_TO_REMOTE, est) < 0)
    {
        return QOT_RETURN_TYPE_ERR;
    }
    
    return QOT_RETURN_TYPE_OK;
}

qot_return_t timeline_rem2core(timeline_t *timeline, timepoint_t *est) 
{    
    if(!timeline)
        return QOT_RETURN_TYPE_ERR;
    if (fcntl(timeline->fd, F_GETFD)==-1)
        return QOT_RETURN_TYPE_ERR;
    
    // Get the timeline time
    if(ioctl(timeline->fd, TIMELINE_REMOTE_TO_CORE, est) < 0)
    {
        return QOT_RETURN_TYPE_ERR;
    }
    
    return QOT_RETURN_TYPE_OK;
}