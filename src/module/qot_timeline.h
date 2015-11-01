/*
 * @file qot_timelines.h
 * @brief Linux 4.1.6 kernel module for creation and destruction of QoT timelines
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

#ifndef _QOT_TIMELINE_H_
#define _QOT_TIMELINE_H_

// BASIC FUNCTIONS

// Bind to a clock called UUID with a given accuracy and resolution
int qot_timeline_bind(const char *uuid, uint64_t acc, uint64_t res);

// Get the POSIX clock index for a given binding ID
int qot_timeline_index(int bid);

// Get the timeline UUID for a given binding ID
const char* qot_timeline_uuid(int bid);

// Unbind from a timeline
int qot_timeline_unbind(int bid);

// CLOCK CONFIGURATION

// Update the accuracy of a binding
int qot_timeline_set_accuracy(int bid, uint64_t acc);

// Update the resolution of a binding
int qot_timeline_set_resolution(int bid, uint64_t res);

// COORDINATE TRANSFORMATIONS

// Convert a core time (ns) to a timeline time (ns)
int qot_timeline_time_core2line(int bid, int64_t *core);

// Convert a timeline time (ns) to a core time (ns)
int qot_timeline_time_line2core(int bid, int64_t *line);

// Convert a core duration (ns) to a timeline duration (ns)
int qot_timeline_duration_core2line(int bid, uint64_t *core);

// Convert a timeline duration (ns) to a core duration (ns)
int qot_timeline_duration_line2core(int bid, uint64_t *line);

// BASIC INITIALIZATION

// Initialize the timeline engine
int qot_timeline_init(void);

// Clean up the timeline engine
void qot_timeline_cleanup(void);

#endif