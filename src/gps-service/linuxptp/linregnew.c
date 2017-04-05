/**
 * @file pi.c
 * @brief Implements a Proportional Integral clock servo.
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
#include <stdlib.h>
#include <math.h>

#include "linregnew.h"
#include "print.h"
#include "servo_private.h"

#define REGRESSION_TABLE_SIZE 6

struct linregnew_servo {
	struct servo servo;
	double data[REGRESSION_TABLE_SIZE][2];
	uint8_t next;
	uint8_t n;
	double m;
	double b;
	double r;
};

static void linregnew_destroy(struct servo *servo)
{
	struct linregnew_servo *s = container_of(servo, struct linregnew_servo, servo);
	free(s);
}

double linregnew_sample(struct servo *servo,
			int64_t offset,
			uint64_t local_ts,
			enum servo_state *state,
			double *max_drift,
			double *min_drift)
{
	struct linregnew_servo *s = container_of(servo, struct linregnew_servo, servo);

	double ppb, ppbnow = 0.0;
	double ppbmax = 0.0;
	double ppbmin = 0.0;

	// Map from a local time to an error
	s->data[s->next][0] = (double) local_ts;
	s->data[s->next][1] = (double) offset;

	// update table size
	if (s->n < REGRESSION_TABLE_SIZE) 
		s->n++;

	if (s->n > 1) {
	// Temp variables
	double   sumx = 0.0;                        /* sum of x                      */
	double   sumx2 = 0.0;                       /* sum of x**2                   */
	double   sumxy = 0.0;                       /* sum of x * y                  */
	double   sumy = 0.0;                        /* sum of y                      */
	double   sumy2 = 0.0;                       /* sum of y**2                   */

	// k = l_0
	double k = s->data[0][0];

	// uncertainty initialization
	ppbnow = ((s->data[1][1] - s->data[0][1]) / (s->data[1][0] - s->data[0][0])) * 1e9;
	ppbmax = ppbmin = ppbnow;

	double c = 0;
	for (int i = 0; i < s->n; i++)
	{
		c     += 1.0;                      		   			/* increment num of data points  */
		sumx  += (s->data[i][0] - k);                         	/* compute sum of x              */
		sumx2 += (s->data[i][0] - k) * (s->data[i][0] - k);      	/* compute sum of x**2           */
		sumxy += (s->data[i][0] - k) * s->data[i][1];            	/* compute sum of x * y          */
		sumy  += s->data[i][1];                           	   	/* compute sum of y              */
		sumy2 += s->data[i][1] * s->data[i][1];                  	/* compute sum of y**2           */

		// highest and lowest slope calculation (for uncertainty in sync)
		if(i > 1){
			ppbnow = ((s->data[i][1] - s->data[i-1][1]) / (s->data[i][0] - s->data[i-1][0])) * 1e9;
			if (ppbnow > ppbmax)
				ppbmax = ppbnow;
			if (ppbnow < ppbmin)
				ppbmin = ppbnow;
		}
	}                                        	/* loop again for more data      */

	// mean and stddev of offset
	

	s->m = (c * sumxy  -  sumx * sumy) /        	/* compute slope                 */
		(c * sumx2 - sumx*sumx);

	s->b = (sumy * sumx2  -  sumx * sumxy) /    	/* compute y-intercept           */
		(c * sumx2  -  sumx*sumx);

	s->r = k;										/* local offset */

	}
	// move pointer to the next free entry
	s->next = (s->next + 1) % REGRESSION_TABLE_SIZE;

	if(s->n < 2)
		*state = SERVO_UNLOCKED;
	else if (s->n == 2)
		*state = SERVO_JUMP;
	else
		*state = SERVO_LOCKED;

	ppb = s->m * 1e9;
	*max_drift = ppbmax;
	*min_drift = ppbmin;

	//pr_info("n: %i, next: %i, m: %.9f, ppb: %.3f", s->n, s->next, s->m, ppb);
	return ppb;
}

static void linregnew_sync_interval(struct servo *servo, double interval)
{
	pr_debug("nothing to do yet");
}

static void linregnew_reset(struct servo *servo)
{
	struct linregnew_servo *s = container_of(servo, struct linregnew_servo, servo);

	s->n = 0;
	s->next = 0;
	s->m = 0.0;
}

struct servo *linregnew_servo_create()
{
	struct linregnew_servo *s;

	s = calloc(1, sizeof(*s));
	if (!s)
		return NULL;

	s->servo.destroy = linregnew_destroy;
	//s->servo.sample  = linregnew_sample; //QoT
	s->servo.sync_interval = linregnew_sync_interval;
	s->servo.reset   = linregnew_reset;
	s->next          = 0;
	s->n             = 0;
	s->m             = 0.0;

	return &s->servo;
}
