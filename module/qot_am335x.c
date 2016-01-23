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

/* Useful definitions */
#define MODULE_NAME 			"qot_am335x"
#define AM335X_REWRITE_DELAY 	13
#define AM335X_NO_PRESCALER 	0xFFFFFFFF
#define MIN_PERIOD_NS			1000ULL
#define MAX_PERIOD_NS			10000000000ULL

// PLATFORM DATA ///////////////////////////////////////////////////////////////

struct qot_am335x_data;

struct qot_am335x_channel {
	struct qot_am335x_data *parent;			/* Pointer to parent */
	struct omap_dm_timer *timer;			/* OMAP Timer */
	struct ptp_clock_request state;			/* PTP state */
	struct gpio_desc *gpiod;				/* GPIO description */
	int first;								/* Are we in PWM first edge */
};

struct qot_am335x_data {
	spinlock_t lock;						/* Protects timer registers */
	struct qot_am335x_channel  core;		/* Timer channel (core) */
	struct qot_am335x_channel *pins;		/* Timer channel (GPIO) */
	int num_pins;							/* Number of pins */
	struct timecounter tc;					/* Time counter */
	struct cyclecounter cc;					/* Cycle counter */
	u32 cc_mult; 							/* f0 */
	struct ptp_clock *clock;				/* PTP clock */
	struct ptp_clock_info info;				/* PTP clock info */
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
	return t->sec * 1000000000LL + t->nsec;
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
		break;
	case EVENT_STOP:
		omap_dm_timer_stop(timer);
		break;
	}

	return 0;
}

static int qot_am335x_perout(struct qot_am335x_channel *channel, int event)
{
	s64 ts, tp;
	u32 load, match, tc, offset;
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

		break;
	
	case EVENT_OVERFLOW:
	
		break;

	case EVENT_START:

		/* Get the signed ns start time and period */
		ts = ptp_to_s64(&channel->state.perout.start);
		tp = ptp_to_s64(&channel->state.perout.period);

		/* Some basic period checks for sanity */
		if (tp < MIN_PERIOD_NS || tp > MAX_PERIOD_NS)
			return -EINVAL;

		
		spin_lock_irqsave(&channel->parent->lock, flags);

		/* Work out the cycle count corresponding to this edge */
		ts = ts - channel->parent->tc.nsec;
		ts = div_u64((ts << channel->parent->cc.shift)
			+ channel->parent->tc.frac, channel->parent->cc.mult);
		tc = ts;
		tc = tc + channel->parent->tc.cycle_last;

		/* Use the cyclecounter mult at shift to scale */
		tp = div_u64((tp << channel->parent->cc.shift)
			+ channel->parent->tc.frac, channel->parent->cc.mult);

		spin_unlock_irqrestore(&channel->parent->lock, flags);

		/* Map to an offset, load and match */
		offset = tc; 		// (start)
		load   = tp; 		// (low+high)
		match  = tp/2; 		// (low)

		pr_info("qot_am335x: PWM offset...%u\n",offset);
		pr_info("qot_am335x: PWM period...%u\n",load);

		/* Configure timer */
		omap_dm_timer_enable(timer);
		omap_dm_timer_set_prescaler(timer, AM335X_NO_PRESCALER);
		omap_dm_timer_set_pwm(timer, 1, 1, 
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
		memcpy(&pdata->pins[rq->extts.index].state, rq, 
			sizeof(struct ptp_clock_request));
		return qot_am335x_extts(&pdata->pins[rq->extts.index], 
			(on ? EVENT_START : EVENT_STOP));
	case PTP_CLK_REQ_PEROUT:
		memcpy(&pdata->pins[rq->perout.index].state, rq, 
			sizeof(struct ptp_clock_request));
		return qot_am335x_perout(&pdata->pins[rq->perout.index], 
			(on ? EVENT_START : EVENT_STOP));
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

// DEVICE TREE PARSING /////////////////////////////////////////////////////////

static void qot_am335x_cleanup(struct qot_am335x_data *pdata)
{
	pr_info("qot_am335x: Cleaning up...\n");
	if (pdata) {
		int i;
		for (i = 0; i < pdata->num_pins; i++) {
		    omap_dm_timer_set_source(pdata->pins[i].timer, 
		    	OMAP_TIMER_SRC_SYS_CLK); // TCLKIN is stopped during boot
			omap_dm_timer_set_int_disable(pdata->pins[i].timer, 
				OMAP_TIMER_INT_CAPTURE | OMAP_TIMER_INT_OVERFLOW);
			free_irq(pdata->pins[i].timer->irq, &pdata->pins[i]);
			omap_dm_timer_free(pdata->pins[i].timer);
			pdata->pins[i].timer = NULL;
		}
		if (pdata->info.pin_config)
			kfree(pdata->info.pin_config);
		if (pdata->pins)
			kfree(pdata->pins);
		kfree(pdata);
	}
}

static struct qot_am335x_data *qot_am335x_of_parse(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct fwnode_handle *child;
	struct of_phandle_args args;
	struct device_node *nodec, *nodet;
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

	/* Initialize spin lock for protecting time registers */
	pr_info("qot_am335x: Initializing spinlock...\n");
	spin_lock_init(&pdata->lock);

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
	nodet = of_find_node_by_phandle(be32_to_cpup(phand));
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
	pdata->core.timer = omap_dm_timer_request_by_node(nodet);
	if (!pdata->core.timer) {
		pr_err("qot_am335x: request_by_node failed\n");
		goto err;
	}
	switch (args.args[0]) {
	default:
	case 0: timer_source = OMAP_TIMER_SRC_SYS_CLK; 	break;
	case 1: timer_source = OMAP_TIMER_SRC_32_KHZ; 	break;
	case 2: timer_source = OMAP_TIMER_SRC_EXT_CLK; 	break;
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
	pdata->cc.mult = 174762667;
	pdata->cc.shift = 22;
	spin_lock_irqsave(&pdata->lock, flags);
	timecounter_init(&pdata->tc, &pdata->cc, ktime_to_ns(ktime_get_real()));
	spin_unlock_irqrestore(&pdata->lock, flags);

	/* Get the number of children = number of PTP pins */
	count = device_get_child_node_count(dev);
	if (!count)
		return ERR_PTR(-ENODEV);
	pdata->info = qot_am335x_info;
	pdata->info.n_pins = count;
	pdata->info.n_ext_ts = count;
	pdata->info.n_per_out = count;
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
		tmp = "unnamed";
		if (fwnode_property_present(child, "label"))
			fwnode_property_read_string(child, "label", &tmp);
		strncpy(pdata->info.pin_config[pdata->num_pins].name,tmp,
			sizeof(pdata->info.pin_config[pdata->num_pins].name));
		tmp = NULL;

		/* Create the timer */
		nodec = of_node(child);
		phand = of_get_property(nodec, "timer", NULL);
		nodet = of_find_node_by_phandle(be32_to_cpup(phand));
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

	/* Initialize a PTP clock */
	pr_info("qot_am335x: Initializing PTP clock...\n");
	pdata->clock = ptp_clock_register(&pdata->info, &pdev->dev);
	if (IS_ERR(pdata->clock)) {
		pr_err("qot_am335x: problem creating PTP interface\n");
		goto err;
	}

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
	return 0;
}

static int qot_am335x_remove(struct platform_device *pdev)
{
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