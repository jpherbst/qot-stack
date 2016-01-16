/*
 * @file qot_am335x.c
 * @brief Driver that exposes AM335x subsystem as a PTP clock for the QoT stack
 * @author Andrew Symington
 *
 * Copyright (c) Regents of the University of California, 2015. 
 *   All rights reserved.
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
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/clocksource.h>
#include <linux/timecounter.h>

/* We are going to export the whol am335x as a PTP clock */
#include <linux/ptp_clock_kernel.h>

/* DMTimer Code specific to the AM335x */
#include "/export/bb-kernel/KERNEL/arch/arm/plat-omap/include/plat/dmtimer.h"

/* Different event calls for GPIO */
enum qot_am335x_events {
	EVENT_MATCH,
	EVENT_OVERFLOW,
	EVENT_START,
	EVENT_STOP
};

/* Latency between a single timer counter read and write action */
#define MODULE_NAME 			"qot_am335x"
#define AM335X_KLUDGE_FACTOR 	13

// PLATFORM DATA ///////////////////////////////////////////////////////////////

struct qot_am335x_data;

struct qot_am335x_channel {
	struct omap_dm_timer *timer;			/* Timer */
	struct qot_am335x_data *parent;			/* Pointer to parent */
	struct ptp_clock_request state;			/* Pin state */
};

struct qot_am335x_data {
	spinlock_t lock;						/* Protects timer registers */
	struct qot_am335x_channel core;			/* OMAP timers (core) */
	struct qot_am335x_channel gpio[4];		/* OMAP timers (GPIO) */
	struct timecounter tc;					/* Time counter */
	struct cyclecounter cc;					/* Cycle counter */
	u32 cc_mult; 							/* f0 */
	struct ptp_clock *clock;				/* PTP clock */
	struct ptp_clock_info info;				/* PTP clock info */
};

// TIMER MANAGEMENT ////////////////////////////////////////////////////////////

static void qot_am335x_core(struct qot_am335x_channel *channel, int event)
{
	u32 ctrl;
	struct omap_dm_timer *timer = channel->timer;
	switch (event) {
	case EVENT_START:
		omap_dm_timer_enable(timer);
		ctrl = __omap_dm_timer_read(timer, OMAP_TIMER_CTRL_REG, timer->posted);
		ctrl &= ~(OMAP_TIMER_CTRL_PRE | (0x07 << 2));
		ctrl |= OMAP_TIMER_CTRL_AR;
		__omap_dm_timer_write(timer, OMAP_TIMER_LOAD_REG, 0, timer->posted);
		ctrl |= OMAP_TIMER_CTRL_ST;
		__omap_dm_timer_load_start(timer, ctrl, 0, timer->posted);
		timer->context.tclr = ctrl;
		timer->context.tldr = 0;
		timer->context.tcrr = 0;
		break;
	case EVENT_STOP:
		omap_dm_timer_disable(timer);
		break;
	}
}

static void qot_am335x_perout(struct qot_am335x_channel *channel, int event)
{
	u32 ctrl, value;
	struct omap_dm_timer *timer = channel->timer;
	switch (event) {
	case EVENT_MATCH:
		break;
	case EVENT_OVERFLOW:
		break;
	case EVENT_START:
		omap_dm_timer_enable(timer);
		__omap_dm_timer_write(timer, OMAP_TIMER_LOAD_REG, 0, timer->posted);
		ctrl = __omap_dm_timer_read(timer, OMAP_TIMER_CTRL_REG, timer->posted);
		ctrl &= ~(OMAP_TIMER_CTRL_PRE | (0x07 << 2));
		ctrl |= (OMAP_TIMER_CTRL_AR|OMAP_TIMER_CTRL_ST|OMAP_TIMER_CTRL_GPOCFG);
		__omap_dm_timer_load_start(timer, ctrl, 0, timer->posted);
		value = __omap_dm_timer_read(channel->parent->core.timer,
			OMAP_TIMER_COUNTER_REG, channel->parent->core.timer->posted);
		__omap_dm_timer_write(timer, OMAP_TIMER_COUNTER_REG,
			timer->context.tcrr + AM335X_KLUDGE_FACTOR, timer->posted);
		timer->context.tclr = ctrl;
		timer->context.tldr = 0;
		timer->context.tcrr = value;
		break;
	case EVENT_STOP:
		omap_dm_timer_disable(timer);
		break;
	}
}

static void qot_am335x_extts(struct qot_am335x_channel *channel, int event)
{
	u32 ctrl, value, irqcfg;
	struct omap_dm_timer *timer = channel->timer;
	switch (event) {
	case EVENT_START:

		/* Determine interrupt polarity */
		irqcfg = 0;
		if (   (channel->state.extts.flags & PTP_RISING_EDGE)
			&& (channel->state.extts.flags & PTP_FALLING_EDGE))
			irqcfg |= OMAP_TIMER_CTRL_TCM_BOTHEDGES;
		else if (channel->state.extts.flags & PTP_RISING_EDGE)
			irqcfg |= OMAP_TIMER_CTRL_TCM_LOWTOHIGH;
		else if (channel->state.extts.flags & PTP_FALLING_EDGE)
			irqcfg |= OMAP_TIMER_CTRL_TCM_HIGHTOLOW;

		/* Setup capture on the timer pin */
		omap_dm_timer_enable(timer);
		__omap_dm_timer_write(timer, OMAP_TIMER_LOAD_REG, 0, timer->posted);
		ctrl = __omap_dm_timer_read(timer, OMAP_TIMER_CTRL_REG, timer->posted);
		ctrl &= ~(OMAP_TIMER_CTRL_PRE | (0x07 << 2));
		ctrl |= (OMAP_TIMER_CTRL_AR | OMAP_TIMER_CTRL_ST);
		ctrl |= (irqcfg | OMAP_TIMER_CTRL_GPOCFG);
		__omap_dm_timer_load_start(timer, ctrl, 0, timer->posted);
		value = __omap_dm_timer_read(channel->parent->core.timer,
			OMAP_TIMER_COUNTER_REG, channel->parent->core.timer->posted);
		__omap_dm_timer_write(timer, OMAP_TIMER_COUNTER_REG,
			timer->context.tcrr + AM335X_KLUDGE_FACTOR, timer->posted);
	    timer->context.tclr = ctrl;
		timer->context.tldr = 0;
		timer->context.tcrr = value;
		
		break;

	case EVENT_STOP:

		/* Disable the interrupt timer */
		omap_dm_timer_disable(timer);
	
		break;
	}
}

static cycle_t qot_am335x_read(const struct cyclecounter *cc)
{
	cycle_t val;
	struct qot_am335x_data *pdata = container_of(
	                                    cc, struct qot_am335x_data, cc);
	return __omap_dm_timer_read_counter(pdata->core.timer,
	                                    pdata->core.timer->posted);
	return val;
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
	spin_lock_irqsave(&channel->parent->lock, flags);
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
		}
		break;
	case PTP_CLK_REQ_PEROUT:
		if (irq_status & OMAP_TIMER_INT_MATCH)
			qot_am335x_perout(channel, EVENT_MATCH);
		if (irq_status & OMAP_TIMER_INT_OVERFLOW)
			qot_am335x_perout(channel, EVENT_OVERFLOW);
		break;
	case PTP_CLK_REQ_PPS:
		if (	(irq_status & OMAP_TIMER_INT_MATCH)
		        || 	(irq_status & OMAP_TIMER_INT_OVERFLOW))
			qot_am335x_overflow(channel);
		break;
	}
	__omap_dm_timer_write_status(channel->timer, irq_status);
	spin_unlock_irqrestore(&channel->parent->lock, flags);
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
	spin_lock_irqsave(&pdata->lock, flags);
	timecounter_read(&pdata->tc);
	pdata->cc.mult = neg_adj ? mult - diff : mult + diff;
	spin_unlock_irqrestore(&pdata->lock, flags);
	return 0;
}

static int qot_am335x_adjtime(struct ptp_clock_info *ptp, s64 delta)
{
	unsigned long flags;
	struct qot_am335x_data *pdata = container_of(
	                                    ptp, struct qot_am335x_data, info);
	spin_lock_irqsave(&pdata->lock, flags);
	timecounter_adjtime(&pdata->tc, delta);
	spin_unlock_irqrestore(&pdata->lock, flags);
	return 0;
}

static int qot_am335x_gettime(struct ptp_clock_info *ptp, struct timespec64 *ts)
{
	u64 ns;
	unsigned long flags;
	struct qot_am335x_data *pdata = container_of(
	                                    ptp, struct qot_am335x_data, info);
	spin_lock_irqsave(&pdata->lock, flags);
	ns = timecounter_read(&pdata->tc);
	spin_unlock_irqrestore(&pdata->lock, flags);
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
	spin_lock_irqsave(&pdata->lock, flags);
	timecounter_init(&pdata->tc, &pdata->cc, ns);
	spin_unlock_irqrestore(&pdata->lock, flags);
	return 0;
}

static int qot_am335x_enable(struct ptp_clock_info *ptp,
                             struct ptp_clock_request *rq, int on)
{
	struct qot_am335x_data *pdata = container_of(
        ptp, struct qot_am335x_data, info);
	switch (rq->type) {
	case PTP_CLK_REQ_EXTTS:
		memcpy(&pdata->gpio[rq->extts.index].state, rq, 
			sizeof(struct ptp_clock_request));
		if (on)
			qot_am335x_extts(&pdata->gpio[rq->extts.index], EVENT_START);
		else
			qot_am335x_extts(&pdata->gpio[rq->extts.index], EVENT_STOP);
		return 0;
	case PTP_CLK_REQ_PEROUT:
		memcpy(&pdata->gpio[rq->perout.index].state, rq, 
			sizeof(struct ptp_clock_request));
		if (on)
			qot_am335x_perout(&pdata->gpio[rq->perout.index], EVENT_START);
		else
			qot_am335x_perout(&pdata->gpio[rq->perout.index], EVENT_STOP);
		return 0;
	default:
		break;
	}
	return -EOPNOTSUPP;
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

static struct ptp_pin_desc qot_am335x_pins[4] = {
	{
		.name = "AM335X_GPIO0",
		.index = 0,
		.func = PTP_PF_NONE,
		.chan = 0
	},
	{
		.name = "AM335X_GPIO1",
		.index = 1,
		.func = PTP_PF_NONE,
		.chan = 1
	},
	{
		.name = "AM335X_GPIO2",
		.index = 2,
		.func = PTP_PF_NONE,
		.chan = 2
	},
	{
		.name = "AM335X_GPIO3",
		.index = 3,
		.func = PTP_PF_NONE,
		.chan = 3
	}
};

static struct ptp_clock_info qot_am335x_info = {
	.owner		= THIS_MODULE,
	.name		= "AM335x timer",
	.max_adj	= 1000000,
	.n_alarm    = 0,
	.n_ext_ts	= 4,
	.n_pins		= 4,
	.pps		= 0,
	.pin_config = qot_am335x_pins,
	.adjfreq	= qot_am335x_adjfreq,
	.adjtime	= qot_am335x_adjtime,
	.gettime64	= qot_am335x_gettime,
	.settime64	= qot_am335x_settime,
	.enable		= qot_am335x_enable,
	.verify     = qot_am335x_verify,
};

// DEVICE TREE PARSING /////////////////////////////////////////////////////////

static struct qot_am335x_data *qot_am335x_of_parse(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct device_node *timer_node;
	struct of_phandle_args timer_args;
	struct qot_am335x_data *pdata;
	const __be32 *phandle;
	const char *tmp = NULL;
	char name[128];
	int i, timer_source;
	unsigned long flags;

	/* Try allocate platform data */
	pdata = devm_kzalloc(&pdev->dev,
		sizeof(struct qot_am335x_data), GFP_KERNEL);
	if (!pdata)
		return NULL;

	/* Initialize spin lock for protecting time registers */
	spin_lock_init(&pdata->lock);

	/* Setup core timer */
	pr_info("qot_am335x: Setting up core timer...");
	phandle = of_get_property(np, "core", NULL);
	if (!phandle) {
		pr_err("qot_am335x: could not find phandle for core");
		goto problem;
	}
	if (of_parse_phandle_with_fixed_args(np, "core", 1, 0, &timer_args) < 0) {
		pr_err("qot_am335x: could not parse core timer arguments\n");
		goto problem;
	}
	timer_node = of_find_node_by_phandle(be32_to_cpup(phandle));
	if (!timer_node) {
		pr_err("qot_am335x: could not find the timer node\n");
		goto problem;
	}
	of_property_read_string_index(timer_node, "ti,hwmods", 0, &tmp);
	if (!tmp) {
		pr_err("qot_am335x: ti,hwmods property missing?\n");
		goto problem;
	}
	pdata->core.timer = omap_dm_timer_request_by_node(timer_node);
	if (!pdata->core.timer) {
		pr_err("qot_am335x: request_by_node failed\n");
		goto problem;
	}
	switch (timer_args.args[0]) {
	default:
	case 0: timer_source = OMAP_TIMER_SRC_SYS_CLK; 	break;
	case 1: timer_source = OMAP_TIMER_SRC_32_KHZ; 	break;
	case 2: timer_source = OMAP_TIMER_SRC_EXT_CLK; 	break;
	}

	/* Clean up device tree node */
	of_node_put(timer_node);

	/* Setup clock source for core */
	omap_dm_timer_set_source(pdata->core.timer, timer_source);

	/* Initialize the core timer */	
	qot_am335x_core(&pdata->core, EVENT_START);

	/* Initialize a PTP clock */
	pdata->info = qot_am335x_info;
	pdata->clock = ptp_clock_register(&pdata->info, &pdev->dev);
	if (IS_ERR(pdata->clock)) {
		pr_err("qot_am335x: problem creating PTP interface\n");
		goto problem;
	}

	/* Create a time counter */
	pdata->cc.read = qot_am335x_read;
	pdata->cc.mask = CLOCKSOURCE_MASK(32);
	pdata->cc.mult = 174762667;
	pdata->cc.shift = 22;

	/* Initialize a time counter time to near system time */
	spin_lock_irqsave(&pdata->lock, flags);
	timecounter_init(&pdata->tc, &pdata->cc, ktime_to_ns(ktime_get_real()));
	spin_unlock_irqrestore(&pdata->lock, flags);

	/* Initialize the four timers */
	for (i = 0; i < 4; i++) {

		/* Set the parent struct */
		pdata->gpio[i].parent = pdata;

		/* Try and find the node handle */
		sprintf(name, "gpio%d", i);
		phandle = of_get_property(np, name, NULL);
		if (!phandle) {
			pr_err("qot_am335x: cannot find phandle for gpio%d", i);
			continue;
		}
		timer_node = of_find_node_by_phandle(be32_to_cpup(phandle));
		tmp = NULL;
		of_property_read_string_index(timer_node, "ti,hwmods", 0, &tmp);
		if (!tmp) {
			pr_err("qot_am335x: ti,hwmods property missing?\n");
			continue;
		}
		pdata->gpio[i].timer = omap_dm_timer_request_by_node(timer_node);
		if (!pdata->gpio[i].timer) {
			pr_err("qot_am335x: request_by_node failed\n");
			continue;
		}

		/* Request an IRQ for this timer */
		if (request_irq(pdata->gpio[i].timer->irq, qot_am335x_interrupt,
            IRQF_TIMER, MODULE_NAME, &pdata->gpio[i])) {
			pr_err("qot_am335x: cannot register IRQ");
			continue;
		}

		/* Setup clock source */
		omap_dm_timer_set_source(pdata->gpio[i].timer, timer_source);

		/* Put the device tree node back */
		of_node_put(timer_node);
	}

	/* Return the platform data */
	return pdata;

problem:

	/* Make sure we free memory on bad init */
	devm_kfree(&pdev->dev, pdev->dev.platform_data);
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
	const struct of_device_id *match = of_match_device(
    	qot_am335x_dt_ids, &pdev->dev);
	if (match) {
		pdev->dev.platform_data = qot_am335x_of_parse(pdev);
		if (!pdev->dev.platform_data)
			return -ENODEV;
	} else {
		pr_err("of_match_device failed\n");
		return -ENODEV;
	}
	return 0;
}

static int qot_am335x_remove(struct platform_device *pdev)
{
	if (pdev->dev.platform_data) {
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

