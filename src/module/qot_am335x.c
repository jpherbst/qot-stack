/*
 * @file qot_am335x.c
 * @brief QoT Driver for BBB-GPSDO in Linux 4.1.6
 * @author Andrew Symington
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

// Clock related kernel stuff
#include <linux/workqueue.h>
#include <linux/clk.h>
#include <linux/clocksource.h>

// Module information
#define MODULE_NAME "qot_am335x"

// This is a fixed offset added to the dual timer sync to account for the fact that
// the actual instructions used to read/write to the counter take a few cycles to 
// execute on the microcontroller. It was derivide empirically.
#define KLUDGE_FACTOR 13

// DMTimer Code specific to the AM335x
#include "../../thirdparty/Kronux/arch/arm/plat-omap/include/plat/dmtimer.h"

// QoT kernel API
#include "qot.h"

// FORWARD DELCLARATIONS /////////////////////////////////////////////////////////

// Platform data -- specific to the AM335x
struct qot_am335x_data {
	struct clocksource clksrc;
	struct omap_dm_timer *core;
	struct omap_dm_timer *timer4;
	struct omap_dm_timer *timer5;
	struct omap_dm_timer *timer6;
	struct omap_dm_timer *timer7;
};

// Stores a capture event for future processing
struct qot_am335x_capture {
	struct qot_capture_event event;
	struct work_struct capture_work;
};

// Stateless queue for capture events	
static struct workqueue_struct *queue_capture;

// Allow compilation on (x86 type systems)
extern struct device_node *of_find_node_by_phandle(phandle handle);

// Lock to protect asynchronous IRQ events from colliding
static DEFINE_SPINLOCK(qot_am335x_lock);

// DEFERRED WORK ////////////////////////////////////////////////////////////////////

static void qot_am335x_captue_work(struct work_struct *work)
{
	// Extract the capture information
	struct qot_am335x_capture *capture
		= container_of(work, struct qot_am335x_capture, capture_work);
	
	// Pass the capture up to the HAL
	qot_capture(&capture->event);
	
	// Free the memory used by this work
	kfree(capture);
}

// REGISTER SETUP FOR TIMERS ////////////////////////////////////////////////////////

static void omap_dm_timer_setup_capture(struct omap_dm_timer *timer, int timer_irqcfg)
{
	u32 ctrl;

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

static void omap_dm_timer_setup_compare(struct omap_dm_timer *timer)
{
	u32 ctrl;

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


// This function

static void qot_am_event_scheduler(struct roseline_clock_data *pdata, int type)
{
	uint32_t h, l;
	u32 load, match, value;

	// Optimization: don't bother checking compare timer that's disabled
	if (!pdata->comp.enable)
	{
		// Stop any events from occuring
		__omap_dm_timer_write(pdata->compare_timer, OMAP_TIMER_LOAD_REG,  0, pdata->compare_timer->posted);
		__omap_dm_timer_write(pdata->compare_timer, OMAP_TIMER_MATCH_REG, 0, pdata->compare_timer->posted);
		pdata->compare_timer->context.tmar = 0;
		pdata->compare_timer->context.tldr = 0;

		// Success
		return;
	}

	// If a match has just been fired
	switch (type)
	{
	// Both the initialization and the capture overflow cases do the same thing.
	// The objective is to check whether we need to schedule a load / match in
	// the current 32bit timer period 
	case FROM_INIT:

		// We need to increment this to get expected behaviour
		if (pdata->comp.limit > 0) 
			pdata->comp.limit++;

	case FROM_CAPTURE_OVERFLOW:

		// Find the high and low bits of the timer
		h = (pdata->comp.next >> 32);
		l = (pdata->comp.next);

		// If the event must occur in the current overflow
		if (h == pdata->capt.overflow)
		{	
			// Set load and match
			load  = -(pdata->comp.cycles_high + pdata->comp.cycles_low);
			match = -(pdata->comp.cycles_low);
			value = __omap_dm_timer_read(pdata->capture_timer, OMAP_TIMER_COUNTER_REG, pdata->capture_timer->posted) - l + KLUDGE_FACTOR;
			__omap_dm_timer_write(pdata->compare_timer, OMAP_TIMER_COUNTER_REG, value, pdata->compare_timer->posted);
			__omap_dm_timer_write(pdata->compare_timer, OMAP_TIMER_LOAD_REG, load, pdata->compare_timer->posted);
			__omap_dm_timer_write(pdata->compare_timer, OMAP_TIMER_MATCH_REG, match, pdata->compare_timer->posted);
			pdata->compare_timer->context.tcrr = value;
			pdata->compare_timer->context.tmar = match;
			pdata->compare_timer->context.tldr = load;		
		}

		break;

	// If a match has just been fired, it means that we have transitioned from a high to
	// a low PWM cycle. We need to check how many cycles were requested by the user. If 
	// we have reached that number, we need to turn disable the timer to prevent
	case FROM_COMPARE_MATCH:

		// A pdata->comp.limit = zero indicates an infinite loop. So, we need to 
		// disable the PWM. We do this by setting enable = 0.
		if (pdata->comp.limit > 1)
			pdata->comp.limit--;
		else if (pdata->comp.limit == 1)
		{
			// Reset load and match to stop toggles
			load  = 0;
			match = 0;
			__omap_dm_timer_write(pdata->compare_timer, OMAP_TIMER_LOAD_REG,  load,  pdata->compare_timer->posted);
			__omap_dm_timer_write(pdata->compare_timer, OMAP_TIMER_MATCH_REG, match, pdata->compare_timer->posted);
			pdata->compare_timer->context.tmar = match;
			pdata->compare_timer->context.tldr = load;
			omap_dm_timer_trigger(pdata->compare_timer);
			pdata->comp.enable = 0;
		}
		
		break;

	}
}

// Interrupt handler for compare timers
static irqreturn_t qot_am335x_interrupt(int irq, void *data)
{
	unsigned long flags;
	unsigned int irq_status;
	struct omap_dm_timer *timer = data;
	struct qot_am335x_capture *capture;

	spin_lock_irqsave(&qot_am335x_lock, flags);

	// If the OMAP_TIMER_INT_CAPTURE flag in the IRQ status is set
	irq_status = omap_dm_timer_read_status(timer);

	// If this was a capture event
	if (irq_status & OMAP_TIMER_INT_CAPTURE)
	{
		// Alloate work memeory
		capture = (struct qot_am335x_capture*) 
			kmalloc(sizeof(struct qot_am335x_capture), GFP_KERNEL)
		
		// Initialize the work
		INIT_WORK(&capture->work, qot_am335x_capture_work);

		// Write the capture data and set the status
		capture->id = timer->id;
		capture->count_at_interrupt = omap_dm_timer_read_counter(timer);
		capture->count_at_capture   = __omap_dm_timer_read(timer, 
			OMAP_TIMER_CAPTURE_REG, timer->posted);
		__omap_dm_timer_write_status(timer, OMAP_TIMER_INT_CAPTURE);

		// Enqueue the work for processing by the top half
		queue_work(queue_capture, &capture->work);
	}

	// If this was an overflow event
	if (irq_status & OMAP_TIMER_INT_OVERFLOW)
	{
		__omap_dm_timer_write_status(timer, OMAP_TIMER_INT_OVERFLOW);
	}

	// if this was a match event
	if (irq_status & OMAP_TIMER_INT_MATCH)
	{
		__omap_dm_timer_write_status(timer, OMAP_TIMER_INT_MATCH);
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
    	OMAP_TIMER_INT_MATCH | OMAP_TIMER_INT_COMPARE| OMAP_TIMER_INT_OVERFLOW);
    free_irq(timer->irq, pdata);
    omap_dm_timer_stop(timer);
    omap_dm_timer_free(timer);
    timer = NULL;
}

// EXPORTED SYMBOLS ///////////////////////////////////////////////////////////

// Read the OMAP cycle counter
static cycle_t qot_am335x_read_cycles(struct clocksource *src)
{
	struct qot_am335x_data *pdata = container_of(src, struct qot_am335x_data, clksrc);
	return (cycle_t) __omap_dm_timer_read_counter(pdata->core, pdata->core->posted);
}

// Initialize a clock source
static void qot_am335x_clocksource_init(struct clocksource *clksrc, struct omap_dm_timer *timer)
{
	// Extract the nominal frequency
	struct clk *gt_fclk;
	uint32_t f0;
	gt_fclk = omap_dm_timer_get_fclk(timer);
	f0 = clk_get_rate(gt_fclk);

	// Set up the clock source
	clksrc->name   = "core";
	clksrc->rating = 299;
	clksrc->read   = qot_am335x_read_cycles;
	clksrc->mask   = CLOCKSOURCE_MASK(32);
	clksrc->flags  = CLOCK_SOURCE_IS_CONTINUOUS;
	if (clocksource_register_hz(clksrc, f0))
		pr_err("qot_am335x: Could not register clocksource %s\n", clksrc->name);
	else
		pr_info("qot_am335x: clocksource: %s at %u Hz\n", clksrc->name, f0);
}

// Cleanup a clocksource
static void qot_am335x_clocksource_cleanup(struct clocksource *clksrc)
{
	clocksource_unregister(clksrc);
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
	struct qot_am335x_data *pdata;
	struct device_node *np = pdev->dev.of_node;
	struct device_node *timer_node;
	struct of_phandle_args timer_args;
	struct omap_dm_timer *timer;
	const __be32 *phandle;
	char name[128];
	int i, timer_irqcfg, timer_source, compare;

  	// Allocate memory for the platform data
	pdata = devm_kzalloc(&pdev->dev, sizeof(struct qot_am335x_data), GFP_KERNEL);
	if (!pdata)
		return NULL;

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
	if (qot_am335x_timer_init(pdata->core, timer_node))
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

	// Setup core timer and clock source
	omap_dm_timer_use_clk(pdata->core, timer_source);
	qot_am335x_clocksource_init(&pdata->clksrc, pdata->core);
	qot_am335x_enable_irq(pdata->core, OMAP_TIMER_INT_OVERFLOW);
	of_node_put(timer_node);

	// Register the clocksource with the QoT HAL
	qot_core_register(&pdata->clksrc);

	// Setup the secondary timers (for interrupt processing and generation)
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
		compare = 0;
		switch (timer_args.args[0])
		{
		default:
			pr_err("qot_am335x: %d is not a valid timer%d config",timer_args.args[0],i);
		case 0: compare = 1; break;
		case 1: timer_irqcfg = OMAP_TIMER_CTRL_TCM_LOWTOHIGH; break;
		case 2: timer_irqcfg = OMAP_TIMER_CTRL_TCM_HIGHTOLOW; break;
		case 3: timer_irqcfg = OMAP_TIMER_CTRL_TCM_BOTHEDGES; break;
		}
		if (compare)
		{
			omap_dm_timer_setup_compare(timer);
			omap_dm_timer_use_clk(timer, timer_source);
			qot_am335x_enable_irq(timer, OMAP_TIMER_INT_MATCH | OMAP_TIMER_INT_OVERFLOW);
			qot_register_compare(timer->id);
		}
		else
		{
			omap_dm_timer_setup_capture(timer, timer_irqcfg);
			omap_dm_timer_use_clk(timer, timer_source);
			qot_am335x_enable_irq(timer, OMAP_TIMER_INT_CAPTURE | OMAP_TIMER_INT_OVERFLOW);
			qot_register_capture(timer->id);
		}
		switch(i)
		{
			case 4: pdata->timer4 = timer; break;
			case 5: pdata->timer5 = timer; break;
			case 6: pdata->timer6 = timer; break;
			case 7: pdata->timer7 = timer; break;
			default:
				pr_err("qot_am335x: %d is not a recognised timer id\n",i);
		}
		of_node_put(timer_node);
	}

	// Return th platform data
	return pdata;
}

int qot_am335x_ext_osc_register(struct qot_properties* osc)
{
	return 0;
}
EXPORT_SYMBOL(qot_am335x_ext_osc_register);

int qot_am335x_ext_osc_unregister(struct qot_properties* osc)
{
	return 0;
}
EXPORT_SYMBOL(qot_am335x_ext_osc_unregister);

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

		// Create a work queue for event capture
		queue_capture = create_workqueue("capture");
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
	
	// Don't allow interrupts during timer destruction
	spin_lock_irqsave(&qot_am335x_lock, flags);

	// Get the platform data
	pdata = pdev->dev.platform_data;
	if (pdata)
	{
		// Flush and destroy the work queues
		flush_workqueue(queue_capture);
		destroy_workqueue(queue_capture);

		// Unregister the clock source with the HAL
		qot_core_unregister(&pdata->clksrc);

		// Clean up the core timer
		qot_am335x_clocksource_cleanup(pdata->clksrc);
		if (core)
			qot_am335x_timer_cleanup(pdata->core, pdev);

		// Clean up the individual timers
		if (pdata->timer4) 
			qot_am335x_timer_cleanup(pdata->timer4, pdev);
		if (pdata->timer5) 
			qot_am335x_timer_cleanup(pdata->timer5, pdev);
		if (pdata->timer6) 
			qot_am335x_timer_cleanup(pdata->timer6, pdev);
		if (pdata->timer7) 
			qot_am335x_timer_cleanup(pdata->timer7, pdev);

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
