/*
 * @file qot_core.h
 * @brief Linux 4.1.x kernel module for creation and destruction of QoT timelines
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

#ifndef _QOT_CORE_H_
#define _QOT_CORE_H_

/*
 * struct qot_driver - Callbacks that allow the QoT to core hooks into the hardware
 *                     timers. Implementation is really up to the driver.
 *
 * @compare:            read the current "core" time value in nanoseconds
 * @compare:            action a pin interrupt for the given period
 */
struct qot_clock {
	uint64_t (*read)(void);
	int (*compare)(const char *name, uint8_t enable, 
		uint64_t start, uint32_t high, uint32_t low, uint64_t repeat);
};	

// Register a given clocksource as the primary driver for time
int qot_register(struct qot_driver *driver);

// Send a compare action to the QoT stack (from the driver)
int qot_push_capture(const char *name, int64_t epoch);

// Unregister the  clock source, oscillators and pins
int qot_unregister(void);

#endif