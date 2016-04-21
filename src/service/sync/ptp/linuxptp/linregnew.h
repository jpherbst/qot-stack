/**
 * @file linreg_new.h
 * @note Copyright (C) 2014 Miroslav Lichvar <mlichvar@redhat.com>
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
#ifndef HAVE_LINREGNEW_H
#define HAVE_LINREGNEW_H

#include "servo.h"

struct servo *linregnew_servo_create();

double linregnew_sample(struct servo *servo,
			int64_t offset,
			uint64_t local_ts,
			enum servo_state *state,
			double *max_drift,
			double *min_drift);

#endif
