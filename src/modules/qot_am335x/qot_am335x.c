/*
 * @file qot_am335x.c
 * @brief Driver that exposes AM335x subsystem as a PTP clock for the QoT stack
 * @author Andrew Symington and Sandeep D'souza
 *
 * Copyright (c) Regents of the University of California, 2015.
 * Copyright (c) Carnegie Mellon University, 2016-2018.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 	1. Redistributions of source code must retain the above copyright notice,
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

/* Sufficient to develop a device-tree based platform driver */
#include <linux/version.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/timecounter.h>
#include <linux/clocksource.h>
#include <linux/gpio.h>
#include <linux/clockchips.h>

#include <kernel/time/tick-internal.h>

/* We are going to export the whol am335x as a PTP clock */
#include <linux/ptp_clock_kernel.h>

/*QoT Core Clock Registration*/
#include "../qot/qot_core.h"

/* DMTimer Code specific to the AM335x */
#include <arch/arm/plat-omap/include/plat/dmtimer.h>

/* Available Pins and Timers -> Based on Device Tree Configuration*/
#define PHC2PHC_PIN 0
#define CAPTURE_PIN 1
#define COMPARE_PIN 2

#define PHC2PHC_TIMER 5
#define CAPTURE_TIMER 6
#define COMPARE_TIMER 7

#define OVERIDE_FLAG 1 // override predefined values (timers and pins)

static int overide_flag = OVERIDE_FLAG;

/* Different event calls for GPIO */
enum qot_am335x_events {
	EVENT_MATCH,
	EVENT_OVERFLOW,
	EVENT_START,
	EVENT_STOP
};

/* Useful definitions */
#define MODULE_NAME 			"qot_am335x"
#define AM335X_REWRITE_DELAY 	13
#define AM335X_NO_PRESCALER 	0xFFFFFFFF
#define MIN_PERIOD_NS			1000ULL
#define MAX_PERIOD_NS			100000000000ULL


// PLATFORM DATA ///////////////////////////////////////////////////////////////

struct qot_am335x_data;

// Scheduler specific stuff added by Sandeep
static struct omap_dm_timer **sched_timer = NULL;
static struct qot_am335x_data *qot_am335x_data_ptr = NULL;

struct qot_am335x_compare_interface{
	qot_perout_t perout;					/* Periodic Output Data        		  */
	u32 load;								/* Load Register               		  */
	u32 match;								/* Match Register              		  */
	int key;								/* Key to enable single access        */
	int reprogram_flag;                     /* Flag indicating whether a reprogram is needed */
	qot_return_t (*callback)(qot_perout_t *perout_ret, timepoint_t *event_core_timestamp, timepoint_t *next_event_time);	/* Time Conversion Callback */
};

struct qot_am335x_sched_interface {
	struct qot_am335x_data *parent;			/* Pointer to parent */
	struct omap_dm_timer *timer;
	struct clock_event_device qot_clockevent;
	long (*callback)(void);
};

struct qot_am335x_channel {
	struct qot_am335x_data *parent;			/* Pointer to parent */
	struct omap_dm_timer *timer;			/* OMAP Timer */
	struct ptp_clock_request state;			/* PTP state */
	struct gpio_desc *gpiod;				/* GPIO description */
};

struct qot_am335x_data {
	raw_spinlock_t lock;					/* Protects timer registers */
	struct qot_am335x_channel  core;		/* Timer channel (core) */
	struct qot_am335x_channel *pins;		/* Timer channel (GPIO) */
	int num_pins;							/* Number of pins */
	struct timecounter tc;					/* Time counter */
	struct cyclecounter cc;					/* Cycle counter */
	u32 cc_mult; 							/* f0 */
	struct ptp_clock *clock;				/* PTP clock */
	struct ptp_clock_info info;				/* PTP clock info */

	// Added by Sandeep
	struct qot_am335x_sched_interface core_sched;	/* Scheduler Interface */
	struct qot_clock_impl qot_am335x_impl_info; 	/* QoT Info */
	struct qot_am335x_compare_interface compare;    /* Compare Interface */
};

static void omap_dm_timer_setup_capture(struct omap_dm_timer *timer, u32 edge) {
	u32 ctrl, mask = OMAP_TIMER_CTRL_TCM_BOTHEDGES;
	if ((edge & PTP_RISING_EDGE) && (edge & PTP_FALLING_EDGE))
		mask = OMAP_TIMER_CTRL_TCM_BOTHEDGES;
	else if (edge & PTP_FALLING_EDGE)
		mask = OMAP_TIMER_CTRL_TCM_HIGHTOLOW;
	else if (edge & PTP_RISING_EDGE)
		mask = OMAP_TIMER_CTRL_TCM_LOWTOHIGH;
	ctrl = __omap_dm_timer_read(timer, OMAP_TIMER_CTRL_REG, timer->posted);
	ctrl |= mask | OMAP_TIMER_CTRL_GPOCFG;
	__omap_dm_timer_write(timer, OMAP_TIMER_CTRL_REG, ctrl, timer->posted);
	timer->context.tclr = ctrl;
}

static s64 ptp_to_s64(struct ptp_clock_time *t) {
	return (s64) t->sec * (s64) 1000000000 + (s64) t->nsec;
}

// TIMER MANAGEMENT ////////////////////////////////////////////////////////////

static int qot_am335x_core(struct qot_am335x_channel *channel, int event)
{
	struct omap_dm_timer *timer = channel->timer;
	switch (event) {
	case EVENT_START:
		omap_dm_timer_enable(timer);
		omap_dm_timer_set_prescaler(timer, AM335X_NO_PRESCALER);
		omap_dm_timer_set_load(timer, 1, 0); /* 1: autoreload, 0: load val */
		omap_dm_timer_start(timer);
		omap_dm_timer_set_int_enable(timer, OMAP_TIMER_INT_OVERFLOW);
		omap_dm_timer_enable(timer);
		break;
	case EVENT_STOP:
		omap_dm_timer_set_int_disable(timer, OMAP_TIMER_INT_OVERFLOW);
		omap_dm_timer_enable(timer);
		omap_dm_timer_stop(timer);
		break;
	}

	return 0;
}

static timepoint_t qot_am335x_read_time(void)
{
	u64 ns;
	timepoint_t time_now;
	unsigned long flags;
	struct qot_am335x_data *pdata;
	pdata = qot_am335x_data_ptr;

	raw_spin_lock_irqsave(&pdata->lock, flags);
	ns = timecounter_read(&pdata->tc);
	raw_spin_unlock_irqrestore(&pdata->lock, flags);
	TP_FROM_nSEC(time_now, (s64)ns);

	return time_now;
}

static int qot_am335x_perout(struct qot_am335x_channel *channel, int event)
{
	s64 ts, tp, duty;
	u32 load, match, tc, offset;
	timepoint_t event_core_timestamp, next_event;
	unsigned long flags;
	struct omap_dm_timer *timer, *core;

	/* Check if the channel is valid */
	if (!channel)
		return -EINVAL;

	/* Get references to timer and core */
	timer = channel->timer;
	core  = channel->parent->core.timer;
	if (!timer || !core)
		return -EINVAL;
	/* What event is needed */
	switch (event) {

	case EVENT_MATCH:

		/* Do nothing - can be used to stop PWM */
	    if(channel->parent->compare.callback != NULL)
	    {
	    	event_core_timestamp = qot_am335x_read_time();
			channel->parent->compare.callback(&channel->parent->compare.perout, &event_core_timestamp, &next_event);
		}
		
		// Compare start of reprogram with current timeline reference time (callback returns timeline reference in event_core_timestamp)
	    if(channel->parent->compare.reprogram_flag == 1 && (TP_TO_nSEC(channel->parent->compare.perout.start) - TL_TO_nSEC(channel->parent->compare.perout.period)) < TP_TO_nSEC(event_core_timestamp))
	    {
		    tc = omap_dm_timer_read_counter(timer);
		    //omap_dm_timer_enable(timer);
		    omap_dm_timer_stop(timer);
			omap_dm_timer_set_load(timer, 1, -channel->parent->compare.load);		 /*1 = autoreload */
			omap_dm_timer_set_match(timer, 1, -channel->parent->compare.match);		 /* 1 = enable */
			omap_dm_timer_start(timer);
			omap_dm_timer_write_counter(timer, -channel->parent->compare.match + AM335X_REWRITE_DELAY);
		}
		break;

	case EVENT_OVERFLOW:

		/* Do nothing - can be used to modify PWM */
		if(timer->id == COMPARE_TIMER)
		{
			// TODO/* Get an s64 value in core time */
			if(channel->parent->compare.callback != NULL)
	    	{
				event_core_timestamp = qot_am335x_read_time();
				channel->parent->compare.callback(&channel->parent->compare.perout, &event_core_timestamp, &next_event);
			}
		}

		break;

	case EVENT_START:

		/* Get the signed ns start time and period */
		ts = ptp_to_s64(&channel->state.perout.start);
		tp = ptp_to_s64(&channel->state.perout.period);
		if(timer->id == COMPARE_TIMER)
			duty = div_s64(tp*(100LL-(s64)channel->parent->compare.perout.duty_cycle),100L);
		else
		{
			duty = div_s64(tp*99LL,100L);
			channel->parent->compare.perout.edge = 1;
		}

		/* Some basic period checks for sanity */
		if (tp < MIN_PERIOD_NS || tp > MAX_PERIOD_NS)
			return -EINVAL;
		raw_spin_lock_irqsave(&channel->parent->lock, flags);

		ts = ts - channel->parent->tc.nsec;

		/* Some basic period checks for sanity */
		if (ts < MIN_PERIOD_NS || ts > MAX_PERIOD_NS)
			return -EINVAL;

		pr_info("qot_am335x: compare tp = %lld, ts = %lld\n", tp, ts);

		/* Work out the cycle count corresponding to this edge */
		ts = div_u64((ts << channel->parent->cc.shift)
			+ channel->parent->tc.frac, channel->parent->cc.mult);
		tc = ts;
		tc = tc + channel->parent->tc.cycle_last;
	
		/* Use the cyclecounter mult at shift to scale */
		tp = div_u64((tp << channel->parent->cc.shift)
			+ channel->parent->tc.frac, channel->parent->cc.mult);
		duty = div_u64((duty << channel->parent->cc.shift)
			+ channel->parent->tc.frac, channel->parent->cc.mult);

		raw_spin_unlock_irqrestore(&channel->parent->lock, flags);

		/* Map to an offset, load and match */
		offset = tc; 		// (start)
		load   = tp; 		// (low+high)
		match = duty;       // (low)

		pr_info("qot_am335x: compare duty cycle = %lld, period = %lld, edge = %d\n", duty, tp, channel->parent->compare.perout.edge);
		
		channel->parent->compare.load = load;
		channel->parent->compare.match = match;

		/* Configure timer */
		if(timer->id == COMPARE_TIMER || overide_flag)
			omap_dm_timer_set_int_enable(timer, OMAP_TIMER_INT_OVERFLOW | OMAP_TIMER_INT_MATCH);
		omap_dm_timer_enable(timer);
		omap_dm_timer_set_prescaler(timer, AM335X_NO_PRESCALER);
		omap_dm_timer_set_pwm(timer, channel->parent->compare.perout.edge - 1, 1,
			OMAP_TIMER_TRIGGER_OVERFLOW_AND_COMPARE);	/* 3 = trigger */
		omap_dm_timer_set_load(timer, 1, -load);		/* 1 = autoreload */
		omap_dm_timer_set_match(timer, 1, -match);		/* 1 = enable */

		/* Bootstrap timer to core time */
		omap_dm_timer_start(timer);
		omap_dm_timer_write_counter(timer,
			omap_dm_timer_read_counter(core) - offset + AM335X_REWRITE_DELAY);

		break;

	case EVENT_STOP:

		/* Disable the timer */
		if(timer->id == COMPARE_TIMER || overide_flag)
			omap_dm_timer_set_int_disable(timer, OMAP_TIMER_INT_OVERFLOW | OMAP_TIMER_INT_MATCH);
		omap_dm_timer_enable(timer);
		omap_dm_timer_stop(timer);

		break;
	}

	return 0;
}

static int qot_am335x_extts(struct qot_am335x_channel *channel, int event)
{
	struct omap_dm_timer *timer, *core;

	/* Check if the channel is valid */
	if (!channel)
		return -EINVAL;

	/* Get references to timer and core */
	timer = channel->timer;
	core  = channel->parent->core.timer;
	if (!timer || !core)
		return -EINVAL;

	/* Determine action */
	switch (event) {

	case EVENT_START:

		/* Configure timer */
		omap_dm_timer_enable(timer);
		omap_dm_timer_set_prescaler(timer, AM335X_NO_PRESCALER);
		omap_dm_timer_setup_capture(timer,channel->state.extts.flags);
		omap_dm_timer_set_load(timer, 1, 0);

		/* Enable interrupts */
		omap_dm_timer_set_int_enable(timer, OMAP_TIMER_INT_CAPTURE);
		omap_dm_timer_enable(timer);

		/* Bootstrap timer to core time */
		omap_dm_timer_start(timer);
		omap_dm_timer_write_counter(timer,
			omap_dm_timer_read_counter(core) + AM335X_REWRITE_DELAY);

		break;

	case EVENT_STOP:

		/* Disable the timer and interrupts */
		omap_dm_timer_set_int_disable(timer, OMAP_TIMER_INT_CAPTURE);
		omap_dm_timer_enable(timer);
		omap_dm_timer_stop(timer);

		break;
	}

	return 0;
}

static cycle_t qot_am335x_read(const struct cyclecounter *cc)
{
	struct qot_am335x_data *pdata = container_of(
        cc, struct qot_am335x_data, cc);
	return __omap_dm_timer_read_counter(pdata->core.timer,
	    pdata->core.timer->posted);
}

static void qot_am335x_overflow(struct qot_am335x_channel *channel)
{
	timecounter_read(&channel->parent->tc);
}

// INTERRUPT HANDLING //////////////////////////////////////////////////////////

static irqreturn_t qot_am335x_interrupt(int irq, void *data)
{
	unsigned long flags;
	unsigned int irq_status;
	struct ptp_clock_event pevent;
	struct qot_am335x_channel *channel = data;
	raw_spin_lock_irqsave(&channel->parent->lock, flags);
	irq_status = omap_dm_timer_read_status(channel->timer);
	switch (channel->state.type) {
	case PTP_CLK_REQ_EXTTS:
		if (irq_status & OMAP_TIMER_INT_CAPTURE) {
			pevent.timestamp = timecounter_cyc2time(&channel->parent->tc,
	            __omap_dm_timer_read(channel->timer, OMAP_TIMER_CAPTURE_REG,
    		        channel->timer->posted));
			pevent.type = PTP_CLOCK_EXTTS;
			pevent.index = channel->state.extts.index;
			ptp_clock_event(channel->parent->clock, &pevent);
			qot_am335x_overflow(channel); // Fatima: Fix for time going backward in capture
		}
		break;
	case PTP_CLK_REQ_PEROUT:
		if (irq_status & OMAP_TIMER_INT_MATCH)
			qot_am335x_perout(channel, EVENT_MATCH);
		if (irq_status & OMAP_TIMER_INT_OVERFLOW)
			qot_am335x_perout(channel, EVENT_OVERFLOW);
		qot_am335x_overflow(channel); // Fatima: Fix for time going backward in capture
		break;
	case PTP_CLK_REQ_PPS:
		if (	(irq_status & OMAP_TIMER_INT_MATCH)
		        || 	(irq_status & OMAP_TIMER_INT_OVERFLOW))
			qot_am335x_overflow(channel);
		break;
	}
	__omap_dm_timer_write_status(channel->timer, irq_status);
	raw_spin_unlock_irqrestore(&channel->parent->lock, flags);
	return IRQ_HANDLED;
}

// PTP ABSTRACTION /////////////////////////////////////////////////////////////

static int qot_am335x_adjfreq(struct ptp_clock_info *ptp, s32 ppb)
{
	u64 adj;
	u32 diff, mult;
	unsigned long flags;
	int neg_adj = 0;
	struct qot_am335x_data *pdata = container_of(
		ptp, struct qot_am335x_data, info);
	if (ppb < 0) {
		neg_adj = 1;
		ppb = -ppb;
	}
	mult = pdata->cc_mult;
	adj = mult;
	adj *= ppb;
	diff = div_u64(adj, 1000000000ULL);

	raw_spin_lock_irqsave(&pdata->lock, flags);
	timecounter_read(&pdata->tc);
	pdata->cc.mult = neg_adj ? mult - diff : mult + diff;
	raw_spin_unlock_irqrestore(&pdata->lock, flags);
	return 0;
}

static int qot_am335x_adjtime(struct ptp_clock_info *ptp, s64 delta)
{
	unsigned long flags;
	struct qot_am335x_data *pdata = container_of(
        ptp, struct qot_am335x_data, info);
	raw_spin_lock_irqsave(&pdata->lock, flags);
	timecounter_adjtime(&pdata->tc, delta);
	raw_spin_unlock_irqrestore(&pdata->lock, flags);
	return 0;
}

static int qot_am335x_gettime(struct ptp_clock_info *ptp, struct timespec64 *ts)
{
	u64 ns;
	unsigned long flags;
	struct qot_am335x_data *pdata = container_of(
    	ptp, struct qot_am335x_data, info);
	raw_spin_lock_irqsave(&pdata->lock, flags);
	ns = timecounter_read(&pdata->tc);
	raw_spin_unlock_irqrestore(&pdata->lock, flags);
	*ts = ns_to_timespec64(ns);
	return 0;
}


static int qot_am335x_settime(struct ptp_clock_info *ptp,
                            const struct timespec64 *ts)
{
	u64 ns;
	unsigned long flags;
	struct qot_am335x_data *pdata = container_of(
        ptp, struct qot_am335x_data, info);
	ns = timespec64_to_ns(ts);
	raw_spin_lock_irqsave(&pdata->lock, flags);
	timecounter_init(&pdata->tc, &pdata->cc, ns);
	raw_spin_unlock_irqrestore(&pdata->lock, flags);
	return 0;
}

static int qot_am335x_enable(struct ptp_clock_info *ptp,
                             struct ptp_clock_request *rq, int on)
{
	struct qot_am335x_data *pdata = container_of(
        ptp, struct qot_am335x_data, info);
	switch (rq->type) {
	case PTP_CLK_REQ_EXTTS:
		/* Check if the correct timer is requested -> This timer pin is set to INPUT in the Device Tree*/
		//if(rq->extts.index != CAPTURE_PIN)
		//	return -EACCES;

		memcpy(&pdata->pins[rq->extts.index].state, rq,
			sizeof(struct ptp_clock_request));
		return qot_am335x_extts(&pdata->pins[rq->extts.index],
			(on ? EVENT_START : EVENT_STOP));
	case PTP_CLK_REQ_PEROUT:
		/* Check if the correct timer is requested -> This timer pin is set to OUTPUT in the Device Tree*/
		// if(rq->perout.index != PHC2PHC_PIN)
		// 	return -EACCES;

		memcpy(&pdata->pins[rq->perout.index].state, rq,
			sizeof(struct ptp_clock_request));
		return qot_am335x_perout(&pdata->pins[rq->perout.index],
			(on ? EVENT_START : EVENT_STOP));
	default:
		break;
	}
	return -EOPNOTSUPP;
}

static long qot_am335x_timeline_enable_compare(timepoint_t *core_start, timelength_t *core_period, qot_perout_t *perout, qot_return_t (*callback)(qot_perout_t *perout_ret, timepoint_t *event_core_timestamp, timepoint_t *next_event), int on)
{
	unsigned long flags;
	struct qot_am335x_data *pdata = qot_am335x_data_ptr;
    int overwrite_request = 0;
    timelength_t core_period_time = *core_period;
    s64 tp, duty;
	if(on)
	{
		raw_spin_lock_irqsave(&pdata->lock, flags);
		if(pdata->compare.key != 0 && pdata->compare.perout.owner_file != perout->owner_file)
		{
			raw_spin_unlock_irqrestore(&pdata->lock, flags);
			pr_info("enable_compare: compare failed\n");
			return 1;
		}
		else if(pdata->compare.key == 1)
		{
			pr_info("enable_compare: being reprogrammed\n");
			if(TP_TO_nSEC(pdata->compare.perout.start) < TP_TO_nSEC(perout->start))
			{
				tp = (s64) TL_TO_nSEC(core_period_time);
				if (tp < MIN_PERIOD_NS || tp > MAX_PERIOD_NS)
				{
					raw_spin_unlock_irqrestore(&pdata->lock, flags);
					pr_info("enable_compare: period too large\n");
					return 1;
				}
				tp = div_u64((tp << pdata->cc.shift) + pdata->tc.frac, pdata->cc.mult);
				pdata->compare.load = tp;
				duty = div_s64(tp*(100LL-(s64)pdata->compare.perout.duty_cycle),100L);
				pdata->compare.match = duty;
				pdata->compare.perout = *perout;
				pdata->compare.reprogram_flag = 1;
				raw_spin_unlock_irqrestore(&pdata->lock, flags);
				return 0;
			}
			overwrite_request = 1;
		}
		pdata->compare.key = 1;
		raw_spin_unlock_irqrestore(&pdata->lock, flags);

		// Set Callback and perout data
		pdata->compare.callback = callback;
		pdata->compare.perout = *perout;
		pr_info("enable_compare: timeline %d, period %llu %llu\n", perout->timeline.index, perout->period.sec, perout->period.asec);
		
		// Set PTP state
		pdata->pins[COMPARE_PIN].state.type = PTP_CLK_REQ_PEROUT;
		
		// Set Time Values
		pdata->pins[COMPARE_PIN].state.perout.start.sec = core_start->sec;
		pdata->pins[COMPARE_PIN].state.perout.start.nsec = (u32) div_u64(core_start->asec, (u32) 1000000000);
		pdata->pins[COMPARE_PIN].state.perout.period.sec = (s64) core_period->sec;
		pdata->pins[COMPARE_PIN].state.perout.period.nsec = (u32) div_u64(core_period->asec, (u32) 1000000000);
	}
	else
	{
		raw_spin_lock_irqsave(&pdata->lock, flags);
		if(pdata->compare.key != 1)
		{
			raw_spin_unlock_irqrestore(&pdata->lock, flags);
			return 1;
		}
		pdata->compare.key = 0;
		pdata->compare.perout.owner_file = NULL;
		pdata->compare.reprogram_flag = 0;
		raw_spin_unlock_irqrestore(&pdata->lock, flags);
	}
	
	if (overwrite_request == 1)
	{
		qot_am335x_perout(&pdata->pins[COMPARE_PIN], EVENT_STOP);
	}
	if(qot_am335x_perout(&pdata->pins[COMPARE_PIN], (on ? EVENT_START : EVENT_STOP)) < 0)
	{
		if(on)
		{
			raw_spin_lock_irqsave(&pdata->lock, flags);
			pdata->compare.perout.owner_file = NULL;
			pdata->compare.key = 0;
			raw_spin_unlock_irqrestore(&pdata->lock, flags);
		}
		return 1;
	}
	return 0;
}

static int qot_am335x_verify(struct ptp_clock_info *ptp, unsigned int pin,
                             enum ptp_pin_function func, unsigned int chan)
{
	/* Check number of pins */
	if (pin >= ptp->n_pins || !ptp->pin_config)
		return -EINVAL;

	/* Lock the channel */
	if (chan != ptp->pin_config[pin].chan)
		return -EINVAL;

	/* Check function */
	switch (func) {
	case PTP_PF_NONE:	/* Nothing */
	case PTP_PF_EXTTS:	/* External time stamp */
	case PTP_PF_PEROUT:	/* Periodic signal */
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static struct ptp_clock_info qot_am335x_info = {
	.owner		= THIS_MODULE,
	.name		= "AM335x timer",
	.max_adj	= 1000000,
	.n_pins		= 0,
	.n_alarm    = 0,	/* Will be changed by probe */
	.n_ext_ts	= 0,	/* Will be changed by probe */
	.n_per_out  = 0,	/* Will be changed by probe */
	.pps		= 0,
	.adjfreq	= qot_am335x_adjfreq,
	.adjtime	= qot_am335x_adjtime,
	.gettime64	= qot_am335x_gettime,
	.settime64	= qot_am335x_settime,
	.enable		= qot_am335x_enable,
	.verify     = qot_am335x_verify,
};

// SCHEDULER INTERFACE TIMER ///////////////////////////////////////////////////
// Helper Function figures out the maximum and minimum values in ns which the timer supports
static u64 clkev_delta2ns(unsigned long latch, struct clock_event_device *evt, bool ismax)
{
	u64 clc = (u64) latch << evt->shift;
	u64 rnd;

	if (unlikely(!evt->mult)) {
		evt->mult = 1;
		WARN_ON(1);
	}
	rnd = (u64) evt->mult - 1;

	/*
	 * Upper bound sanity check. If the backwards conversion is
	 * not equal latch, we know that the above shift overflowed.
	 */
	if ((clc >> evt->shift) != (u64)latch)
		clc = ~0ULL;

	/*
	 * Scaled math oddities:
	 *
	 * For mult <= (1 << shift) we can safely add mult - 1 to
	 * prevent integer rounding loss. So the backwards conversion
	 * from nsec to device ticks will be correct.
	 *
	 * For mult > (1 << shift), i.e. device frequency is > 1GHz we
	 * need to be careful. Adding mult - 1 will result in a value
	 * which when converted back to device ticks can be larger
	 * than latch by up to (mult - 1) >> shift. For the min_delta
	 * calculation we still want to apply this in order to stay
	 * above the minimum device ticks limit. For the upper limit
	 * we would end up with a latch value larger than the upper
	 * limit of the device, so we omit the add to stay below the
	 * device upper boundary.
	 *
	 * Also omit the add if it would overflow the u64 boundary.
	 */
	if ((~0ULL - clc > rnd) &&
	    (!ismax || evt->mult <= (1ULL << evt->shift)))
		clc += rnd;

	do_div(clc, evt->mult);

	/* Deltas less than 1usec are pointless noise */
	return clc > 1000 ? clc : 1000;
}

// Helper function, configures the clockevent device to convert a ns value to cycle counts
void clkev_configure(struct clock_event_device *dev, u32 freq)
{
	u64 sec;
	u64 tmp;
    u32 sft, sftacc= 32;

	sec = dev->max_delta_ticks;
	do_div(sec, freq);
	if (!sec)
		sec = 1;
	else if(sec > 600 && dev->max_delta_ticks > UINT_MAX)
		sec = 600;


	/*
	 * Calculate the shift factor which is limiting the conversion
	 * range:
     */
	tmp = ((u64)sec * NSEC_PER_SEC) >> 32;
    while (tmp) {
	    tmp >>=1;
        sftacc--;
	}

	/*
     * Find the conversion shift/mult pair which has the best
     * accuracy and fits the maxsec conversion range:
     */
	for (sft = 32; sft > 0; sft--)
	{
	    tmp = (u64) freq << sft;
	    tmp += NSEC_PER_SEC / 2;
	    do_div(tmp, NSEC_PER_SEC);
	    if ((tmp >> sftacc) == 0)
	        break;
	}
	dev->mult = tmp;
	dev->shift = sft;
	dev->min_delta_ns = clkev_delta2ns(dev->min_delta_ticks, dev, false);
	dev->max_delta_ns = clkev_delta2ns(dev->max_delta_ticks, dev, true);
	pr_info("qot_am335x: clkev mult = %lu\n", (unsigned long)dev->mult);
	pr_info("qot_am335x: clkev shift = %lu\n", (unsigned long)dev->shift);
	pr_info("qot_am335x: clkev max_ns = %llu\n", (unsigned long long)dev->max_delta_ns);
	pr_info("qot_am335x: clkev min_ns = %llu\n", (unsigned long long)dev->min_delta_ns);

	return;
}

// Programs the Sched Timer Interrupt
static int qot_am335x_sched_interface_program_interrupt(unsigned long cycles)
{
	omap_dm_timer_set_load_start(*sched_timer, 0, 0xffffffff - cycles);
	return 0;
}

// Helper Function, programs the Sched Timer Interrupt
static int clkev_program_event(struct clock_event_device *dev, u64 delta)
{
     unsigned long long clc;
     int rc;

     if (delta <= 0)
        return -ETIME;

     delta = min(delta, (uint64_t) dev->max_delta_ns);
     delta = max(delta, (uint64_t) dev->min_delta_ns);

     clc = ((unsigned long long) delta * dev->mult) >> dev->shift;
     rc = qot_am335x_sched_interface_program_interrupt((unsigned long) clc);

     return rc;
}

// Interface function to qot_core, used to program the scheduler interface interrupt using a timepoint_t value
static long qot_am335x_program_sched_interrupt(timepoint_t expiry, int force, long (*callback)(void))
{
	int retval;
	unsigned long flags;
	struct qot_am335x_sched_interface *interface;
	u64 ns;
	u64 expiry_ns;
	struct qot_am335x_data *pdata;
	pdata = qot_am335x_data_ptr;
	interface = container_of(sched_timer, struct qot_am335x_sched_interface, timer);
	expiry_ns = TP_TO_nSEC(expiry);
	raw_spin_lock_irqsave(&pdata->lock, flags);
	ns = timecounter_read(&interface->parent->tc);
	raw_spin_unlock_irqrestore(&pdata->lock, flags);
	
	// Check if expiry is not behind current time Else return error code
	if(expiry_ns > ns + interface->qot_clockevent.min_delta_ns)
	{
		ns = expiry_ns - ns;
	}
	else
	{
		if(force == 1)
		{
			ns = interface->qot_clockevent.min_delta_ns;
		}
		else
		{
			return 1;
		}
	}
	interface->callback = callback;
	retval = clkev_program_event(&interface->qot_clockevent, ns);
	return retval;
}

// Interface function to qot_core, used to cancel a programmed interrupt
static long qot_am335x_cancel_sched_interrupt(void)
{
	omap_dm_timer_stop(*sched_timer);
	return 0;
}

// Sched Timer Interrupt
static irqreturn_t qot_am335x_sched_interface_interrupt(int irq, void *data)
{
	struct qot_am335x_sched_interface *interface = data;
	omap_dm_timer_write_status(interface->timer, OMAP_TIMER_INT_OVERFLOW);
	interface->callback();
	return IRQ_HANDLED;
}

// Configure the Scheduler Interface Timer
static int qot_am335x_core_sched(struct qot_am335x_sched_interface *interface, int event)
{
	struct omap_dm_timer *timer = interface->timer;
	switch (event) {
	case EVENT_START:
		pr_info("qot_am335x: Enable sched timer\n");
		omap_dm_timer_enable(timer);
		omap_dm_timer_set_prescaler(timer, AM335X_NO_PRESCALER);
		omap_dm_timer_set_int_enable(timer, OMAP_TIMER_INT_OVERFLOW);
		omap_dm_timer_enable(timer);

		pr_info("qot_am335x: Create Clockevent device\n");
		interface->qot_clockevent.min_delta_ticks = 3;
		interface->qot_clockevent.max_delta_ticks = 0xffffffff;
		pr_info("qot_am335x: Configure Clockevent device\n");
		clkev_configure(&interface->qot_clockevent, timer->rate);
		break;
	case EVENT_STOP:
		omap_dm_timer_set_int_disable(timer, OMAP_TIMER_INT_OVERFLOW);
		omap_dm_timer_enable(timer);
		omap_dm_timer_stop(timer);
		break;
	}

	return 0;
}

// POWER MANAGEMENT FUNCTIONS /////////////////////////////////////////////////////////

// Puts the Oscillator into Sleep Mode -> this is a stub
static long qot_am335x_sleep(void)
{
	return 0;
}

// Restarts the oscillator from sleep -> this is a stub
static long qot_am335x_wake(void)
{
	return 0;
}


// DEVICE TREE PARSING /////////////////////////////////////////////////////////

static void qot_am335x_cleanup(struct qot_am335x_data *pdata)
{
	pr_info("qot_am335x: Cleaning up...\n");
	if (pdata) {
		int i;

		/* Remove the PTP clock */
		ptp_clock_unregister(pdata->clock);

		/* Free GPIO timers */
		for (i = 0; i < pdata->num_pins; i++) {

			/* Stop the timer */
			switch(pdata->pins[i].state.type) {
			case PTP_CLK_REQ_EXTTS:
				qot_am335x_extts(&pdata->pins[i], EVENT_STOP);
				break;
			case PTP_CLK_REQ_PEROUT:
				qot_am335x_perout(&pdata->pins[i], EVENT_STOP);
				break;
			default:
				continue;
			}

			/* Destroy the timer */
			omap_dm_timer_set_source(pdata->pins[i].timer,
				OMAP_TIMER_SRC_SYS_CLK);
			free_irq(pdata->pins[i].timer->irq, &pdata->pins[i]);
			omap_dm_timer_enable(pdata->pins[i].timer);
			omap_dm_timer_stop(pdata->pins[i].timer);
			omap_dm_timer_free(pdata->pins[i].timer);
			pdata->pins[i].timer = NULL;

			/* Free the GPIO descriptor */
			gpiod_put(pdata->pins[i].gpiod);
		}

		/* Free core timer */
		if (pdata->core.timer) {
			qot_am335x_core(&pdata->core, EVENT_STOP);
		    omap_dm_timer_set_source(pdata->core.timer, OMAP_TIMER_SRC_SYS_CLK);
			free_irq(pdata->core.timer->irq, &pdata->core);
			omap_dm_timer_enable(pdata->core.timer);
			omap_dm_timer_stop(pdata->core.timer);
			omap_dm_timer_free(pdata->core.timer);
		}

		/* Free core sched timer */
		if (pdata->core_sched.timer) {
			qot_am335x_core_sched(&pdata->core_sched, EVENT_STOP);
			free_irq(pdata->core_sched.timer->irq, &pdata->core_sched);
			omap_dm_timer_free(pdata->core_sched.timer);
		}

		/* Free pin cofniguration */
		if (pdata->info.pin_config)
		{
			kfree(pdata->info.pin_config);
			pdata->info.pin_config = NULL;
		}
		if (pdata->pins)
		{
			kfree(pdata->pins);
			pdata->pins = NULL;
		}

		/* Free platform data */
		// kfree(pdata);
		// pdata = NULL;
	}
}

// QoT PLATFORM CLOCK ABSTRACTIONS //////////////////////////////////////////////////////////////
struct qot_clock qot_am335x_properties = { // Does not contain accurate values meeds to be filled TODO
	.name = "qot_am335x",
	.nom_freq_nhz = 24000000000,
	.nom_freq_nwatt = 1000,
};

struct qot_clock_impl qot_am335x_impl_info = {
	.read_time = qot_am335x_read_time,
	.program_interrupt =  qot_am335x_program_sched_interrupt,
	.cancel_interrupt = qot_am335x_cancel_sched_interrupt,
	.enable_compare = qot_am335x_timeline_enable_compare,
	.sleep = qot_am335x_sleep,
	.wake = qot_am335x_wake
};

/// DEVICE TREE PARSING
static struct qot_am335x_data *qot_am335x_of_parse(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct fwnode_handle *child;
	struct of_phandle_args args;
	struct device_node *nodec, *nodet, *nodeb;
	int count, timer_source;
	unsigned long flags;
	const __be32 *phand;
	const char *tmp;
	struct qot_am335x_data *pdata = NULL;

	/* Try allocate platform data */
	pr_info("qot_am335x: Allocating platform data...\n");
	pdata = devm_kzalloc(&pdev->dev,
		sizeof(struct qot_am335x_data), GFP_KERNEL);
	if (!pdata)
		goto err;

	/* Initialize a Global Variable for easy reference later */
	qot_am335x_data_ptr = pdata;

	/* Initialize spin lock for protecting time registers */
	pr_info("qot_am335x: Initializing spinlock...\n");
	raw_spin_lock_init(&pdata->lock);

	/* Setup core timer */
	nodec = pdev->dev.of_node;
	phand = of_get_property(nodec, "core", NULL);
	if (!phand) {
		pr_err("qot_am335x: could not find phandle for core");
		goto err;
	}
	if (of_parse_phandle_with_fixed_args(nodec, "core", 1, 0, &args) < 0) {
		pr_err("qot_am335x: could not parse core timer arguments\n");
		goto err;
	}
	switch (args.args[0]) {
	default:
	case 0: timer_source = OMAP_TIMER_SRC_SYS_CLK; 	break;
	case 1: timer_source = OMAP_TIMER_SRC_32_KHZ; 	break;
	case 2: timer_source = OMAP_TIMER_SRC_EXT_CLK; 	break;
	}
	nodet = (struct device_node*) of_find_node_by_phandle(be32_to_cpup(phand));
	if (!nodet) {
		pr_err("qot_am335x: could not find the timer node\n");
		goto err;
	}
	of_property_read_string_index(nodet, "ti,hwmods", 0, &tmp);
	if (!tmp) {
		pr_err("qot_am335x: ti,hwmods property missing?\n");
		goto err;
	}
	pdata->core.parent = pdata;
	pdata->core.state.type = PTP_CLK_REQ_PPS; /* Flag indicates core */
	pdata->core.timer = omap_dm_timer_request_by_node(nodet);
	if (!pdata->core.timer) {
		pr_err("qot_am335x: request_by_node failed\n");
		goto err;
	}
	if (request_irq(pdata->core.timer->irq, qot_am335x_interrupt, IRQF_TIMER,
		MODULE_NAME, &pdata->core)) {
		pr_err("qot_am335x: cannot register core IRQ");
		goto err;
	}
	of_node_put(nodet);

	/* Setup clock source for core */
	pr_info("qot_am335x: Configuring core timer...\n");
	omap_dm_timer_set_source(pdata->core.timer, OMAP_TIMER_SRC_SYS_CLK);
	qot_am335x_core(&pdata->core, EVENT_START);

	/* Create a time counter */
	pr_info("qot_am335x: Creating core time counter...\n");
	pdata->cc.read = qot_am335x_read;
	pdata->cc.mask = CLOCKSOURCE_MASK(32);
	pdata->cc_mult = 2796202667UL;
	pdata->cc.mult = 2796202667UL;
	pdata->cc.shift = 26;
	raw_spin_lock_irqsave(&pdata->lock, flags);
	timecounter_init(&pdata->tc, &pdata->cc, ktime_to_ns(ktime_get_real()));
	raw_spin_unlock_irqrestore(&pdata->lock, flags);

	/* Register a Timer to drive the Scheduler Interface */
	phand = of_get_property(nodec, "sched", NULL);
	if (!phand) {
		pr_err("qot_am335x: could not find phandle for sched");
		goto err;
	}
	if (of_parse_phandle_with_fixed_args(nodec, "sched", 1, 0, &args) < 0) {
		pr_err("qot_am335x: could not parse sched timer arguments\n");
		goto err;
	}
	switch (args.args[0]) {
	default:
	case 0: timer_source = OMAP_TIMER_SRC_SYS_CLK; 	break;
	case 1: timer_source = OMAP_TIMER_SRC_32_KHZ; 	break;
	case 2: timer_source = OMAP_TIMER_SRC_EXT_CLK; 	break;
	}
	nodeb = (struct device_node*) of_find_node_by_phandle(be32_to_cpup(phand));
	if (!nodeb) {
		pr_err("qot_am335x: could not find the timer node\n");
		goto err;
	}
	of_property_read_string_index(nodeb, "ti,hwmods", 0, &tmp);
	if (!tmp) {
		pr_err("qot_am335x: ti,hwmods property missing?\n");
		goto err;
	}

	pdata->core_sched.parent = pdata;
	pr_info("qot_am335x: Initializing sched timer...\n");
	// pdata->core_sched.timer = omap_dm_timer_request_by_cap(OMAP_TIMER_ALWON);
	pdata->core_sched.timer = omap_dm_timer_request_by_node(nodeb);
    //pdata->core_sched.timer = omap_dm_timer_request_by_cap(OMAP_TIMER_HAS_PWM);
    pr_info("qot_am335x: timer chosen as sched timer...\n");

	if (!pdata->core_sched.timer) {
			pr_err("qot_am335x: request_by_cap failed for sched_timer initialization\n");
			goto err;
	}
	if (request_irq(pdata->core_sched.timer->irq, qot_am335x_sched_interface_interrupt, IRQF_TIMER | IRQF_IRQPOLL, MODULE_NAME, &pdata->core_sched)) {
		pr_err("qot_am335x: cannot register core sched_timer IRQ");
		goto err;
	}

	of_node_put(nodeb);
	sched_timer = &pdata->core_sched.timer;


	/* Setup clock source for core sched timer*/
	pr_info("qot_am335x: Configuring core sched_timer...\n");
	omap_dm_timer_set_source(pdata->core_sched.timer, OMAP_TIMER_SRC_SYS_CLK);
	pdata->core_sched.timer->rate = clk_get_rate(pdata->core_sched.timer->fclk);
	pdata->core_sched.timer->reserved = 1;
	qot_am335x_core_sched(&pdata->core_sched, EVENT_START);

	/* Get the number of children = number of PTP pins */
	count = device_get_child_node_count(dev);		// One pin cannot be used due to the sched timer
	if (!count)
	{
		pr_info("qot_am335x: number of pins zero\n");
		return ERR_PTR(-ENODEV);
	}
	pdata->info = qot_am335x_info;

	pr_info("qot_am335x: Configuring %d pins...\n", count);


	/* Allocate pins and config */
	pr_info("qot_am335x: Allocating memory for  pins...\n");
	pdata->pins = devm_kzalloc(dev,
		count*sizeof(struct qot_am335x_channel), GFP_KERNEL);
	if (!pdata->pins)
		return ERR_PTR(-ENOMEM);
	pdata->info.pin_config =
		devm_kzalloc(dev, count*sizeof(struct ptp_pin_desc), GFP_KERNEL);
	if (!pdata->info.pin_config)
		return ERR_PTR(-ENOMEM);

	/* Initialize the pins */
	pdata->num_pins = 0;
	device_for_each_child_node(dev, child) {

		/* Get the Label Information */
		tmp = "unnamed";
		if (fwnode_property_present(child, "label"))
			fwnode_property_read_string(child, "label", &tmp);
		if(strcmp(tmp, "NOT_USED" ) == 0)
		{
			pr_info("qot_am335x: Redundant pin %s detected\n", tmp);
			if(pdata->num_pins == count - 1)
				break;
			else
				continue;
		}
		pr_info("qot_am335x: pin for %s being configured\n", tmp);

		strncpy(pdata->info.pin_config[pdata->num_pins].name,tmp,
			sizeof(pdata->info.pin_config[pdata->num_pins].name));
		tmp = NULL;


		/* Get the GPIO description */
		pdata->pins[pdata->num_pins].gpiod =
			devm_get_gpiod_from_child(dev, NULL, child);
		if (IS_ERR(pdata->pins[pdata->num_pins].gpiod)) {
			fwnode_handle_put(child);
			goto err;
		}

		/* Set the PTP name, function, channel and index of this GPIO */
		pdata->info.pin_config[pdata->num_pins].index = pdata->num_pins;
		pdata->info.pin_config[pdata->num_pins].chan = pdata->num_pins;
		pdata->info.pin_config[pdata->num_pins].func = PTP_PF_NONE;


		/* Create the timer */
#if LINUX_VERSION_CODE <= KERNEL_VERSION(4,1,12) // to check version for 'of_node()' vs 'to_of_node()' -sean kim
		nodec = of_node(child);
#else
		nodec = to_of_node(child);
#endif
		phand = of_get_property(nodec, "timer", NULL);
		nodet = (struct device_node*) of_find_node_by_phandle(be32_to_cpup(phand));
		of_property_read_string_index(nodet, "ti,hwmods", 0, &tmp);
		if (!tmp) {
			pr_err("qot_am335x: ti,hwmods property missing?\n");
			goto err;
		}
		pdata->pins[pdata->num_pins].parent = pdata;
		pdata->pins[pdata->num_pins].timer =
			omap_dm_timer_request_by_node(nodet);
		if (!pdata->pins[pdata->num_pins].timer) {
			pr_err("qot_am335x: request_by_node failed\n");
			goto err;
		}
		/* Set the timer ID -> Changes with the Device Tree */
		pdata->pins[pdata->num_pins].timer->id = pdata->num_pins + PHC2PHC_TIMER;

		/* Request and interrupt for the timer */
		if (request_irq(pdata->pins[pdata->num_pins].timer->irq,
			qot_am335x_interrupt, IRQF_TIMER, MODULE_NAME,
				&pdata->pins[pdata->num_pins])) {
			pr_err("qot_am335x: cannot register IRQ");
			goto err;
		}

		/* Configure the timer and set input clock source */
		omap_dm_timer_set_source(pdata->pins[pdata->num_pins].timer,
			OMAP_TIMER_SRC_SYS_CLK);
		qot_am335x_extts(&pdata->pins[pdata->num_pins], EVENT_STOP);

		/* Print some debug information */
		pr_info("qot_am335x: Pin %s registered on GPIO %d...\n",
			pdata->info.pin_config[pdata->num_pins].name,
				desc_to_gpio(pdata->pins[pdata->num_pins].gpiod));

		/* Next pin */
		pdata->num_pins++;
	}
	count = pdata->num_pins;
	pdata->info.n_pins = count;
	pdata->info.n_ext_ts = count;
	pdata->info.n_per_out = count;
	pr_info("qot_am335x: Configured %d pins...\n", count);

	/* Initialize a PTP clock */
	pr_info("qot_am335x: Initializing PTP clock...\n");
	pdata->clock = ptp_clock_register(&pdata->info, &pdev->dev);
	if (IS_ERR(pdata->clock)) {
		pr_err("qot_am335x: problem creating PTP interface\n");
		goto err;
	}

	qot_am335x_properties.phc_id = ptp_clock_index(pdata->clock);
	
	/* Initialize key for output compare */
	pdata->compare.key = 0;
	pdata->compare.perout.owner_file = NULL;
	pdata->compare.reprogram_flag = 0;
	pdata->compare.callback = NULL; // Initialize to NULL

	/* Return the platform data */
	return pdata;

err:
	pr_err("Cannot parse device tree\n");
	qot_am335x_cleanup(pdata);
	return NULL;
}



// MODULE LOADING AND UNLOADING  ///////////////////////////////////////////////

static const struct of_device_id qot_am335x_dt_ids[] = {
	{ .compatible = "qot_am335x", },
	{ }
};

MODULE_DEVICE_TABLE(of, qot_am335x_dt_ids);

static int qot_am335x_probe(struct platform_device *pdev)
{
	timelength_t read_lat_estimate;		/* Estimate of Read Latency */
	timeinterval_t read_lat_interval;	/* Uncertainty interval around Read Latency */
	timelength_t int_lat_estimate;		/* Estimate of Interrupt Latency */
	timeinterval_t int_lat_interval;	/* Uncertainty interval around Interrupt Latency */

	const struct of_device_id *match = of_match_device(
    	qot_am335x_dt_ids, &pdev->dev);
	if (match) {
		if (pdev->dev.platform_data)
			qot_am335x_cleanup(pdev->dev.platform_data);
		pdev->dev.platform_data = qot_am335x_of_parse(pdev);
		if (!pdev->dev.platform_data)
			return -ENODEV;
	} else {
		pr_err("of_match_device failed\n");
		return -ENODEV;
	}
	// TODO: Values are hard coded and arbitrary, change them
	/* Clock Read Latency */
	read_lat_estimate.sec = 0;
	read_lat_estimate.asec = 10*ASEC_PER_NSEC;             
	read_lat_interval.below.sec = 0;
	read_lat_interval.below.asec = 1*ASEC_PER_NSEC;        
	read_lat_interval.above.sec = 0;
	read_lat_interval.above.asec = 1*ASEC_PER_NSEC;
	
	/* Interrupt Read Latency */
	int_lat_estimate.sec = 0;
	int_lat_estimate.asec = 10*ASEC_PER_NSEC;
	int_lat_interval.below.sec = 0;
	int_lat_interval.below.asec = 1*ASEC_PER_NSEC;
	int_lat_interval.above.sec = 0;
	int_lat_interval.above.asec = 1*ASEC_PER_NSEC;
	
	/* QoT Clock Properties */ 
	qot_am335x_properties.read_latency.estimate = read_lat_estimate;
	qot_am335x_properties.read_latency.interval = read_lat_interval;
	
	qot_am335x_properties.interrupt_latency.estimate = int_lat_estimate;
	qot_am335x_properties.interrupt_latency.interval = int_lat_interval;

	qot_am335x_impl_info.info = qot_am335x_properties;

	/* QoT PTP Clock Info */
	qot_am335x_impl_info.ptpclk = qot_am335x_info;
	if(qot_register(&qot_am335x_impl_info))
		return -EACCES;
	return 0;
}

static int qot_am335x_remove(struct platform_device *pdev)
{
	if(qot_unregister(&qot_am335x_impl_info))
		return -EACCES;
	if (pdev->dev.platform_data) {
		qot_am335x_cleanup(pdev->dev.platform_data);
		devm_kfree(&pdev->dev, pdev->dev.platform_data);
		pdev->dev.platform_data = NULL;
	}
	platform_set_drvdata(pdev, NULL);
	return 0;
}

static struct platform_driver qot_am335x_driver = {
	.probe    = qot_am335x_probe,
	.remove   = qot_am335x_remove,
	.driver   = {
		.name = MODULE_NAME,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(qot_am335x_dt_ids),
	},
};

module_platform_driver(qot_am335x_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andrew Symington");
MODULE_DESCRIPTION("Linux PTP Driver for AM335x");
MODULE_VERSION("0.5.0");

