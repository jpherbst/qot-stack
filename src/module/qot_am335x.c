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

IMPORTANT : To use this driver you will need to add the following snippet to your
            default device tree, or to an overlay (if supported). This will cause
            the module to be loaded on boot, and make use of the QoT stack.

qot_am335x {
	compatible = "qot_am335x";
	status = "okay";
	core   = <&timer3 0>; (0=CLK-SYS-24MHz, 1=CLK-SYS-32kHz, 2=TCLKLIN)
	timer4 = <&timer4 0>; (0=CAP-RISING, 1=CAP-FALLING, 2=CAP-BOTH, 3=COMPARE)
	timer5 = <&timer5 0>; (0=CAP-RISING, 1=CAP-FALLING, 2=CAP-BOTH, 3=COMPARE)
	timer6 = <&timer6 0>; (0=CAP-RISING, 1=CAP-FALLING, 2=CAP-BOTH, 3=COMPARE)
	timer7 = <&timer7 0>; (0=CAP-RISING, 1=CAP-FALLING, 2=CAP-BOTH, 3=COMPARE)
	pinctrl-names = "default";
	pinctrl-0 = <&timer_pins>;
};

*/

// Sufficient to develop a device-tree based platform driver
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/workqueue.h>
#include <linux/timecounter.h>
#include <linux/rbtree.h>

// DMTimer Code specific to the AM335x
#include "../../thirdparty/Kronux/arch/arm/plat-omap/include/plat/dmtimer.h"

// QoT kernel API
#include "qot.h"
#include "qot_core.h"

// FORWARD DELCLARATIONS /////////////////////////////////////////////////////////

// Module information
#define MODULE_NAME 			"qot_am335x"
#define AM335X_TYPE_CAPTURE 	(0)
#define AM335X_TYPE_COMPARE 	(1)
#define AM335X_PIN_MAXLEN 		(128)
#define AM335X_OVERFLOW_PERIOD 	(8*HZ)
#define AM335X_KLUDGE_FACTOR 	13

// Basic information about a platform timer
struct qot_am335x_timer {
	char name[AM335X_PIN_MAXLEN];		// Human-readable name, like "core", or "timer4"
	struct omap_dm_timer *dmtimer;		// Pointer to the OMAP dual-mode timer
	uint8_t type;						// Type of pin (0 = capture, 1 = compare, 2 = sys)
	uint8_t enable;				 	    // COMPARE: [0] = disable, [1] = enabled
	uint64_t next;						// COMPARE: Time of next event (clock ticks)
	uint32_t cycles_high;				// COMPARE: High cycle time (clock ticks)
	uint32_t cycles_low;				// COMPARE: Low cycle time (clock ticks)
	uint32_t limit;						// COMPARE: Number of repetition (0 = infinite)
    struct rb_node node;				// Red-black tree is used to store timers on name
};

// Platform data -- specific to the AM335x
struct qot_am335x_data {
	struct omap_dm_timer *core;			// CORE: timer
	struct timecounter tc;				// CORE: time counter
	struct cyclecounter cc;				// CORE: cycle counter
	struct delayed_work overflow_work;	// CORE: scheduled work
};

// Allow compilation on x86 type systems
extern struct device_node *of_find_node_by_phandle(phandle handle);

// A hash table designed to quickly find a timeline based on its UUID
static struct rb_root timer_root = RB_ROOT;

// Lock to protect asynchronous IRQ events from colliding
static DEFINE_SPINLOCK(qot_am335x_lock);

//////////////////////////////// DATA STRUCTURE FUNCTIONS ////////////////////////////////////

// Search our data structure for a given timer
static struct qot_am335x_timer *qot_am335x_timer_search(struct rb_root *root, const char *name)
{
	int result;
	struct qot_am335x_timer *timeline;
	struct rb_node *node = root->rb_node;
	while (node)
	{
		timeline = container_of(node, struct qot_am335x_timer, node);
		result = strcmp(name, timeline->name);
		if (result < 0)
			node = node->rb_left;
		else if (result > 0)
			node = node->rb_right;
		else
			return timeline;
	}
	return NULL;
}

// REGISTER SETUP FOR TIMERS ////////////////////////////////////////////////////////

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

// Event scheduler that implements PWM
static void qot_am335x_compare_scheduler(struct qot_am335x_timer *core, struct qot_am335x_timer *pin, int type)
{
	uint32_t h, l;
	uint32_t load, match, value;

	// Don't bother checking compare timer that's disabled
	if (pin->type != AM335X_TYPE_COMPARE)
		return;

	// If compare mode is disabled, zero the load and match registers
	if (pin->enable == 0)
	{
		__omap_dm_timer_write(pin->timer, OMAP_TIMER_LOAD_REG,  0, pin->timer->posted);
		__omap_dm_timer_write(pin->timer, OMAP_TIMER_MATCH_REG, 0, pin->timer->posted);
		pin->timer->context.tmar = 0;
		pin->timer->context.tldr = 0;
		return;
	}

	// If a match has just been fired
	switch (type)
	{

	// Both the initialization and the capture overflow cases do the same thing.
	// The objective is to check whether we need to schedule a load / match in
	// the current 32bit timer period. We use the OMAP_TIMER_INT_CAPTURE flag in
	// place of a custom INIT value :) 
	case OMAP_TIMER_INT_CAPTURE: 

		if (timer->)

		// We need to increment this to get expected behaviour
		if (pin->limit > 0) 
			pin->limit++;

		// NOTE THAT THERE IS NO BREAK HERE BY DESIGN...

	case OMAP_TIMER_INT_OVERFLOW:

		// Find the high and low bits of the timer
		h = (pin.next >> 32);
		l = (pin.next);

		// If the event must occur in the current overflow
		if (h == pin->overflow)
		{	
			// Set load and match
			load  = -(pin->cycles_high + pin->cycles_low);
			match = -(pin->cycles_low);
			value = __omap_dm_timer_read(core->timer->, OMAP_TIMER_COUNTER_REG, core->timer->posted) - l + AM335X_KLUDGE_FACTOR;
			__omap_dm_timer_write(pin->timer, OMAP_TIMER_COUNTER_REG, value, pin->timer->posted);
			__omap_dm_timer_write(pin->timer, OMAP_TIMER_LOAD_REG, load, pin->timer->posted);
			__omap_dm_timer_write(pin->timer, OMAP_TIMER_MATCH_REG, match, pin->timer->posted);
			pdata->compare_timer->context.tcrr = value;
			pdata->compare_timer->context.tmar = match;
			pdata->compare_timer->context.tldr = load;		
		}

		break;

	// If a match has just been fired, it means that we have transitioned from a high to
	// a low PWM cycle. We need to check how many cycles were requested by the user. If 
	// we have reached that number, we need to turn disable the timer to prevent
	case OMAP_TIMER_INT_MATCH:

		// A pdata->comp.limit = zero indicates an infinite loop. So, we need to 
		// disable the PWM. We do this by setting enable = 0.
		if (pin->limit > 1)
			pin->limit--;
		else if (pin->limit == 1)
		{
			// Reset load and match to stop toggles
			load  = 0;
			match = 0;
			__omap_dm_timer_write(pin->timer, OMAP_TIMER_LOAD_REG,  load,  pin->timer->posted);
			__omap_dm_timer_write(pin->timer, OMAP_TIMER_MATCH_REG, match, pin->timer->posted);
			pin->timer->context.tmar = match;
			pin->timer->context.tldr = load;
			omap_dm_timer_trigger(pin->timer);
			pin->enable = 0;
		}

		break;
	}
}

// Setup the compare pin - start/high/low are in core time units. The first task is to map them
// to the cycle counts using the mult/shift paramaters in the timecounter.
static int qot_am335x_setup_compare(const char *name, uint8_t enable, 
	int64_t start, uint32_t high, uint32_t low, uint32_t repeat)
{
	// Search for the timer with the supplied name
	struct qot_am335x_timer *timer = qot_am335x_timer_search(timer_root, name);
	if (!timer)
		return -1;



	return 0;
}

////////////////////////// INTERRUPT HANDLING ////////////////////////////////////////////////

// Interrupt handler for compare timers
static irqreturn_t qot_am335x_interrupt(int irq, void *data)
{
	unsigned long flags;
	unsigned int irq_status;
	struct omap_dm_timer *timer = data;
	struct qot_am335x_timer *pin = container_of(data, struct qot_am335x_timer, timer);
	// Extract the platform data from the pin
	struct qot_am335x_data *pdata
	struct list_head *ptr = &pin->list;
	while (ptr->prev) ptr = ptr->prev;
	pdata = container_of(ptr, struct qot_am335x_data, pin_head.next);

	spin_lock_irqsave(&qot_am335x_lock, flags);	

	// If the OMAP_TIMER_INT_CAPTURE flag in the IRQ status is set
	irq_status = omap_dm_timer_read_status(timer);

	// If this was a capture event
	if (irq_status & OMAP_TIMER_INT_CAPTURE)
	{
		// Clear interrupt
		__omap_dm_timer_write_status(timer, OMAP_TIMER_INT_CAPTURE);
	}

	// if this was a match event
	if (irq_status & OMAP_TIMER_INT_MATCH)
	{
		// TODO: deal with matches
		__omap_dm_timer_write_status(timer, OMAP_TIMER_INT_MATCH);
	}

	// If this was an overflow event
	if (irq_status & OMAP_TIMER_INT_OVERFLOW)
	{
		// TODO: deal with overflows
		__omap_dm_timer_write_status(timer, OMAP_TIMER_INT_OVERFLOW);
	}

	spin_unlock_irqrestore(&qot_am335x_lock, flags);

	return IRQ_HANDLED;
}

// Set the source of a timer to a given clock
static void omap_dm_timer_use_clk(struct omap_dm_timer *timer, int clk)
{
	omap_dm_timer_set_source(timer, clk);
	switch (clk)
	{
	case OMAP_TIMER_SRC_SYS_CLK: pr_info("qot_am335x: Timer %d SYS\n", timer->id); break;
	case OMAP_TIMER_SRC_32_KHZ:  pr_info("qot_am335x: Timer %d RTC\n", timer->id); break;
	case OMAP_TIMER_SRC_EXT_CLK: pr_info("qot_am335x: Timer %d EXT\n", timer->id); break;
	}
}

// Initialise a timer
static int qot_am335x_timer_init(struct omap_dm_timer *timer, struct device_node *node)
{
	// Used to check for successful device tree read
	const char *tmp = NULL;
	of_property_read_string_index(node, "ti,hwmods", 0, &tmp);
	if (!tmp)
	{
		pr_err("qot_am335x: ti,hwmods property missing?\n");
		return -ENODEV;
	}
	timer = omap_dm_timer_request_by_node(node);
	if (!timer)
	{
		pr_err("qot_am335x: request_by_node failed\n");
		return -ENODEV;
	}
	if (request_irq(timer->irq, qot_am335x_interrupt, IRQF_TIMER, MODULE_NAME, timer))
	{
		pr_err("qot_am335x: cannot register IRQ %d\n", timer->irq);
		return -EIO;
	}
	return 0;
}

// Cleanup a timer
static void qot_am335x_timer_cleanup(struct omap_dm_timer *timer, struct qot_am335x_data *pdata)
{
    omap_dm_timer_set_source(timer, OMAP_TIMER_SRC_SYS_CLK);
    omap_dm_timer_set_int_disable(timer, 
    	OMAP_TIMER_INT_MATCH | OMAP_TIMER_INT_CAPTURE | OMAP_TIMER_INT_OVERFLOW);
    free_irq(timer->irq, pdata);
    omap_dm_timer_stop(timer);
    omap_dm_timer_free(timer);
    timer = NULL;
}

// EXPORTED SYMBOLS ///////////////////////////////////////////////////////////

// Read the CORE timer hardware count
static cycle_t qot_am335x_core_timer_read(const struct cyclecounter *cc)
{
	struct qot_am335x_data *pdata = container_of(cc, struct qot_am335x_data, cc);
	return (cycle_t) __omap_dm_timer_read_counter(pdata->core, pdata->core->posted);
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
	struct qot_am335x_timer *pin;
	struct qot_am335x_data *pdata;
	struct device_node *np = pdev->dev.of_node;
	struct device_node *timer_node;
	struct of_phandle_args timer_args;
	struct omap_dm_timer *timer;
	const __be32 *phandle;
	char name[128];
	int i, timer_irqcfg, timer_source, type;

  	// Allocate memory for the platform data
	pdata = devm_kzalloc(&pdev->dev, sizeof(struct qot_am335x_data), GFP_KERNEL);
	if (!pdata)
		return NULL;

	// Setup the list for timer pins
	INIT_LIST_HEAD(&pdata->pin_head);

	// Setup the core timer
	phandle = of_get_property(np, "core", NULL);
	if (!phandle)
	{
		pr_err("qot_am335x: cannot find phandle for core");
		return NULL;
	} 
	if (of_parse_phandle_with_fixed_args(np, "core", 1, 0, &timer_args) < 0)
	{
		pr_err("qot_am335x: could not parse core timer arguments\n");
		return NULL;
	}
	timer_node = of_find_node_by_phandle(be32_to_cpup(phandle));
	if (qot_am335x_timer_init(timer, timer_node))
	{
		pr_err("qot_am335x: could create the core timer\n");
		return NULL;
	}
	switch (timer_args.args[0])
	{
	default:
		pr_err("the value %d is not a valid source, so using SYS_CLK\n",timer_args.args[0]);
	case 0: timer_source = OMAP_TIMER_SRC_SYS_CLK; 	break;
	case 1: timer_source = OMAP_TIMER_SRC_32_KHZ; 	break;
	case 2: timer_source = OMAP_TIMER_SRC_EXT_CLK; 	break;
	}

	// Setup the clock source, enable IRQ and put the node back into the device tree
	omap_dm_timer_use_clk(timer, timer_source);
	qot_am335x_enable_irq(timer, OMAP_TIMER_INT_OVERFLOW);
	of_node_put(timer_node);

	// Add the timer to the pin list and store is as a core reference
	list_add_tail(&pdata->pin_head, &pin->list);
	pdata->core = timer;

	// Set up a timecounter to keep track of time and take care of overflows
	pdata->cc.read = qot_am335x_core_timer_read;
	pdata->cc.mask = CLOCKSOURCE_MASK(32);
	pdata->cc.mult = 0x80000000;
	pdata->cc.shift = 29;
	timecounter_init(&pdata->tc, &pdata->cc, ktime_to_ns(ktime_get_real()));
	INIT_DELAYED_WORK(&pdata->overflow_work, qot_am335x_overflow_check);
	schedule_delayed_work(&pdata->overflow_work, AM335X_OVERFLOW_PERIOD);

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
		if (qot_am335x_timer_init(timer, timer_node))
		{
			pr_err("qot_am335x: could create timer%d",i);
			continue;
		}
		type = AM335X_TYPE_CAPTURE;
		switch (timer_args.args[0])
		{
		default:
			pr_err("qot_am335x: %d is not a valid timer%d config",timer_args.args[0],i);
		case 0: type = AM335X_TYPE_COMPARE; break;
		case 1: timer_irqcfg = OMAP_TIMER_CTRL_TCM_LOWTOHIGH; break;
		case 2: timer_irqcfg = OMAP_TIMER_CTRL_TCM_HIGHTOLOW; break;
		case 3: timer_irqcfg = OMAP_TIMER_CTRL_TCM_BOTHEDGES; break;
		}

		// Setup the timer based on the type
		switch(type)
		{
		case AM335X_TYPE_COMPARE:
			omap_dm_timer_setup_compare(timer);
			omap_dm_timer_use_clk(timer, timer_source);
			qot_am335x_enable_irq(timer, OMAP_TIMER_INT_MATCH | OMAP_TIMER_INT_OVERFLOW);
			break;
		case AM335X_TYPE_CAPTURE:
			omap_dm_timer_setup_capture(timer, timer_irqcfg);
			omap_dm_timer_use_clk(timer, timer_source);
			qot_am335x_enable_irq(timer, OMAP_TIMER_INT_CAPTURE | OMAP_TIMER_INT_OVERFLOW);
			break;
		}

		// Align the hardware count of this timer as best as possible to the core timer. Since they
		// are clocked by the same oscillator, there should be no relative drift.
		__omap_dm_timer_write(timer, OMAP_TIMER_COUNTER_REG, __omap_dm_timer_read(pdata->core, 
			OMAP_TIMER_COUNTER_REG, pdata->core->posted) + AM335X_KLUDGE_FACTOR, timer->posted);
		
		// Allocate memory for the pin
		pin = kzalloc(sizeof(struct qot_am335x_timer), GFP_KERNEL);
		if (pin == NULL)
			return NULL;
		pin->type = type;
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

		// Add the pin to the list
		list_add_tail(&pdata->pin_head, &pin->list);

		// Put the device tree node back
		of_node_put(timer_node);
	}

	// Return th platform data
	return pdata;
}

// MODULE INITIALIZATION //////////////////////////////////////////////////////

static struct qot_driver qot_driver_ops = {
    .owner    = THIS_MODULE,
    .compare  = qot_am335x_compare_event,
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
	unsigned long flags;	

	const struct of_device_id *match = 
		 of_match_device(qot_am335x_dt_ids, &pdev->dev);
	if (match)
	{
		// Don't allow interrupts during timer creation
		spin_lock_irqsave(&qot_am335x_lock, flags);

		// Extract the platform data from the device tree
		pdev->dev.platform_data = qot_am335x_of_parse(pdev);

		// Restore IRQs
		spin_unlock_irqrestore(&qot_am335x_lock, flags);

		// Return error if no platform data was allocated
		if (!pdev->dev.platform_data)
			return -ENODEV;
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
	unsigned long flags;
	struct qot_am335x_data *pdata;
	struct qot_am335x_timer *pin;
	struct list_head *pos, *q;
	
	// Don't allow interrupts during timer destruction
	spin_lock_irqsave(&qot_am335x_lock, flags);

	// Get the platform data
	pdata = pdev->dev.platform_data;
	if (pdata)
	{
		// Clean up the individual pins
		list_for_each_safe(pos, q, &pdata->pin_head)
		{
			// Extract the pin from the list
			 pin = list_entry(pos, struct qot_am335x_timer, list);
			 
			 // Remove from the list
			 list_del(pos);

			 // Free the timer
			 qot_am335x_timer_cleanup(pin->timer, pdata);

			 // Free the pin memory
			 kfree(pin);
		}

		// Free the platform data
		devm_kfree(&pdev->dev, pdev->dev.platform_data);
		pdev->dev.platform_data = NULL;
	}
	platform_set_drvdata(pdev, NULL);

	// Restore IRQs
	spin_unlock_irqrestore(&qot_am335x_lock, flags);

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
MODULE_DESCRIPTION("QoT Driver for BBB-GPSDO in Linux 4.1.6");
MODULE_VERSION("0.4.0");
