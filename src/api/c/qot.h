/*
 * @file qot.h
 * @brief A simple C application programming interface to the QoT stack
 * @author Andrew Symington
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

#ifndef QOT_STACK_SRC_API_C_QOT_H
#define QOT_STACK_SRC_API_C_QOT_H

/* Opaque type */
typedef struct timeline timeline_t;

/* Function callbacks */
typedef void (*qot_callback_t)(const qot_event_t *evt);

/* Timer ids */
typedef int qot_timer_t;

/**
 * @brief Bind to a timeline with a given resolution and accuracy
 * @param timeline Pointer to a timeline struct
 * @param uuid Name of the timeline
 * @param name Name of this binding
 * @param res Maximum tolerable unit of time
 * @param acc Maximum tolerable deviation from true time
 * @return A status code indicating success (0) or other
 **/
qot_response_t timeline_bind(timeline_t *timeline, const char *uuid,
    const char *name, timelength_t res, timeinterval_t acc);

/**
 * @brief Unbind from a timeline
 * @param timeline Pointer to a timeline struct
 * @return A status code indicating success (0) or other
 **/
qot_response_t timeline_unbind(timeline_t *timeline);

/**
 * @brief Get the accuracy requirement associated with this binding
 * @param timeline Pointer to a timeline struct
 * @param acc Maximum tolerable deviation from true time
 * @return A status code indicating success (0) or other
 **/
qot_response_t timeline_get_accuracy(timeline_t *timeline, timeinterval_t *acc);

/**
 * @brief Get the resolution requirement associated with this binding
 * @param timeline Pointer to a timeline struct
 * @param res Maximum tolerable unit of time
 * @return A status code indicating success (0) or other
 **/
qot_response_t timeline_get_resolution(timeline_t *timeline, timelength_t *res);

/**
 * @brief Query the name of this application
 * @param timeline Pointer to a timeline struct
 * @param name Pointer to the where the name will be written
 * @return A status code indicating success (0) or other
 **/
qot_response_t timeline_get_name(timeline_t *timeline, const char *name);

/**
 * @brief Query the timeline's UUID
 * @param timeline Pointer to a timeline struct
 * @param uuid Name of the timeline
 * @return A status code indicating success (0) or other
 **/
qot_response_t timeline_get_uuid(timeline_t *timeline, const char *uuid);


/**
 * @brief Set the accuracy requirement associated with this binding
 * @param timeline Pointer to a timeline struct
 * @param acc Maximum tolerable deviation from true time
 * @return A status code indicating success (0) or other
 **/
qot_response_t timeline_set_accuracy(timeline_t *timeline, timeinterval_t *acc);

/**
 * @brief Set the resolution requirement associated with this binding
 * @param timeline Pointer to a timeline struct
 * @param res Maximum tolerable unit of time
 * @return A status code indicating success (0) or other
 **/
qot_response_t timeline_set_resolution(timeline_t *timeline, timelength_t *res);

/**
 * @brief Query the time according to the timeline
 * @param timeline Pointer to a timeline struct
 * @param est Estimated time
 * @return A status code indicating success (0) or other
 **/
qot_response_t timeline_gettime(timeline_t *timeline, utimepoint_t *est);

/**
 * @brief Request an interrupt be generated on a given pin
 * @param timeline Pointer to a timeline struct
 * @param request Pointer to interrupt configuration
 * @param callback A function to call on each edge event
 * @return A status code indicating success (0) or other
 **/
qot_response_t timeline_config_pin_interrupt(timeline_t *timeline,
    qot_perout_t *request, qot_callback_t callback);

/**
 * @brief Request an interrupt be generated on a given pin
 * @param timeline Pointer to a timeline struct
 * @param request Pointer to timestamp configuration
 * @param callback A function to call on each edge event
 * @return A status code indicating success (0) or other
 **/
qot_response_t timeline_config_pin_timestamp(timeline_t *timeline,
    qot_extts_t *request, qot_callback_t callback);
​
/**
 * @brief Request to be informed of timeline events
 * @param timeline Pointer to a timeline struct
 * @param enable Enable or disable callback for the events
 * @param callback The function that will be called
 * @return A status code indicating success (0) or other
 **/
qot_response_t timeline_config_events(timeline_t *timeline, uint8_t enable,
    qot_callback_t callback);

/**
 * @brief Block wait until a specified uncertain point
 * @param timeline Pointer to a timeline struct
 * @param utp The time point at which to resume. This will be modified by the
 *            function to reflect the predicted time of resume
 * @return A status code indicating success (0) or other
 **/
qot_response_t timeline_waituntil(timeline_t *timeline, utimepoint_t *utp);

/**
 * @brief Block for a specified length of uncertain time
 * @param timeline Pointer to a timeline struct
 * @param utl The period for blocking. This will be modified by the
 *             function to reflect the estimated time of blocking
 * @return A status code indicating success (0) or other
 **/
qot_response_t timeline_sleep(timeline_t *timeline, utimelength_t *utl);

/**
 * @brief Non-blocking call to create a timer
 * @param timeline Pointer to a timeline struct
 * @param start The start time for the timer
 * @param period The period between calls
 * @param cnt The number of iterations called back
 * @param callback The function that will be called
 * @param timer A pointer to a timer object
 * @return A status code indicating success (0) or other
 **/
qot_response_t timeline_timer_create(timeline_t *timeline, utimepoint_t *start,
    utimelength_t *period, int cnt, qot_callback_t callback, timer_t *timer);

/**
 * @brief Non-blocking call to cancel a timer
 * @param timeline Pointer to a timeline struct
 * @param timer A pointer to a timer object
 * @return A status code indicating success (0) or other
 **/
qot_response_t timeline_timer_cancel(timeline_t *timeline, timer_t *timer);

#endif