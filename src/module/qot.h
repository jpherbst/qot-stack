/*
 * @file qot.h
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

#ifndef _QOT_H_
#define _QOT_H_

// Basic properties of an oscillator (provided by device)
struct qot_properties {
	uint64_t frequency;						// Fundamental output frequency
	uint64_t short_term_stability;			// Short term stability
	uint64_t phase_noise;					// Phase noise
	uint64_t aging;							// Aging in parts per billion
	uint64_t temperature_stability;			// Temperature stability
	uint64_t power_consumption;				// Power consumption
	uint64_t warm_up_time;					// Warm up time
	int (*power_on)(void);					// Turn on an oscillator
	int (*power_off)(void);					// Turn off this oscillator
	int (*enable)(void);					// Switch to this oscillator
	struct list_head list;					// List...
};

// Information about a capture event
struct qot_capture_event {
	uint32_t id;							// Hardware timer ID
	uint64_t count_at_capture;				// Hardware count at capture
	uint64_t count_at_interrupt;			// Hardware count at INTERRUPTION
};

// Information about a compare event
struct qot_compare_info {
	uint32_t id;							// Hardware timer ID
	uint64_t next;							// Time of next event (clock ticks)
	uint64_t cycles_high;					// High cycle time (clock ticks)
	uint64_t cycles_low;					// Low cycle time (clock ticks)
	uint64_t limit;							// Number of repetition (0 = infinite)
	uint8_t  enable;						// [0] = disable, [1] = enabled
};

// Register a clock source as the primary driver of time
int qot_register(struct clocksource *clk);

// Add an oscillator
int qot_add_osc(struct qot_oscillator* osc);

// Register the existence of a timer pin
int qot_add_pin(int id, void (*compare)(struct qot_compare_event*));

// Called by driver when a capture event occurs
int qot_capture(const char* name, int id);

// Register a clock source as the primary driver of time
int qot_unregister(struct clocksource *clk);

#endif