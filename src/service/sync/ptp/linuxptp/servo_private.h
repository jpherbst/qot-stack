/**
 * @file servo_private.h
 * @note Copyright (C) 2011 Richard Cochran <richardcochran@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#ifndef HAVE_SERVO_PRIVATE_H
#define HAVE_SERVO_PRIVATE_H

#include "contain.h"
#include <sys/types.h>
#include <sys/queue.h>

#define MAX_SAMPLES 4 /* QOT, number of points needed for uncertainty calculation */

struct uncertainty_stats{
	uint64_t disp;		// error in measurement used to calculate peer-dispersion
	int64_t offset;		// offsets stored to calculate jitter
	uint64_t delay;		// one-way propagation delay, lesser the delay more accurate the offset value is
};

struct clock_uncertainty_stats {
	struct uncertainty_stats u_stats[MAX_SAMPLES];	// uncertainty stats
	unsigned int num_points;						// number of stored points
	unsigned int last_point;						// index of newest point
};

struct servo {
    LIST_ENTRY(servo) list; 				/* QOT */
    int tml_clkid; 							/* timeline clock id */       
    struct clock_uncertainty_stats u_stats; /* QOT, stats to calculate uncertainty in time */
	uint64_t local_ts; 						/* QOT, store the last ingress timestamp */
	double ppb;								/* QOT, store the last freq correction */

	double max_frequency;
	double step_threshold;
	double first_step_threshold;
	int first_update;

	void (*destroy)(struct servo *servo);

	double (*sample)(struct servo *servo,
			 int64_t offset, uint64_t local_ts,
			 enum servo_state *state);

	void (*sync_interval)(struct servo *servo, double interval);

	void (*reset)(struct servo *servo);

	double (*rate_ratio)(struct servo *servo);

	void (*leap)(struct servo *servo, int leap);
};

#endif
