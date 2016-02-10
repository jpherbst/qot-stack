/*
 * @file qot_core.h
 * @brief Interfaces used by clocks to interact with core
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

#ifndef _PTP_CLOCK_KERNEL_H_
#define _PTP_CLOCK_KERNEL_H_

#include <linux/device.h>
#include <linux/pps_kernel.h>
#include <linux/ptp_clock.h>


struct ptp_clock_request {
	enum {
		PTP_CLK_REQ_EXTTS,
		PTP_CLK_REQ_PEROUT,
		PTP_CLK_REQ_PPS,
	} type;
	union {
		struct ptp_extts_request extts;
		struct ptp_perout_request perout;
	};
};

/**
 * struct ptp_clock_info - decribes a PTP hardware clock
 *
 * @owner:     The clock driver should set to THIS_MODULE.
 * @name:      A short "friendly name" to identify the clock and to
 *             help distinguish PHY based devices from MAC based ones.
 *             The string is not meant to be a unique id.
 * @max_adj:   The maximum possible frequency adjustment, in parts per billon.
 * @n_alarm:   The number of programmable alarms.
 * @n_ext_ts:  The number of external time stamp channels.
 * @n_per_out: The number of programmable periodic signals.
 * @n_pins:    The number of programmable pins.
 * @pps:       Indicates whether the clock supports a PPS callback.
 * @pin_config: Array of length 'n_pins'. If the number of
 *              programmable pins is nonzero, then drivers must
 *              allocate and initialize this array.
 *
 * clock operations
 *
 * @adjfreq:  Adjusts the frequency of the hardware clock.
 *            parameter delta: Desired frequency offset from nominal frequency
 *            in parts per billion
 *
 * @adjtime:  Shifts the time of the hardware clock.
 *            parameter delta: Desired change in nanoseconds.
 *
 * @gettime64:  Reads the current time from the hardware clock.
 *              parameter ts: Holds the result.
 *
 * @settime64:  Set the current time on the hardware clock.
 *              parameter ts: Time value to set.
 *
 * @enable:   Request driver to enable or disable an ancillary feature.
 *            parameter request: Desired resource to enable or disable.
 *            parameter on: Caller passes one to enable or zero to disable.
 *
 * @verify:   Confirm that a pin can perform a given function. The PTP
 *            Hardware Clock subsystem maintains the 'pin_config'
 *            array on behalf of the drivers, but the PHC subsystem
 *            assumes that every pin can perform every function. This
 *            hook gives drivers a way of telling the core about
 *            limitations on specific pins. This function must return
 *            zero if the function can be assigned to this pin, and
 *            nonzero otherwise.
 *            parameter pin: index of the pin in question.
 *            parameter func: the desired function to use.
 *            parameter chan: the function channel index to use.
 *
 * Drivers should embed their ptp_clock_info within a private
 * structure, obtaining a reference to it using container_of().
 *
 * The callbacks must all return zero on success, non-zero otherwise.
 */

struct ptp_clock_info {
	struct module *owner;
	char name[16];
	s32 max_adj;
	int n_alarm;
	int n_ext_ts;
	int n_per_out;
	int n_pins;
	int pps;
	struct ptp_pin_desc *pin_config;
	int (*adjfreq)(struct ptp_clock_info *ptp, s32 delta);
	int (*adjtime)(struct ptp_clock_info *ptp, s64 delta);
	int (*gettime64)(struct ptp_clock_info *ptp, struct timespec64 *ts);
	int (*settime64)(struct ptp_clock_info *p, const struct timespec64 *ts);
	int (*enable)(struct ptp_clock_info *ptp,
		      struct ptp_clock_request *request, int on);
	int (*verify)(struct ptp_clock_info *ptp, unsigned int pin,
		      enum ptp_pin_function func, unsigned int chan);
};

struct ptp_clock;

/**
 * ptp_clock_register() - register a PTP hardware clock driver
 *
 * @info:   Structure describing the new clock.
 * @parent: Pointer to the parent device of the new clock.
 */

extern struct qot_timeline *qot_timeline_register(
    struct qot_timeline_info *info, struct device *parent);

/**
 * ptp_clock_unregister() - unregister a PTP hardware clock driver
 *
 * @ptp:  The clock to remove from service.
 */

extern int ptp_clock_unregister(struct ptp_clock *ptp);


enum ptp_clock_events {
	PTP_CLOCK_ALARM,
	PTP_CLOCK_EXTTS,
	PTP_CLOCK_PPS,
	PTP_CLOCK_PPSUSR,
};

/**
 * struct ptp_clock_event - decribes a PTP hardware clock event
 *
 * @type:  One of the ptp_clock_events enumeration values.
 * @index: Identifies the source of the event.
 * @timestamp: When the event occurred (%PTP_CLOCK_EXTTS only).
 * @pps_times: When the event occurred (%PTP_CLOCK_PPSUSR only).
 */

struct ptp_clock_event {
	int type;
	int index;
	union {
		u64 timestamp;
		struct pps_event_time pps_times;
	};
};

/**
 * ptp_clock_event() - notify the PTP layer about an event
 *
 * @ptp:    The clock obtained from ptp_clock_register().
 * @event:  Message structure describing the event.
 */

extern void ptp_clock_event(struct ptp_clock *ptp,
			    struct ptp_clock_event *event);

/**
 * ptp_clock_index() - obtain the device index of a PTP clock
 *
 * @ptp:    The clock obtained from ptp_clock_register().
 */

extern int ptp_clock_index(struct ptp_clock *ptp);

/**
 * ptp_find_pin() - obtain the pin index of a given auxiliary function
 *
 * @ptp:    The clock obtained from ptp_clock_register().
 * @func:   One of the ptp_pin_function enumerated values.
 * @chan:   The particular functional channel to find.
 * Return:  Pin index in the range of zero to ptp_clock_caps.n_pins - 1,
 *          or -1 if the auxiliary function cannot be found.
 */

int ptp_find_pin(struct ptp_clock *ptp,
		 enum ptp_pin_function func, unsigned int chan);

#endif
