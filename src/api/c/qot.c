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
#include <poll.h>

#include <linux/ptp_clock.h>

/* Header for function definitions */
#include "qot.h"

#ifdef PARAVIRT_GUEST
/* Header for guest to host virtserial-based communication for a PV QEMU-KVM guest */
#include "../../virt/qot_virtguest.h"
/* Header for file host to guest ivshmem-based communication for a PV QEMU-KVM guest */
#include "../../virt/pci_mmio/qot_pci_ivshmem.h"
#endif

#define DEBUG 1

/* Timeline implementation */
typedef struct timeline {
    qot_timeline_t info;                  /* Basic timeline information               */
    qot_binding_t binding;                /* Basic binding info                       */
    int fd;                               /* File descriptor to /dev/timelineX ioctl  */
    int qotusr_fd;                        /* File descriptor to /dev/qotusr ioctl     */
    int clock_fd;                         /* File Descriptor to /dev/ptpY             */
    qot_callback_t event_callback;        /* Event Callback Function                  */
    #ifdef PARAVIRT_GUEST
    qot_timeline_t virt_info;             /* Virtual (host) timeline information      */
    int pci_dataregion;                   /* PCI IVSHMEM data region                  */
    tl_clockparams_t *timeline_clock;     /* Pointer to timeline clock parameters     */
    #endif
} timeline_t;

/* Is the given timeline a valid one */
qot_return_t timeline_check_fd(timeline_t *timeline) {
    if (fcntl(timeline->fd, F_GETFD)==-1)
        return QOT_RETURN_TYPE_ERR;
    return QOT_RETURN_TYPE_OK;
}

/* Create a Timeline Data Structure */
timeline_t *timeline_t_create()
{
    timeline_t *timeline;
    timeline = (timeline_t*) malloc(sizeof(struct timeline));
    return timeline;
}

/* Destroy a Timeline Data Structure */
void timeline_t_destroy(timeline_t *timeline)
{
    free(timeline);
}

/* Bind to a timeline */
qot_return_t timeline_bind(timeline_t *timeline, const char *uuid, const char *name, timelength_t res, timeinterval_t acc) 
{
    char qot_timeline_filename[15];
    int usr_file;
    char *gl_start;

    // Check to make sure the UUID and Name is valid
    if (uuid == NULL || name == NULL)
        return QOT_RETURN_TYPE_ERR;

    if (strlen(uuid) > QOT_MAX_NAMELEN)
        return QOT_RETURN_TYPE_ERR;

    // Check to find whether the timeline type is global (first three characters should be "gl_")
    gl_start = strstr(uuid, GLOBAL_TL_STRING);
    if(gl_start == uuid)
        timeline->info.type = QOT_TIMELINE_GLOBAL;
    else
        timeline->info.type = QOT_TIMELINE_LOCAL; // Default to Local

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
        // If it exists try to get information
        if(ioctl(timeline->qotusr_fd, QOTUSR_GET_TIMELINE_INFO, &timeline->info) < 0)
        {
            return QOT_RETURN_TYPE_ERR;
        }    
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
    TL_FROM_SEC(timeline->binding.period, 0);
    TP_FROM_SEC(timeline->binding.start_offset, 0);
    
    if (DEBUG) 
        printf("Binding to timeline %s\n", uuid);
    // Bind to the timeline
    if(ioctl(timeline->fd, TIMELINE_BIND_JOIN, &timeline->binding) < 0)
    {
        return QOT_RETURN_TYPE_ERR;
    }
    if (DEBUG) 
        printf("Bound to timeline %s\n", uuid);

    // Virtualization-specific Guest extensions -> Send TIMELINE_CREATE Message
    #ifdef PARAVIRT_GUEST
    // Populate the timeline+binding metadata
    qot_virtmsg_t virt_msg;
    virt_msg.info = timeline->info;
    virt_msg.msgtype = TIMELINE_CREATE;
    virt_msg.demand = timeline->binding.demand;
    virt_msg.retval = QOT_RETURN_TYPE_ERR;
    if (DEBUG) 
        printf("Sending timeline metadata to host\n");
    if(send_message(&virt_msg) == QOT_RETURN_TYPE_ERR)
    {
        if (DEBUG) 
            printf("Failed to send timeline metadata to host\n");
        return QOT_RETURN_TYPE_ERR;
    }
    else
    {
        // Add error handling -> retry
        timeline->virt_info = virt_msg.info;
        if (DEBUG) 
            printf("Host replied with %d retval, host timeline id is %d\n",virt_msg.retval, virt_msg.info.index);
    }
    // Setup Memory mapping to read timeline clock parameters (mapping, uncertainty)
    timeline->pci_dataregion = setup_pci_mmio();
    if (timeline->pci_dataregion < 0)
    {
        if (DEBUG) 
            printf("Failed to open PCI IVSHMEM region\n");
        return QOT_RETURN_TYPE_ERR;
    } 
    // Get pointer to memory region corresponding to timeline clock parameters
    __u32 offset = timeline->virt_info.index*sizeof(tl_clockparams_t);
    timeline->timeline_clock = read_timeline_clock_parameters(timeline->pci_dataregion, offset);
    if (timeline->timeline_clock == NULL)
    {
        if (DEBUG) 
            printf("Failed to get a pointer to PCI IVSHMEM region\n");
        return QOT_RETURN_TYPE_ERR;
    } 
    #endif

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

    // Virtualization-specific Guest extensions -> Send TIMELINE_DESTROY Message
    #ifdef PARAVIRT_GUEST
    // Populate the timeline+binding metadata
    qot_virtmsg_t virt_msg;
    virt_msg.info = timeline->virt_info;
    virt_msg.msgtype = TIMELINE_DESTROY;
    virt_msg.demand = timeline->binding.demand;
    virt_msg.retval = QOT_RETURN_TYPE_ERR;
    if (DEBUG) 
        printf("Sending timeline metadata to host\n");
    if(send_message(&virt_msg) == QOT_RETURN_TYPE_ERR)
    {
        if (DEBUG) 
            printf("Failed to send timeline metadata to host\n");
        return QOT_RETURN_TYPE_ERR;
    }
    else
    {
        // Add error handling -> retry
        printf("Host replied with retval %d\n",virt_msg.retval);
    }
    #endif

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
    // Virtualization-specific Guest extensions -> Send TIMELINE_UPDATE Message
    #ifdef PARAVIRT_GUEST
    // Populate the timeline+binding metadata
    qot_virtmsg_t virt_msg;
    virt_msg.info = timeline->virt_info;
    virt_msg.msgtype = TIMELINE_UPDATE;
    virt_msg.demand = timeline->binding.demand;
    virt_msg.retval = QOT_RETURN_TYPE_ERR;
    if (DEBUG) 
        printf("Sending updated timeline metadata to host\n");
    if(send_message(&virt_msg) == QOT_RETURN_TYPE_ERR)
    {
        if (DEBUG) 
            printf("Failed to send timeline metadata to host\n");
        return QOT_RETURN_TYPE_ERR;
    }
    else
    {
        // Add error handling -> retry
        printf("Host replied with retval %d\n",virt_msg.retval);
    }
    #endif
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
    // Virtualization-specific Guest extensions -> Send TIMELINE_UPDATE Message
    #ifdef PARAVIRT_GUEST
    // Populate the timeline+binding metadata
    qot_virtmsg_t virt_msg;
    virt_msg.info = timeline->virt_info;
    virt_msg.msgtype = TIMELINE_UPDATE;
    virt_msg.demand = timeline->binding.demand;
    virt_msg.retval = QOT_RETURN_TYPE_ERR;
    if (DEBUG) 
        printf("Sending updated timeline metadata to host\n");
    if(send_message(&virt_msg) == QOT_RETURN_TYPE_ERR)
    {
        if (DEBUG) 
            printf("Failed to send timeline metadata to host\n");
        return QOT_RETURN_TYPE_ERR;
    }
    else
    {
        // Add error handling -> retry
        printf("Host replied with retval %d\n",virt_msg.retval);
    }
    #endif
    *res = timeline->binding.demand.resolution;
    return QOT_RETURN_TYPE_OK;
}

qot_return_t timeline_set_schedparams(timeline_t *timeline, timelength_t *period, timepoint_t *start_offset) 
{
    if(!timeline)
        return QOT_RETURN_TYPE_ERR;
    if (fcntl(timeline->fd, F_GETFD)==-1)
        return QOT_RETURN_TYPE_ERR;

    timeline->binding.start_offset = *start_offset;
    timeline->binding.period = *period;
    // Update the binding
    if(ioctl(timeline->fd, TIMELINE_BIND_UPDATE, &timeline->binding) < 0)
    {
        return QOT_RETURN_TYPE_ERR;
    }
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

#ifdef PARAVIRT_GUEST
// BASIC TIME PROJECTION FUNCTIONS /////////////////////////////////////////////
qot_return_t qot_loc2rem(timeline_t *timeline, utimepoint_t *est, int period)
{    
    int64_t val;
    if(!timeline)
        return QOT_RETURN_TYPE_ERR;

    val = TP_TO_nSEC(est->estimate);

    // Check if this is correct -> makes the assumption that val is mostly greater than 1s (1 billion ns) (consider using floating point ops)
    if (period)
        val += (timeline->timeline_clock->translation.mult*val)/1000000000L;
    else
    {
        val -= timeline->timeline_clock->translation.last;
        val  = timeline->timeline_clock->translation.nsec + val + ((timeline->timeline_clock->translation.mult*val)/1000000000L);
    }
    TP_FROM_nSEC(est->estimate, val); 
    return QOT_RETURN_TYPE_OK;
}

qot_return_t qot_rem2loc(timeline_t *timeline, utimepoint_t *est, int period)
{
    int64_t val;
    int64_t rem;

    if(!timeline || !timeline->timeline_clock)
        return QOT_RETURN_TYPE_ERR;
    val = TP_TO_nSEC(est->estimate);

    if (period)
    {
        val = (int64_t)floor(((double)val/(double)(timeline->timeline_clock->translation.mult + 1000000000LL))*1000000000LL);
    }
    else
    {
        int64_t diff = (val - timeline->timeline_clock->translation.nsec);
        int64_t quot = (int64_t)floor(((double)diff/(double)(timeline->timeline_clock->translation.mult + 1000000000LL))*1000000000LL);
        val = timeline->timeline_clock->translation.last + quot; 
    }
    TP_FROM_nSEC(est->estimate, val); 
    return QOT_RETURN_TYPE_OK;
}

qot_return_t timeline_getvtime(timeline_t *timeline, utimepoint_t *est)
{
    // This function should be populated (use timeline->timeline_clock to translate from core time): URGENT
    qot_return_t retval;
    timeline_getcoretime(timeline, est);
    if (DEBUG)
    {
    	printf("reading time using ivshmem\n");
    	printf("Timeline Parameters are mult:%lld last:%lld\n", 
    		timeline->timeline_clock->translation.mult, 
    		timeline->timeline_clock->translation.last);
    }
    retval = qot_loc2rem(timeline, est, 0);
    return retval;
}
#endif

qot_return_t timeline_gettime(timeline_t *timeline, utimepoint_t *est) 
{    
    if(!timeline)
        return QOT_RETURN_TYPE_ERR;
    if (fcntl(timeline->fd, F_GETFD)==-1)
        return QOT_RETURN_TYPE_ERR;

    #ifdef PARAVIRT_GUEST
    // Virtualization-specific Guest extensions -> Get Timeline Clock Time
    return timeline_getvtime(timeline, est);
    #else
    // Get the timeline time
    if(ioctl(timeline->fd, TIMELINE_GET_TIME_NOW, est) < 0)
    {
        return QOT_RETURN_TYPE_ERR;
    }
    #endif
    
    return QOT_RETURN_TYPE_OK;
}

qot_return_t timeline_enable_output_compare(timeline_t *timeline,
    qot_perout_t *request) {

    if(!timeline)
        return QOT_RETURN_TYPE_ERR;
    if (fcntl(timeline->fd, F_GETFD)==-1)
        return QOT_RETURN_TYPE_ERR;
    if(request->duty_cycle >= 100)
        return QOT_RETURN_TYPE_ERR;
    if(request->edge != QOT_TRIGGER_RISING && request->edge != QOT_TRIGGER_FALLING)
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

qot_return_t timeline_config_pin_timestamp(timeline_t *timeline, qot_extts_t *request, int enable) 
{
    qot_clock_t core_clock;
    // char ptp_device[QOT_MAX_NAMELEN];           /* File Name              */
    // int fd;                                     /* device file descriptor */
    // int index = 1;                              /* Channel index, 'corresponding to the pin */
    // struct ptp_clock_caps caps;                 /* Clock capabilities */
    // struct ptp_pin_desc desc;                   /* Pin configuration */
    // struct ptp_extts_request extts_request;     /* External timestamp req */


    // if(!timeline)
    //     return QOT_RETURN_TYPE_ERR;
    // if (fcntl(timeline->fd, F_GETFD)==-1)
    //     return QOT_RETURN_TYPE_ERR;
    
    // // Get the presiding core clock
    // if(ioctl(timeline->qotusr_fd, QOTUSR_GET_CORE_CLOCK_INFO, &core_clock) < 0)
    // {
    //     return QOT_RETURN_TYPE_ERR;
    // }

    // // Create the file name
    // sprintf(ptp_device, "/dev/ptp%d", core_clock.phc_id);

    // if(enable)
    // {
    //     /* Open the core clock ptp character device */
    //     fd = open(ptp_device, O_RDWR);
    //     if (fd < 0) {
    //         printf("Failed to open ptp device %s\n", ptp_device);
    //         return QOT_RETURN_TYPE_ERR;
    //     }
    //     printf("Opened ptp device %s with fd = %d\n", ptp_device, fd);
    //     timeline->clock_fd = fd;

    //     /* Pin to perform external time stamping */
    //     index = request->pin_index;

    //     /* Check if clock supports external timestamps */
    //     if (ioctl(fd, PTP_CLOCK_GETCAPS, &caps)) {
    //         printf("cannot get capabilities of clock");
    //         return QOT_RETURN_TYPE_ERR;
    //     }
    //     if(caps.n_ext_ts < 1)
    //     {
    //         printf("clock cannot perform external timestamping\n");
    //         return QOT_RETURN_TYPE_ERR;
    //     }

    //     /* Configure the pin to support external timestamps */
    //     memset(&desc, 0, sizeof(desc));
    //     desc.index = index;
    //     desc.func = 1;                              /* '1' corresponds to external timestamp */
    //     desc.chan = index;
    //     if (ioctl(fd, PTP_PIN_SETFUNC, &desc)) {
    //         printf("Set pin func failed for\n");
    //         return QOT_RETURN_TYPE_ERR;
    //     }
        
    //     /* Request timestamps from the pin */ 
    //     memset(&extts_request, 0, sizeof(extts_request));
    //     extts_request.index = index;
    //     extts_request.flags = PTP_ENABLE_FEATURE;
    //     if (ioctl(fd, PTP_EXTTS_REQUEST, &extts_request)) {
    //         printf("Requesting timestamps failed for\n");
    //         return QOT_RETURN_TYPE_ERR;
    //     }
    // }
    // else
    // {
    //     /* Disable the pin */
    //     if (fcntl(timeline->clock_fd, F_GETFD)==-1)
    //         return QOT_RETURN_TYPE_ERR;
    //     memset(&desc, 0, sizeof(desc));
    //     desc.index = request->pin_index;
    //     desc.func = 0;              // '0' corresponds to no function 
    //     desc.chan = request->pin_index;
    //     if (ioctl(timeline->clock_fd, PTP_PIN_SETFUNC, &desc)) {
    //         perror("PTP_PIN_SETFUNC Disable");
    //     }
        
    //     /* Close the character device */
    //     close(timeline->clock_fd);
    // }
    return QOT_RETURN_TYPE_OK;
}

qot_return_t timeline_read_pin_timestamps(timeline_t *timeline, qot_event_t *event) 
{
    int cnt;
    struct ptp_extts_event event_ptp;               /* PTP event */
    if(!timeline)
        return QOT_RETURN_TYPE_ERR;
    if (fcntl(timeline->fd, F_GETFD)==-1)
        return QOT_RETURN_TYPE_ERR;
    if (fcntl(timeline->clock_fd, F_GETFD)==-1)
        return QOT_RETURN_TYPE_ERR;

    cnt = read(timeline->clock_fd, &event_ptp, sizeof(event_ptp));
    if (cnt != sizeof(event_ptp)) {
        printf("Cannot read event");
    }
    event->type = QOT_EVENT_EXTERNAL_TIMESTAMP;
    event->timestamp.estimate.sec  = event_ptp.t.sec;
    event->timestamp.estimate.asec = event_ptp.t.nsec*nSEC_PER_SEC;
    
    // Convert the core time to remote time
    if(ioctl(timeline->fd, TIMELINE_CORE_TO_REMOTE, &event->timestamp.estimate) < 0)
    {
        return QOT_RETURN_TYPE_ERR;
    }
    return QOT_RETURN_TYPE_OK;
}

qot_return_t timeline_config_events(timeline_t *timeline, uint8_t enable,
    qot_callback_t callback) {
    return QOT_RETURN_TYPE_ERR;
}

qot_return_t timeline_read_events(timeline_t *timeline, qot_event_t *event)
{
    struct pollfd fds;

    if(!timeline)
        return QOT_RETURN_TYPE_ERR;
    if (fcntl(timeline->fd, F_GETFD)==-1)
        return QOT_RETURN_TYPE_ERR;

    fds.fd = timeline->qotusr_fd;
    fds.events = POLLIN;

    if(poll(&fds, 1, -1) <= 0)
        return QOT_RETURN_TYPE_ERR;

    if(fds.revents && POLLIN == POLLIN)
    {
        if(ioctl(timeline->qotusr_fd, QOTUSR_GET_NEXT_EVENT, event) < 0)
        {
            return QOT_RETURN_TYPE_ERR;
        }
    }
    return QOT_RETURN_TYPE_OK;
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
        printf("Task invoked wait until secs %lld %llu\n", sleeper.wait_until_time.estimate.sec, sleeper.wait_until_time.estimate.asec);

    #ifdef PARAVIRT_GUEST
    // Virtualization-specific Guest extensions -> Convert time to local core time
    if (qot_rem2loc(timeline, &sleeper.wait_until_time, 0) == QOT_RETURN_TYPE_ERR)
    	return QOT_RETURN_TYPE_ERR;
    #endif

    
    
    // Blocking wait on remote timeline time
    if(ioctl(timeline->qotusr_fd, QOTUSR_WAIT_UNTIL, &sleeper) < 0)
    {
       return QOT_RETURN_TYPE_ERR;
    }
    *utp = sleeper.wait_until_time;
    return QOT_RETURN_TYPE_OK;
}

qot_return_t timeline_waituntil_nextperiod(timeline_t *timeline, utimepoint_t *utp) 
{
    qot_sleeper_t sleeper;
    timelength_t elapsed_time;
    timepoint_t wakeup_time;
    u64 elapsed_ns = 0;
    u64 period_ns = 0;
    u64 num_periods = 0;
    if(!timeline)
        return QOT_RETURN_TYPE_ERR;
    if (fcntl(timeline->fd, F_GETFD)==-1)
        return QOT_RETURN_TYPE_ERR;

    sleeper.timeline = timeline->info;
    // Get the timeline time
    if(ioctl(timeline->fd, TIMELINE_GET_TIME_NOW, &sleeper.wait_until_time) < 0)
    {
        return QOT_RETURN_TYPE_ERR;
    }
    // Check Start Offset
    if(timepoint_cmp(&timeline->binding.start_offset, &sleeper.wait_until_time.estimate) < 0)
    {
        sleeper.wait_until_time.estimate = timeline->binding.start_offset;
    }
    else 
    {
        // Calculate Next Wakeup Time
        timepoint_diff(&elapsed_time, &sleeper.wait_until_time.estimate, &timeline->binding.start_offset);
        elapsed_ns = TL_TO_nSEC(elapsed_time);
        period_ns = TL_TO_nSEC(timeline->binding.period);
        num_periods = (elapsed_ns/period_ns);
        if(elapsed_ns % period_ns != 0)
            num_periods++;
        elapsed_ns = period_ns*num_periods;
        TL_FROM_nSEC(elapsed_time, elapsed_ns);
        wakeup_time = timeline->binding.start_offset;
        timepoint_add(&wakeup_time, &elapsed_time);
        sleeper.wait_until_time.estimate = wakeup_time;
     }

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

qot_return_t timeline_timer_create(timeline_t *timeline, qot_timer_t *timer, qot_timer_callback_t callback) 
{
    struct sigaction act;

    if(!timeline || !timer)
        return QOT_RETURN_TYPE_ERR;
    if (fcntl(timeline->fd, F_GETFD)==-1)
        return QOT_RETURN_TYPE_ERR;

    // Create a timer
    if(ioctl(timeline->fd, TIMELINE_CREATE_TIMER, timer) < 0)
    {
        printf("Failed To Create Timer\n");
        return QOT_RETURN_TYPE_ERR;
    }

    memset(&act, 0, sizeof(struct sigaction));
    sigemptyset(&act.sa_mask);

    act.sa_sigaction = callback;
    act.sa_flags = SA_SIGINFO;

    if (sigaction(SIGALRM, &act, NULL) == -1)
    {
        printf("sigaction failed !\n");
        return QOT_RETURN_TYPE_ERR;
    }

    return QOT_RETURN_TYPE_OK;
    
}

qot_return_t timeline_timer_cancel(timeline_t *timeline, qot_timer_t *timer) 
{
    if(!timeline || !timer)
        return QOT_RETURN_TYPE_ERR;
    if (fcntl(timeline->fd, F_GETFD)==-1)
        return QOT_RETURN_TYPE_ERR;

    // Create a timer
    if(ioctl(timeline->fd, TIMELINE_DESTROY_TIMER, timer) < 0)
    {
        return QOT_RETURN_TYPE_ERR;
    }

    return QOT_RETURN_TYPE_OK;
}

qot_return_t timeline_core2rem(timeline_t *timeline, stimepoint_t *est) 
{    
    if(!timeline)
        return QOT_RETURN_TYPE_ERR;
    if (fcntl(timeline->fd, F_GETFD)==-1)
        return QOT_RETURN_TYPE_ERR;
    
    #ifdef PARAVIRT_GUEST
    qot_return_t retval;
    utimepoint_t utp;
    utp.estimate = est->estimate;
    retval = qot_loc2rem(timeline, &utp, 0);
    (*est).estimate = utp.estimate; 
    return retval;
    #else
    // Get the timeline time
    if(ioctl(timeline->fd, TIMELINE_CORE_TO_REMOTE, est) < 0)
    {
        return QOT_RETURN_TYPE_ERR;
    }
    return QOT_RETURN_TYPE_OK;
    #endif
}

qot_return_t timeline_rem2core(timeline_t *timeline, timepoint_t *est) 
{    
    if(!timeline)
        return QOT_RETURN_TYPE_ERR;
    if (fcntl(timeline->fd, F_GETFD)==-1)
        return QOT_RETURN_TYPE_ERR;
    
    #ifdef PARAVIRT_GUEST
    qot_return_t retval;
    utimepoint_t utp;
    utp.estimate = *est;
    retval = qot_rem2loc(timeline, &utp, 0);
    *est = utp.estimate; 
    return retval;
    #else
    // Get the timeline time
    if(ioctl(timeline->fd, TIMELINE_REMOTE_TO_CORE, est) < 0)
    {
        return QOT_RETURN_TYPE_ERR;
    }
    return QOT_RETURN_TYPE_OK;
    #endif
}
