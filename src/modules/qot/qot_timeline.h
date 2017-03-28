/*
 * @file qot_timeline.h
 * @brief Interface to timeline management in the QoT stack
 * @author Andrew Symington and Sandeep D'souza
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

#ifndef QOT_STACK_SRC_MODULES_QOT_QOT_TIMELINE_H
#define QOT_STACK_SRC_MODULES_QOT_QOT_TIMELINE_H

#include <linux/spinlock.h>
#include <linux/rbtree.h>

#include "qot_core.h"

/* qot_timeline.h */

/* Timeline subsystem spin lock */
extern raw_spinlock_t qot_timeline_lock;

/* All the functions below assume the argument pointer has been allocated */

void qot_timeline_event_lock(qot_timeline_t *timeline, unsigned long *flags);

void qot_timeline_event_unlock(qot_timeline_t *timeline, unsigned long *flags);

struct rb_root *qot_timeline_eventhead(qot_timeline_t *timeline);

/**
 * @brief Find the first timeline in the system.
 * @param timeline A pointer to a timeline
 * @return A status code indicating success (0) or other (no more timelines)
 **/
qot_return_t qot_timeline_first(qot_timeline_t **timeline);

/**
 * @brief Find the next timeline in the system based on the specified name.
 * @param timeline A pointer to a timeline with the name field populated
 * @return A status code indicating success (0) or other (no more timelines)
 **/
qot_return_t qot_timeline_next(qot_timeline_t **timeline);

/**
 * @brief Get information about a timeline  based on the specified name.
 * @param timeline A pointer to a pointer to a timeline with the name field populated
 * @return A status code indicating success (0) or other (no more timelines)
 **/
qot_return_t qot_timeline_get_info(qot_timeline_t **timeline);

/**
 * @brief Create a timeline based on the specified name.
 * @param timeline A pointer to a timeline with the name field populated
 * @return A status code indicating success (0) or other (no more timelines)
 **/
qot_return_t qot_timeline_create(qot_timeline_t *timeline);

/**
 * @brief Remove a timeline based on the specified name.
 * @param timeline A pointer to a timeline with the name field populated
 * @return A status code indicating success (0) or other (no more timelines)
 **/
qot_return_t qot_timeline_remove(qot_timeline_t *timeline, bool admin_flag);


/**
 * @brief Remove all timelines
 **/
void qot_timeline_remove_all(void);

/**
 * @brief Clean up the timer subsystem
 * @param qot_class Device class for all QoT devices
 **/
void qot_timeline_cleanup(struct class *qot_class);

/**
 * @brief Initialize the timer subsystem
 * @param qot_class Device class for all QoT devices
 * @return A status code indicating success (0) or failure (!0)
 **/
qot_return_t qot_timeline_init(struct class *qot_class);

/* qot_timeline_chdev.h */

/**
 * @brief Register a new /dev/timelineX character device for the timeline
 * @param timeline Information about the timeline
 * @return < 0 failure, 0+ the IDR index of the new timeline
 **/
int qot_timeline_chdev_register(qot_timeline_t *timeline);

/**
 * @brief Remove an existing /dev/timelineX character device
 * @param timeline The timeline to register
 * @return A status code indicating success (0) or failure (!0)
 **/
qot_return_t qot_timeline_chdev_unregister(int index, bool admin_flag);

/**
 * @brief Clean up the timeline character device subsystem
 * @param qot_class Device class for all QoT devices
 **/
void qot_timeline_chdev_cleanup(struct class *qot_class);

/**
 * @brief Initialize the timeline character device subsystem
 * @param qot_class Device class for all QoT devices
 * @return A status code indicating success (0) or failure (!0)
 **/
qot_return_t qot_timeline_chdev_init(struct class *qot_class);

/**
 * @brief Find a timeline's name based on an index
 * @param timeline index and pointer to a name character string
 **/
qot_return_t qot_get_timeline_name(int index, char *name);

/**
 * @brief Find a timeline's resolution based on an index
 * @param timeline index and pointer to a resolution timelength
 **/
qot_return_t qot_get_timeline_resolution(int index, timelength_t *resolution);

/**
 * @brief Find a timeline's accuracy based on an index
 * @param timeline index and pointer to a accuracy timeinterval
 **/
qot_return_t qot_get_timeline_accuracy(int index, timeinterval_t *accuracy);

/* qot_timeline_sysfs.h */

/**
 * @brief Clean up the timeline sysfs device subsystem
 * @param qot_class Device class for all QoT devices
 **/
void qot_timeline_sysfs_cleanup(struct device *qot_device);

/**
 * @brief Initialize the timeline sysfs device subsystem
 * @param qot_class Device class for all QoT devices
 * @return A status code indicating success (0) or failure (!0)
 **/
qot_return_t qot_timeline_sysfs_init(struct device *qot_device);

// BASIC TIME PROJECTION FUNCTIONS /////////////////////////////////////////////

/**
 * @brief Convert Local Core time to remote timeline time
 * @param index Timeline index
 * @param period Period ?
 * @param val Pointer to a time value in ns 
 * @return A status code indicating success (0) or failure (!0)
 **/
qot_return_t qot_loc2rem(int index, int period, s64 *val);

/**
 * @brief Convert Remote Timeline time to core time
 * @param index Timeline index
 * @param period Period ?
 * @param val Pointer to a time value in ns 
 * @return A status code indicating success (0) or failure (!0)
 **/
qot_return_t qot_rem2loc(int index, int period, s64 *val);

/**
 * @brief Helper Function for qot_scheduler to make the timer field of a binding NULL
 * @param task pointer to a task struct
 * @param timeline pointer to a timeline
 * @return A status code indicating success (0) or failure (!0)
 **/
qot_return_t qot_remove_binding_timer(int pid, qot_timeline_t *timeline);

#endif