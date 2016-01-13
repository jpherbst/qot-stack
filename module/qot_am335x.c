/*
 * @file qot_am335x.c
 * @brief QoT Driver for BBB-GPSDO in Linux 4.1.6
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

IMPORTANT : To use this driver you will need to use a special device tree overlay
            that sets up the pinmux correctly and injects the kernel module into
            the running system (see ROSELINE-QOT-00A0.dts) and Makefile. You will
            need to have run ./dtc-overlay.sh in thirdpart/bb.org-overlays first.

*/

// Sufficient to develop a device-tree based platform driver
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/clocksource.h>
#include <linux/workqueue.h>
#include <linux/timecounter.h>
#include <linux/rbtree.h>

// We are going to export the whol am335x as a PTP clock
#include <linux/ptp_clock_kernel.h>

// DMTimer Code specific to the AM335x
#include "/export/bb-kernel/KERNEL/arch/arm/plat-omap/include/plat/dmtimer.h"

// QoT kernel API
#include "qot.h"
#include "qot_core.h"

// PLATFORM DATA ///////////////////////////////////////////////////////////////

struct qot_am335x_pdata {
	spinlock_t lock;					/* Protects timer registers */
	struct ptp_clock *clk;				/* PTP clock */
	struct ptp_clock_info clkinfo;		/* PTP clock info */
	struct clocksource clksrc;			/* Linux Clock source */
};

// PTP ABSTRACTION /////////////////////////////////////////////////////////////

static struct ptp_pin_desc qot_am335x_pins[4] = {
	{
		.name = "AM335X_GPIO0",
		.index = 0,
		.func = PTP_PF_NONE | PTP_PF_PEROUT | PTP_PF_EXTTS,
		.chan = 0
	},
	{
		.name = "AM335X_GPIO1",
		.index = 1,
		.func = PTP_PF_NONE | PTP_PF_PEROUT | PTP_PF_EXTTS,
		.chan = 1
	},
	{
		.name = "AM335X_GPIO2",
		.index = 2,
		.func = PTP_PF_NONE | PTP_PF_PEROUT | PTP_PF_EXTTS,
		.chan = 2
	},
	{
		.name = "AM335X_GPIO3",
		.index = 3,
		.func = PTP_PF_NONE | PTP_PF_PEROUT | PTP_PF_EXTTS,
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
	.adjfreq	= qot_am335x_adjfreq,
	.adjtime	= qot_am335x_adjtime,
	.gettime64	= qot_am335x_gettime,
	.settime64	= qot_am335x_settime,
	.enable		= qot_am335x_enable,
};

static int qot_am335x_adjfreq(struct ptp_clock_info *ptp, s32 delta)
{
	u64 adj;
	u32 diff, tmr_add;
	int neg_adj = 0;
	struct qot_am335x *pdata = container_of(ptp, struct qot_am335x, info);
	if (ppb < 0) {
		neg_adj = 1;
		ppb = -ppb;
	}
	tmr_add = pdata->tmr_add;
	adj = tmr_add;
	adj *= ppb;
	diff = div_u64(adj, 1000000000ULL);
	tmr_add = neg_adj ? tmr_add - diff : tmr_add + diff;
	gfar_write(&pdata->regs->tmr_add, tmr_add);
	return 0;
}

static int qot_am335x_adjtime(struct ptp_clock_info *ptp, s64 delta)
{
	s64 now;
	unsigned long flags;
	struct qot_am335x_data *pdata = container_of(ptp, struct qot_am335x_data, info);

	spin_lock_irqsave(&pdata->lock, flags);

	now = tmr_cnt_read(pdata);
	now += delta;
	tmr_cnt_write(pdata, now);

	spin_unlock_irqrestore(&pdata->lock, flags);

	set_fipers(pdata);
	return 0;
}

static int qot_am335x_gettime64(struct ptp_clock_info *ptp,
                                struct timespec64 *ts)
{
	u64 ns;
	unsigned long flags;
	struct etsects *etsects = container_of(ptp, struct etsects, caps);

	spin_lock_irqsave(&etsects->lock, flags);

	ns = tmr_cnt_read(etsects);

	spin_unlock_irqrestore(&etsects->lock, flags);

	*ts = ns_to_timespec64(ns);

	return 0;
}

static int qot_am335x_settime64(struct ptp_clock_info *p,
                                const struct timespec64 *ts)
{
	u64 ns;
	unsigned long flags;
	struct etsects *etsects = container_of(ptp, struct etsects, caps);

	ns = timespec64_to_ns(ts);

	spin_lock_irqsave(&etsects->lock, flags);

	tmr_cnt_write(etsects, ns);
	set_fipers(etsects);

	spin_unlock_irqrestore(&etsects->lock, flags);
}

static int qot_am335x_enable(struct ptp_clock_info *ptp,
                             struct ptp_clock_request *rq, int on)
{
	struct am335x_data *pdata = container_of(ptp, struct am335x_data, info);
	unsigned long flags;
	u32 bit, mask;

	switch (rq->type) {

	// Configure input timestamping
	case PTP_CLK_REQ_EXTTS:
		switch (rq->extts.index) {
		case 0: case 1: case 2: case 3:
			if (qot_am335x_timer_extts(pdata->timer_gpio[rq->extts.index])
			    return -EINVAL;
			    break;
		    default:
				    return -EINVAL;
			}
		return 0;

	// Configure external interrupt generation
	case PTP_CLK_REQ_PEROUT:
	switch (rq->perout.index) {
		case 0: case 1: case 2: case 3:
			if (qot_am335x_timer_perout(pdata->timer_gpio[rq->perout.index])
			    return -EINVAL;
			if (on) {
				u64 start_ns, period_ns;
				start_ns = rq->perout.start.sec * 1000000000ULL
				           + rq->perout.start.nsec;
				period_ns = rq->perout.period.sec * 1000000000ULL
				            + rq->perout.period.nsec;
				if (qot_am335x_period_on(pdata->timer_gpio[rq->perout.index],
					                         start_ns, period_ns)
					    return -EINVAL;
				}
		break;

	//
	default:
			return -EINVAL;
	}
	return 0;

default:
	break;
}

return -EOPNOTSUPP;
}

static int qot_am335x_verify(struct ptp_clock_info *ptp, unsigned int pin,
                             enum ptp_pin_function func, unsigned int chan)
{
   	// Check that the pin exists
	if (pin >= ptp->n_pins || !ptp->pin_config)
        return -EINVAL;

	// User cannot change the channel
	if (chan != ptp->pin_config[pin].chan)
        return -EINVAL;

	// We dont support PHY sync
	if (func == PTP_PF_PHYSYNC)
        return -EINVAL;

	return 0;
}

// DEVICE TREE PARSING /////////////////////////////////////////////////////////

static struct qot_am335x_data *qot_am335x_of_parse(struct platform_device *pdev)
{
	struct qot_am335x_data *pdata = NULL;
	struct clk *gt_fclk;;

	// Try and allocate some memory
	pdata = devm_kzalloc(&pdev->dev, sizeof(struct qot_am335x_data), GFP_KERNEL);
	if (!pdata)
		return NULL;

	// Create the clocksource from which to drive linux
	gt_fclk = omap_dm_timer_get_fclk(pdata->timer_core);
	pdata->frequency = clk_get_rate(gt_fclk);
	pdata->clksrc.name = "qot";
	pdata->clksrc.rating = 299;
	pdata->clksrc.read = roseline_read_cycles_timer_a;
	pdata->clksrc.mask = CLOCKSOURCE_MASK(32);
	pdata->clksrc.flags = CLOCK_SOURCE_IS_CONTINUOUS;
	if (clocksource_register_hz(&pdata->clksrc, pdata->frequency))
		pr_err("Could not register clocksource %s\n", pdata->clksrc.name);
	else
		pr_info("clocksource: %s at %u Hz\n", pdata->clksrc.name, pdata->frequency);

}

static void roseline_clocksource_cleanup(struct roseline_clock_data *pdata)
{
	clocksource_unregister(&pdata->clksrc);
}

// Return th platform data
return pdata;
}


// MODULE LOADING AND UNLOADING  ///////////////////////////////////////////////

static const struct of_device_id qot_am335x_dt_ids[] = {
	{ .compatible = "qot_am335x", },
	{ }
};

MODULE_DEVICE_TABLE(of, qot_am335x_dt_ids);

static int qot_am335x_probe(struct platform_device *pdev)
{
	struct qot_am335x_pin *pin;
	struct rb_node *node;
	const struct of_device_id *match = of_match_device(
	                                       qot_am335x_dt_ids, &pdev->dev);
	if (match) {
		pdev->dev.platform_data = qot_am335x_of_parse(pdev);
		if (!pdata)
			return -ENODEV;
	} else {
		pr_err("of_match_device failed\n");
		return -ENODEV;
	}
	return 0;
}

static int qot_am335x_remove(struct platform_device *pdev)
{
	struct qot_am335x_pin *pin;
	struct rb_node *node;
	if (pdev->dev.platform_data) {
		clocksource_unregister(&pdata->clksrc);
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


/*

// FORWARD DELCLARATIONS ///////////////////////////////////////////////////////

// Module information
#define MODULE_NAME 			"qot_am335x"
#define AM335X_TYPE_CAPTURE 	(0)
#define AM335X_TYPE_COMPARE 	(1)
#define AM335X_PIN_MAXLEN 		(128)
#define AM335X_OVERFLOW_PERIOD 	(8*HZ)
#define AM335X_KLUDGE_FACTOR 	13

// Platform data
struct qot_am335x_data {
	int ready;							// Prevents interrupts from blocking initialization
	struct omap_dm_timer *timer;		// CORE: pointer to the dmtimer object
	struct timecounter tc;				// CORE: keeps track of time
	struct cyclecounter cc;				// CORE: counts short periods by scaling cycles
	struct delayed_work overflow_work;	// CORE: scheduled work for overflow housekeeping
	struct rb_root pin_root;			// Red-black tree of pins
	spinlock_t lock;					// Spinlock
	struct workqueue_struct *wq;		// WOrk queue for captures
};

// Basic information about a timer
struct qot_am335x_pin {
	char name[AM335X_PIN_MAXLEN];		// Human-readable name, like "core", or "timer4"
	struct omap_dm_timer *timer;		// Pointer to the OMAP dual-mode timer
	uint8_t type;						// Type of pin (0 = capture, 1 = compare, 2 = sys)
	uint8_t enable;				 	    // COMPARE: [0] = disable, [1] = enabled
	uint64_t start;						// COMPARE: Time of next event (clock ticks)
	uint32_t high;						// COMPARE: High cycle time (clock ticks)
	uint32_t low;				   		// COMPARE: Low cycle time (clock ticks)
	uint64_t repeat;					// COMPARE: Number of repetition (0 = infinite)
    struct rb_node node;				// Red-black tree is used to store timers based on id
};

struct qot_am335x_work {
	struct qot_am335x_pin *pin;			// Pointer to puin
	uint32_t edge;						// Edge value at interrupt
	struct work_struct work;			// Work
};

// We need to store a reference to the platform data to service incoming calls from QoT cotre
static struct qot_am335x_data *pdata = NULL;

//////////////////////////////// DATA STRUCTURE FUNCTIONS ////////////////////////////////////

// Search our data structure for a given timer
static struct qot_am335x_pin *qot_am335x_pin_search(struct rb_root *root, const char *name)
{
	int result;
	struct qot_am335x_pin *pin;
	struct rb_node *node = root->rb_node;
	while (node)
	{
		pin = container_of(node, struct qot_am335x_pin, node);
		result = strcmp(name, pin->name);
		if (result < 0)
			node = node->rb_left;
		else if (result > 0)
			node = node->rb_right;
		else
			return pin;
	}
	return NULL;
}

// Insert a pin into the red-black tree
static int qot_am335x_pin_insert(struct rb_root *root, struct qot_am335x_pin *data)
{
	int result;
	struct qot_am335x_pin *pin;
	struct rb_node **new = &(root->rb_node), *parent = NULL;
	while (*new)
	{
		pin = container_of(*new, struct qot_am335x_pin, node);
		result = strcmp(data->name, pin->name);
		parent = *new;
		if (result < 0)
			new = &((*new)->rb_left);
		else if (result > 0)
			new = &((*new)->rb_right);
		else
			return 0;
	}
	rb_link_node(&data->node, parent, new);
	rb_insert_color(&data->node, root);
	return 1;
}

// TIMER FUNCTIONS THAT SHOULD BE IN DMTIMER BUT ARE NOT /////////////////////////////

// Setup a timer pin to be core
static void omap_dm_timer_setup_core(struct omap_dm_timer *timer)
{
	uint32_t ctrl;

  	// Set the timer source
	omap_dm_timer_set_source(timer, OMAP_TIMER_SRC_SYS_CLK);
	omap_dm_timer_enable(timer);

  	// Read the control register
	ctrl = __omap_dm_timer_read(timer, OMAP_TIMER_CTRL_REG, timer->posted);

  	// Disable prescaler
	ctrl &= ~(OMAP_TIMER_CTRL_PRE | (0x07 << 2));

  	// Autoreload
	ctrl |= OMAP_TIMER_CTRL_AR;
	__omap_dm_timer_write(timer, OMAP_TIMER_LOAD_REG, 0, timer->posted);

  	// Start timer
	ctrl |= OMAP_TIMER_CTRL_ST;
	__omap_dm_timer_load_start(timer, ctrl, 0, timer->posted);

	// Save the context
	timer->context.tclr = ctrl;
	timer->context.tldr = 0;
	timer->context.tcrr = 0;
}

// Setup a timer pin to be in capture mode
static void omap_dm_timer_setup_capture(struct omap_dm_timer *timer, int timer_irqcfg)
{
	uint32_t ctrl;

  	// Set the timer source
	omap_dm_timer_set_source(timer, OMAP_TIMER_SRC_SYS_CLK);
	omap_dm_timer_enable(timer);

  	// Read the control register
	ctrl = __omap_dm_timer_read(timer, OMAP_TIMER_CTRL_REG, timer->posted);

  	// Disable prescaler
	ctrl &= ~(OMAP_TIMER_CTRL_PRE | (0x07 << 2));

  	// Autoreload
	ctrl |= OMAP_TIMER_CTRL_AR;
	__omap_dm_timer_write(timer, OMAP_TIMER_LOAD_REG, 0, timer->posted);

  	// Start timer
	ctrl |= OMAP_TIMER_CTRL_ST;

	// Set capture
	ctrl |= timer_irqcfg | OMAP_TIMER_CTRL_GPOCFG;
	__omap_dm_timer_load_start(timer, ctrl, 0, timer->posted);

	// Save the context
	timer->context.tclr = ctrl;
	timer->context.tldr = 0;
	timer->context.tcrr = 0;
}

// Set a timer pin to be in compare mode
static void omap_dm_timer_setup_compare(struct omap_dm_timer *timer)
{
	uint32_t ctrl;

  	// Set the timer source
	omap_dm_timer_set_source(timer, OMAP_TIMER_SRC_SYS_CLK);
	omap_dm_timer_enable(timer);

  	// Read the control register
	ctrl = __omap_dm_timer_read(timer, OMAP_TIMER_CTRL_REG, timer->posted);

  	// Disable prescaler, turn to input and turn off triggers
	ctrl &= ~(OMAP_TIMER_CTRL_PRE | (0x07 << 2) | OMAP_TIMER_CTRL_GPOCFG | (0x03 << 10));

  	// Autoreload
	ctrl |= OMAP_TIMER_CTRL_AR;
	__omap_dm_timer_write(timer, OMAP_TIMER_LOAD_REG, 0, timer->posted);

  	// Start timer
	ctrl |= OMAP_TIMER_CTRL_ST;

	// Start timer
	__omap_dm_timer_load_start(timer, ctrl, 0, timer->posted);

	// Save the context
	timer->context.tclr = ctrl;
	timer->context.tldr = 0;
	timer->context.tcrr = 0;

	// Configure compare
	omap_dm_timer_set_load(timer, 1, 0);
	omap_dm_timer_set_match(timer, 1, 0);
	omap_dm_timer_set_pwm(timer, 0, 1, OMAP_TIMER_TRIGGER_OVERFLOW_AND_COMPARE);
	omap_dm_timer_start(timer);
}

// COMPARE PIN MANAGEMENT ////////////////////////////////////////////////////////////////////

// The capture is scheduled along core time, and it is the responsibility of QoT core to start
// the process "close" to when it needs to occur. If you schedule a capture to occur at some
// global time in the extreme future, your local clock will drift and the edge wont occu when
// you want it to. The driver simply assumes that you are doing this management in core already.
static void qot_am335x_compare_start(struct qot_am335x_pin *pin)
{
	uint64_t c_cycle, f_cycle, m_cycle;
	uint32_t load, match, value;

	// First, using pdata->tc and pdata->cc, determine if the starting edge (pdata->start) is
	// expires some time between now and the end of this overflow period 0xFFFFFFFF. Then, if
	// true, calculate the exact poiny (cycle) that corresponds to the starting edge.

	// mintime = timecounter_read(&pdata->tc);
	// maxtime = mintime + (((cycle_t) 0xFFFFFFFF) * pdata->tc.cc->mult) >> pdata->tc.cc->shift;
	// pr_info("qot_am335x: MINTIME %llu\n",mintime);
	// pr_info("qot_am335x: MAXTIME %llu\n",maxtime);
	// pr_info("qot_am335x: START   %llu\n",pin->start);
	// if (pin->start < mintime || pin->start > maxtime)
	// {
	// 	pr_info("qot_am335x: PWM deferred");
	// 	return;
	// }
	// pr_info("qot_am335x: PWM triggered");


	// Get the cycle at which the PWM must be triggered
	c_cycle = (uint64_t) pdata->cc.read(&pdata->cc);
	f_cycle = (uint64_t) div_u64((pin->start - pdata->tc.nsec) << pdata->tc.cc->shift, pdata->tc.cc->mult) + pdata->tc.cycle_last;
	m_cycle = (uint64_t) f_cycle & 0xFFFFFFFF;
	pr_info("qot_am335x: LASTC %llu\n",c_cycle);
	pr_info("qot_am335x: CYCLE %llu\n",f_cycle);
	pr_info("qot_am335x:   MOD %llu\n",m_cycle);

	// Configure PWM
	load  = -(pin->high + pin->low);
	match = -(pin->low);
	value = __omap_dm_timer_read(pdata->timer, OMAP_TIMER_COUNTER_REG,
		pdata->timer->posted) - (uint32_t) m_cycle + AM335X_KLUDGE_FACTOR;
	__omap_dm_timer_write(pin->timer, OMAP_TIMER_COUNTER_REG, value, pin->timer->posted);
	__omap_dm_timer_write(pin->timer, OMAP_TIMER_LOAD_REG, load, pin->timer->posted);
	__omap_dm_timer_write(pin->timer, OMAP_TIMER_MATCH_REG, match, pin->timer->posted);
	pin->timer->context.tcrr = value;
	pin->timer->context.tmar = match;
	pin->timer->context.tldr = load;
}

// We might have a capture request scheduled beyond the next interrupt. So, we have to check on
// each timer overflow that
static void qot_am335x_compare_stop(struct qot_am335x_pin *pin)
{
	__omap_dm_timer_write(pin->timer, OMAP_TIMER_LOAD_REG,  0,  pin->timer->posted);
	__omap_dm_timer_write(pin->timer, OMAP_TIMER_MATCH_REG, 0, pin->timer->posted);
	pin->timer->context.tmar = 0;
	pin->timer->context.tldr = 0;
	omap_dm_timer_trigger(pin->timer);
	pin->enable = 0;
}

////////////////////////// INTERRUPT HANDLING ////////////////////////////////////////////////

static void qot_am335x_capture_push(struct work_struct *ws)
{
	struct qot_am335x_work *work = container_of(ws, struct qot_am335x_work, work);
	if (work)
	{
		qot_push_capture(work->pin->name, timecounter_cyc2time(&pdata->tc,work->edge));
		kfree(work);
	}
}

// Interrupt handler for compare timers
static irqreturn_t qot_am335x_interrupt(int irq, void *data)
{
	unsigned long flags;
	unsigned int irq_status;
	struct qot_am335x_pin *pin = data;
	struct qot_am335x_work *work;

	spin_lock_irqsave(&pdata->lock, flags);

	// If the OMAP_TIMER_INT_CAPTURE flag in the IRQ status is set
	irq_status = omap_dm_timer_read_status(pin->timer);

	// If this was a capture event
	if (irq_status & OMAP_TIMER_INT_CAPTURE)
	{
		// If we (somehow) get a capture event from a COMPARE or CORE pin
   		if (pin && pin->type == AM335X_TYPE_CAPTURE)
   		{
			// Allocate memory for the pin
			work = kzalloc(sizeof(struct qot_am335x_work), GFP_KERNEL);
			if (work)
			{
				INIT_WORK(&work->work, qot_am335x_capture_push);
				work->pin = pin;
				work->edge = __omap_dm_timer_read(pin->timer,
					OMAP_TIMER_CAPTURE_REG, pin->timer->posted);
				queue_work(pdata->wq, &work->work);
			}
   		}

		// Clear interrupt
		__omap_dm_timer_write_status(pin->timer, OMAP_TIMER_INT_CAPTURE);
	}

	// if this was a match event
	if (irq_status & OMAP_TIMER_INT_MATCH)
	{
		// Don't bother checking  if this is not a compare timer
   		if (pin && pin->type == AM335X_TYPE_COMPARE)
   		{
			// If we have reached the final cycle, stop
			if (pin->repeat == 1)
				qot_am335x_compare_stop(pin);

   			// Otherwise decrement the pin count (0 = INFINITE)
			if (pin->repeat > 1)
				pin->repeat--;
   		}

		// TODO: deal with matches
		__omap_dm_timer_write_status(pin->timer, OMAP_TIMER_INT_MATCH);
	}

	// If this was an overflow event
	if (irq_status & OMAP_TIMER_INT_OVERFLOW)
	{
		// Check if we need to setup a compare for this overflow cycle
		if (pin && pin->type == AM335X_TYPE_COMPARE)
			qot_am335x_compare_start(pin);

		// TODO: deal with overflows
		__omap_dm_timer_write_status(pin->timer, OMAP_TIMER_INT_OVERFLOW);
	}

	spin_unlock_irqrestore(&pdata->lock, flags);

	return IRQ_HANDLED;
}

// EXPORTED SYMBOLS ///////////////////////////////////////////////////////////

// Read the CORE timer hardware count, protecting time registers
static cycle_t qot_am335x_core_timer_read(const struct cyclecounter *cc)
{
	cycle_t val;
	struct qot_am335x_data *pdata = container_of(cc, struct qot_am335x_data, cc);
	val =  __omap_dm_timer_read_counter(pdata->timer, pdata->timer->posted);
	return val;
}

// Periodic check of core time to keep track of overflows
static void qot_am335x_overflow_check(struct work_struct *work)
{
	struct qot_am335x_data *pdata = container_of(work, struct qot_am335x_data, overflow_work.work);
	timecounter_read(&pdata->tc);
	schedule_delayed_work(&pdata->overflow_work, AM335X_OVERFLOW_PERIOD);
}

// Enable the IRQ on a given timer
static void qot_am335x_enable_irq(struct omap_dm_timer *timer, unsigned int interrupt_mask)
{
	__omap_dm_timer_int_enable(timer, interrupt_mask);
	timer->context.tier = interrupt_mask;
	timer->context.twer = interrupt_mask;
}

// Parse the device tree to setup the timers
static struct qot_am335x_data *qot_am335x_of_parse(struct platform_device *pdev)
{
	struct qot_am335x_pin *pin;
	struct qot_am335x_data *pdata;
	struct device_node *np = pdev->dev.of_node;
	struct device_node *timer_node;
	struct of_phandle_args timer_args;
	struct omap_dm_timer *timer;
	const char *tmp = NULL;
	const __be32 *phandle;
	char name[128];
	int i, timer_irqcfg, timer_source;

  	// Allocate memory for the platform data
	pdata = devm_kzalloc(&pdev->dev, sizeof(struct qot_am335x_data), GFP_KERNEL);
	if (!pdata)
		return NULL;

	// Create a workquw
	pr_info("qot_am335x: Creating workqueue...");
	pdata->wq = create_workqueue("events");

	pr_info("qot_am335x: Setting up core timer...");
	phandle = of_get_property(np, "core", NULL);
	if (!phandle)
	{
		pr_err("qot_am335x: could not find phandle for core");
		return NULL;
	}
	if (of_parse_phandle_with_fixed_args(np, "core", 1, 0, &timer_args) < 0)
	{
		pr_err("qot_am335x: could not parse core timer arguments\n");
		return NULL;
	}
	timer_node = of_find_node_by_phandle(be32_to_cpup(phandle));
	if (!timer_node)
	{
		pr_err("qot_am335x: could not find the timer node\n");
		return NULL;
	}
	of_property_read_string_index(timer_node, "ti,hwmods", 0, &tmp);
	if (!tmp)
	{
		pr_err("qot_am335x: ti,hwmods property missing?\n");
		return NULL;
	}
	timer = omap_dm_timer_request_by_node(timer_node);
	if (!timer)
	{
		pr_err("qot_am335x: request_by_node failed\n");
		return NULL;
	}

	// Decide which oscilaltor to clock from
	switch (timer_args.args[0])
	{
	default:
		pr_err("the value %d is not a valid source, so using SYS_CLK\n",timer_args.args[0]);
	case 0: timer_source = OMAP_TIMER_SRC_SYS_CLK; 	break;
	case 1: timer_source = OMAP_TIMER_SRC_32_KHZ; 	break;
	case 2: timer_source = OMAP_TIMER_SRC_EXT_CLK; 	break;
	}

	// Intialize a spin lock to protect IRQs from interrupting critical sections
	spin_lock_init(&pdata->lock);

	// Setup the clock source, enable IRQ and put the node back into the device tree
	switch (timer_source)
	{
	case OMAP_TIMER_SRC_SYS_CLK: pr_info("qot_am335x: Core timer initialized and set to SYS\n"); break;
	case OMAP_TIMER_SRC_32_KHZ:  pr_info("qot_am335x: Core timer initialized and set to RTC\n"); break;
	case OMAP_TIMER_SRC_EXT_CLK: pr_info("qot_am335x: Core timer initialized and set to EXT\n"); break;
	}
	omap_dm_timer_setup_core(timer);
	omap_dm_timer_set_source(timer, timer_source);
	of_node_put(timer_node);

	// Copy over the timer reference and setup periodic overflow checking
	pdata->timer = timer;
	pdata->cc.read = qot_am335x_core_timer_read;
	pdata->cc.mask = CLOCKSOURCE_MASK(32);
	pdata->cc.mult = 174762667;
	pdata->cc.shift = 22;
	timecounter_init(&pdata->tc, &pdata->cc, 0);
	INIT_DELAYED_WORK(&pdata->overflow_work, qot_am335x_overflow_check);
	schedule_delayed_work(&pdata->overflow_work, AM335X_OVERFLOW_PERIOD);

	// Setup the red-black tree root
	pdata->pin_root = RB_ROOT;

	pr_info("qot_am335x: Setting up pins...");

	// Setup each timer pin in the device tree (for interrupt processing and generation)
	for (i = 4; i <= 7; i++)
	{
		sprintf(name,"timer%d",i);
		phandle = of_get_property(np, name, NULL);
		if (!phandle)
		{
			pr_err("qot_am335x: cannot find phandle for timer%d", i);
			continue;
		}
		if (of_parse_phandle_with_fixed_args(np, name, 1, 0, &timer_args) < 0)
		{
			pr_err("qot_am335x: could not parse core timer%d arguments",i);
			continue;
		}
		timer_node = of_find_node_by_phandle(be32_to_cpup(phandle));

		tmp = NULL;
		of_property_read_string_index(timer_node, "ti,hwmods", 0, &tmp);
		if (!tmp)
		{
			pr_err("qot_am335x: ti,hwmods property missing?\n");
			continue;
		}
		timer = omap_dm_timer_request_by_node(timer_node);
		if (!timer)
		{
			pr_err("qot_am335x: request_by_node failed\n");
			continue;
		}

		// Allocate memory for the pin
		pin = kzalloc(sizeof(struct qot_am335x_pin), GFP_KERNEL);
		if (!pin)
			continue;

		// Set pin type and timer
		pin->timer = timer;
		switch(i)
		{
		case 4: strcpy(pin->name,"timer4"); break;
		case 5: strcpy(pin->name,"timer5"); break;
		case 6: strcpy(pin->name,"timer6"); break;
		case 7: strcpy(pin->name,"timer7"); break;
		default:
			pr_err("qot_am335x: %d is not a recognised timer id\n",i);
		}

		// Request an IRQ for this timer, passing the pin into the ISR
		if (request_irq(timer->irq, qot_am335x_interrupt, IRQF_TIMER, MODULE_NAME, pin))
		{
			pr_err("qot_am335x: cannot register IRQ %d\n", timer->irq);
			kfree(pin);
			continue;
		}

		// Set the pin type
		pin->type = AM335X_TYPE_CAPTURE;

		// Set the interrupt typw
		timer_irqcfg = OMAP_TIMER_CTRL_TCM_LOWTOHIGH;
		switch (timer_args.args[0])
		{
		case 0: timer_irqcfg = OMAP_TIMER_CTRL_TCM_LOWTOHIGH; pr_info("qot_am335x: Timer %d set to capture on rising edge\n",i); break;
		case 1: timer_irqcfg = OMAP_TIMER_CTRL_TCM_HIGHTOLOW; pr_info("qot_am335x: Timer %d set to capture on falling edge\n",i); break;
		case 2: timer_irqcfg = OMAP_TIMER_CTRL_TCM_BOTHEDGES; pr_info("qot_am335x: Timer %d set to capture on any edge\n",i); break;
		case 3: pin->type = AM335X_TYPE_COMPARE; pr_info("qot_am335x: Timer %d set to compare (output mode)\n",i);  break;
		}

		// Setup the timer based on the type
		switch(pin->type)
		{
		case AM335X_TYPE_COMPARE:
			omap_dm_timer_setup_compare(timer);
			omap_dm_timer_set_source(timer, timer_source);
			break;
		case AM335X_TYPE_CAPTURE:
			omap_dm_timer_setup_capture(timer, timer_irqcfg);
			omap_dm_timer_set_source(timer, timer_source);
			break;
		}

		// Align the hardware count of this timer as best as possible to the core timer. Since they
		// are clocked by the same oscillator, there should be no relative drift.
		__omap_dm_timer_write(timer, OMAP_TIMER_COUNTER_REG, __omap_dm_timer_read(pdata->timer,
			OMAP_TIMER_COUNTER_REG, pdata->timer->posted) + AM335X_KLUDGE_FACTOR, timer->posted);

		// Add the pin to the list
		qot_am335x_pin_insert(&pdata->pin_root, pin);

		// Put the device tree node back
		of_node_put(timer_node);
	}

	// Return th platform data
	return pdata;
}

// CALLBACK STRUCT ///////////////////////////////////////////////////////////

// Setup the compare pin
int qot_am335x_setup_compare(const char *name, uint8_t enable,
	uint64_t start, uint32_t high, uint32_t low, uint64_t repeat)
{
	// Search for the timer with the supplied name
	struct qot_am335x_pin *pin = qot_am335x_pin_search(&pdata->pin_root, name);
	if (!pin)
		return -1;

	// Don't bother checking  if this is not a compare timer
	if (pin->type != AM335X_TYPE_COMPARE)
		return -2;

	// Stop the current action immediately
	qot_am335x_compare_stop(pin);

	// Now change the parameters
	pin->enable = enable;
	pin->start = start;
	pin->high = high;
	pin->low = low;
	pin->repeat = repeat;

	// Check to see if this needs to be scheduled
	qot_am335x_compare_start(pin);

	// Success!
	return 0;
}

// Read the clock
uint64_t qot_am335x_read(void)
{
	return timecounter_read(&pdata->tc);
}

static struct qot_driver qot_driver_ops = {
    .compare  = qot_am335x_setup_compare,
    .read     = qot_am335x_read,
};

// MODULE INITIALIZATION //////////////////////////////////////////////////////

static const struct of_device_id qot_am335x_dt_ids[] = {
	{ .compatible = "qot_am335x", },
  	{ }
};

MODULE_DEVICE_TABLE(of, qot_am335x_dt_ids);

// INsert the kernel module
static int qot_am335x_probe(struct platform_device *pdev)
{
	struct qot_am335x_pin *pin;
	struct rb_node *node;
	const struct of_device_id *match =
		 of_match_device(qot_am335x_dt_ids, &pdev->dev);
	if (match)
	{
		// Extract the platform data from the device tree
		pdata = pdev->dev.platform_data = qot_am335x_of_parse(pdev);

		// Return error if no platform data was allocated
		if (!pdata)
			return -ENODEV;

		// Only enable IRQs after EVERYTING has initialized
		for (node = rb_first(&pdata->pin_root); node; node = rb_next(node))
		{
			// Get the binding container of this red-black tree node
			pin = container_of(node, struct qot_am335x_pin, node);
			switch(pin->type)
			{
			case AM335X_TYPE_COMPARE:
				qot_am335x_enable_irq(pin->timer, OMAP_TIMER_INT_MATCH | OMAP_TIMER_INT_OVERFLOW);
				break;
			case AM335X_TYPE_CAPTURE:
				qot_am335x_enable_irq(pin->timer, OMAP_TIMER_INT_CAPTURE);
				break;
			}
		}

		// Register with the QoT core
		qot_register(&qot_driver_ops);
	}
	else
	{
		pr_err("of_match_device failed\n");
		return -ENODEV;
	}
	return 0;
}

// Remove the kernel module
static int qot_am335x_remove(struct platform_device *pdev)
{
	struct qot_am335x_pin *pin;
	struct rb_node *node;

	// Get the platform data
	if (pdata)
	{
		// Unregister with the QoT core, to prevent being interrupted
		qot_unregister();

		// Now clean up all timers
		for (node = rb_first(&pdata->pin_root); node; node = rb_next(node))
		{
			// Get the binding container of this red-black tree node
			pin = container_of(node, struct qot_am335x_pin, node);

			// Cleanup a pin timer
			omap_dm_timer_set_source(pin->timer, OMAP_TIMER_SRC_SYS_CLK);
			omap_dm_timer_set_int_disable(pin->timer,
				OMAP_TIMER_INT_MATCH | OMAP_TIMER_INT_CAPTURE | OMAP_TIMER_INT_OVERFLOW);
			free_irq(pin->timer->irq, pin);
			omap_dm_timer_stop(pin->timer);
			omap_dm_timer_free(pin->timer);
			pin->timer = NULL;

			// Free the pin memory
			kfree(pin);

			// Delete the rb-node
			rb_erase(node, &pdata->pin_root);
		}

	  	// Stop the overflow timer
		cancel_delayed_work_sync(&pdata->overflow_work);

		// Free the core timer
		omap_dm_timer_set_source(pdata->timer, OMAP_TIMER_SRC_SYS_CLK);
		omap_dm_timer_stop(pdata->timer);
		omap_dm_timer_free(pdata->timer);
		pdata->timer = NULL;

		// Free the platform data
		devm_kfree(&pdev->dev, pdev->dev.platform_data);

		// Set the pointers to NULL
		pdata = pdev->dev.platform_data = NULL;
	}

	// Cull all driver data
	platform_set_drvdata(pdev, NULL);

	// Success
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
MODULE_DESCRIPTION("QoT Driver for AM335x in Linux 4.1.x");
MODULE_VERSION("0.5.0");

*/