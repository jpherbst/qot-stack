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

#ifndef _PTP_CLOCK_H_
#define _PTP_CLOCK_H_

#include <linux/ioctl.h>
#include <linux/types.h>

/* PTP_xxx bits, for the flags field within the request structures. */
#define PTP_ENABLE_FEATURE (1<<0)
#define PTP_RISING_EDGE    (1<<1)
#define PTP_FALLING_EDGE   (1<<2)

/*
 * struct ptp_clock_time - represents a time value
 *
 * The sign of the seconds field applies to the whole value. The
 * nanoseconds field is always unsigned. The reserved field is
 * included for sub-nanosecond resolution, should the demand for
 * this ever appear.
 *
 */
struct ptp_clock_time {
	__s64 sec;  /* seconds */
	__u32 nsec; /* nanoseconds */
	__u32 reserved;
};

struct ptp_clock_caps {
	int max_adj;   /* Maximum frequency adjustment in parts per billon. */
	int n_alarm;   /* Number of programmable alarms. */
	int n_ext_ts;  /* Number of external time stamp channels. */
	int n_per_out; /* Number of programmable periodic signals. */
	int pps;       /* Whether the clock supports a PPS callback. */
	int n_pins;    /* Number of input/output pins. */
	int rsv[14];   /* Reserved for future use. */
};

struct ptp_extts_request {
	unsigned int index;  /* Which channel to configure. */
	unsigned int flags;  /* Bit field for PTP_xxx flags. */
	unsigned int rsv[2]; /* Reserved for future use. */
};

struct ptp_perout_request {
	struct ptp_clock_time start;  /* Absolute start time. */
	struct ptp_clock_time period; /* Desired period, zero means disable. */
	unsigned int index;           /* Which channel to configure. */
	unsigned int flags;           /* Reserved for future use. */
	unsigned int rsv[4];          /* Reserved for future use. */
};

#define PTP_MAX_SAMPLES 25 /* Maximum allowed offset measurement samples. */

struct ptp_sys_offset {
	unsigned int n_samples; /* Desired number of measurements. */
	unsigned int rsv[3];    /* Reserved for future use. */
	/*
	 * Array of interleaved system/phc time stamps. The kernel
	 * will provide 2*n_samples + 1 time stamps, with the last
	 * one as a system time stamp.
	 */
	struct ptp_clock_time ts[2 * PTP_MAX_SAMPLES + 1];
};

enum ptp_pin_function {
	PTP_PF_NONE,
	PTP_PF_EXTTS,
	PTP_PF_PEROUT,
	PTP_PF_PHYSYNC,
};

struct ptp_pin_desc {
	/*
	 * Hardware specific human readable pin name. This field is
	 * set by the kernel during the PTP_PIN_GETFUNC ioctl and is
	 * ignored for the PTP_PIN_SETFUNC ioctl.
	 */
	char name[64];
	/*
	 * Pin index in the range of zero to ptp_clock_caps.n_pins - 1.
	 */
	unsigned int index;
	/*
	 * Which of the PTP_PF_xxx functions to use on this pin.
	 */
	unsigned int func;
	/*
	 * The specific channel to use for this function.
	 * This corresponds to the 'index' field of the
	 * PTP_EXTTS_REQUEST and PTP_PEROUT_REQUEST ioctls.
	 */
	unsigned int chan;
	/*
	 * Reserved for future use.
	 */
	unsigned int rsv[5];
};

#define PTP_CLK_MAGIC '='

#define PTP_PIN_GETFUNC   _IOWR(PTP_CLK_MAGIC, 6, struct ptp_pin_desc)
#define PTP_CLOCK_GETCAPS  _IOR(PTP_CLK_MAGIC, 1, struct ptp_clock_caps)
#define PTP_EXTTS_REQUEST  _IOW(PTP_CLK_MAGIC, 2, struct ptp_extts_request)
#define PTP_PEROUT_REQUEST _IOW(PTP_CLK_MAGIC, 3, struct ptp_perout_request)
#define PTP_ENABLE_PPS     _IOW(PTP_CLK_MAGIC, 4, int)
#define PTP_SYS_OFFSET     _IOW(PTP_CLK_MAGIC, 5, struct ptp_sys_offset)
#define PTP_PIN_GETFUNC   _IOWR(PTP_CLK_MAGIC, 6, struct ptp_pin_desc)
#define PTP_PIN_SETFUNC    _IOW(PTP_CLK_MAGIC, 7, struct ptp_pin_desc)

struct ptp_extts_event {
	struct ptp_clock_time t; /* Time event occured. */
	unsigned int index;      /* Which channel produced the event. */
	unsigned int flags;      /* Reserved for future use. */
	unsigned int rsv[2];     /* Reserved for future use. */
};

#endif
