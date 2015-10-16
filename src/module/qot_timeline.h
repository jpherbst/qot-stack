/*
 * @file qot_timelines.h
 * @brief Linux 4.1.6 kernel module for creation and destruction of QoT timelines
 * @author Fatima Anwar 
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

// Bind to a clock called UUID with a given accuracy and resolution
int32_t qot_timeline_bind(const char *uuid, uint64_t acc, uint64_t res);

// Get the POSIX clock index for a given binding ID
int32_t qot_timeline_index(int bid);

// Unbind from a timeline
int32_t qot_timeline_unbind(int bid);

// Update the accuracy of a binding
int32_t qot_timeline_set_accuracy(int bid, uint64_t acc);

// Update the resolution of a binding
int32_t qot_timeline_set_resolution(int bid, uint64_t res);

// Initialize the timeline engine
int32_t qot_timeline_init(void);

// Clean up the timeline engine
void qot_timeline_cleanup(void);

#endif