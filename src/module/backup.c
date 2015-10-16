// Basic kernel module includes
#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/siginfo.h>
#include <linux/rcupdate.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/clk.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/pinctrl/consumer.h>
#include <linux/clocksource.h>
#include <linux/version.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <asm/uaccess.h>

// OMAP-specific dual mode timer code
#include "../linux/arch/arm/plat-omap/include/plat/dmtimer.h"

// Expose roseline module to userspace (ioctl) and other modules (export)
#include "roseline_ioctl.h"

// Module information
#define MODULE_NAME "roseline"
#define FIRST_MINOR 0
#define MINOR_CNT 1
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andrew Symington");
MODULE_DESCRIPTION("Linux kernel driver for quality of time");
MODULE_VERSION("0.3.0");

// This is a fixed offset added to the timer syncrhonization to account for the fact that
// the actual instructions used to write to the hardware counter take a few cycles to 
// execute on the microcontroller. It was derivide empirically.
#define KLUDGE_FACTOR 13

// Defferent capture call types
typedef enum 
{
	FROM_CAPTURE_OVERFLOW,
	FROM_COMPARE_MATCH,
	FROM_COMPARE_OVERFLOW,
	FROM_INIT
}
CaptureCallTypes;

// Data specific to a clock - both timers will be driven by the same underlying oscillator.
// However, time will be kept based on the 'compare timer' as it makes events easier to
// schedule. The consequence of this fact is that each input/shadow capture event produces
// a t
struct roseline_clock_data {
	struct omap_dm_timer 	*capture_timer;	// Timer handle for capture
	struct omap_dm_timer 	*compare_timer;	// Timer handle for compare
	char 					timer_name[16];	// Iotimer name
	int                     ready;			// Initialised and no IRQ being serviced
	uint32_t                frequency;		// Clock frequency
	uint32_t                offset;			// Offset between capture and compare
	struct clocksource 		clksrc;			// Clock source
	roseline_capt  			capt;			// Capture data 
	roseline_comp			comp;			// Compare data
	roseline_sync			sync;			// Synchronization data
	roseline_pars			pars;			// QoT data
};

// Platform data is collection of clocks
struct roseline_platform_data {
	struct roseline_clock_data clk[NUM_TIMERS];	// All the clocks
};

// This is the platform data for the system
struct roseline_platform_data *roseline_pdata = NULL;

/////////////////////////////////////////////////////////////////////////////////////////

// This function

static void roseline_event_scheduler(struct roseline_clock_data *pdata, int type)
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

			// This is a short routine to test how well we have aligned the clocks. In principle,
			// compare_count - capture_count + l should be zero. However, you cannot in practice 
			// instantaneously copy the count from one timer to another. Thus, the instructions on
			// line 120 and 121 of this file require a KLUDGE_FACTOR, which represents the fixed
			// number of cycles between reading the capture timer, adding an offset and writing the
			// compare timer. To determinne the offset I read the counters sequentially, switching 
			// order every other iteration to mitigate bias. I hand-selected the KLUDGE_FACTOR to
			// be a value that resilted in the difference printed as 0 on average.
			// int j;
			// int ttot;
			// ttot = 0;
			// for (j = 0; j < 1000; j++)
			// {
			// 	uint32_t tcap;
			// 	uint32_t tcom;
			// 	if (j % 2)
			// 	{
			// 		tcap = omap_dm_timer_read_counter(pdata->capture_timer);
			// 		tcom = omap_dm_timer_read_counter(pdata->compare_timer);
			// 	}
			// 	else
			// 	{
			// 		tcom = omap_dm_timer_read_counter(pdata->compare_timer);
			// 		tcap = omap_dm_timer_read_counter(pdata->capture_timer);
			// 	}
			// 	ttot += tcom - tcap + l;
			// }
			// ttot = ttot / 10;
			// ttot = ttot;
			// pr_info("Difference: %d\n",ttot);
			// 			
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

///////////////////////////////////////////////////////////////////////////////////////////

// Required for ioctl
static dev_t dev;
static struct cdev c_dev;
static struct class *cl;

// What to do when the ioctl is started
static int roseline_ioctl_open(struct inode *i, struct file *f)
{
	return 0;
}

// What to do when the ioctl is stopped
static int roseline_ioctl_close(struct inode *i, struct file *f)
{
	return 0;
}

// What to do when a user queries the ioctl
static long roseline_ioctl_access(struct file *f, unsigned int cmd, unsigned long arg)
{
	struct roseline_clock_data *pdata;
	roseline_sync tmp_sync;
	roseline_pars tmp_pars;
	uint64_t seq;
	int id;

	switch (cmd)
	{
	
	// Get the capture parameters (called by the time synchronization application)
	case ROSELINE_GET_CAPTURE:

		// Check for the requested timer ID
		id = ((roseline_capt*)arg)->id;
		if (id < 0 || id > NUM_TIMERS)
			return -EACCES;

		// Immediately, read and record the counter at request time
		if (roseline_pdata)
		{
			// Get the pointer to data for the current clock
			pdata = &(roseline_pdata->clk[id]);

			// Record the count at the time of request
			pdata->capt.count_at_request = omap_dm_timer_read_counter(pdata->capture_timer);

			// Memcpy the current timer value
			if (copy_to_user((roseline_capt*)arg, &(pdata->capt), sizeof(roseline_capt)))
				return -EACCES;
		}
		else 
			pr_info("Error: roseline_pdata unallocated\n");	
		
		break;

	// Set the compare function to trigger an external event at some time
	case ROSELINE_SET_COMPARE:

		// Check for the requested timer ID
		id = ((roseline_comp*)arg)->id;
		if (id < 0 || id > NUM_TIMERS)
			return -EACCES;

		// Immediately, read and record the counter at request time
		if (roseline_pdata)
		{
			// Get the pointer to data for the current clock
			pdata = &(roseline_pdata->clk[id]);

			// Memcpy the current compare argument into a local structure
			if (copy_from_user(&(pdata->comp), (roseline_comp*)arg, sizeof(roseline_comp)))
				return -EACCES;

			// Temprary hack which is used for testing
			if (!pdata->comp.next)
			{
				// Set the offset to 1s from now
				pdata->comp.next = (((uint64_t) pdata->capt.overflow) << 32) 
					|  ((uint64_t)(omap_dm_timer_read_counter(pdata->capture_timer) + 24000000));
			}

			// Force an update to see if we need to shedule a match for this count sequence
			roseline_event_scheduler(pdata, FROM_INIT);
		}
		else 
			pr_info("Error: roseline_pdata unallocated\n");	
		
		break;

	// Get the syncronization parameters for a given timer (typically called by the scheduler)
	case ROSELINE_GET_SYNC:

		// Check for the requested timer ID
		id = ((roseline_sync*)arg)->id;
		if (id < 0 || id > NUM_TIMERS)
			return -EACCES;

		// Immediately, read and record the counter at request time
		if (roseline_pdata)
		{
			// Get the pointer to data for the current clock
			pdata = &(roseline_pdata->clk[id]);

			// Memcpy the current timer value
			if (copy_to_user((roseline_sync*)arg, &(pdata->sync), sizeof(roseline_sync)))
				return -EACCES;
		}
		else 
			pr_info("Error: roseline_pdata unallocated\n");	

		break;

	// Set the synchronization parameters (called by the user-space timesync application)
	case ROSELINE_SET_SYNC:

		// Check for the requested timer ID
		id = ((roseline_sync*)arg)->id;
		if (id < 0 || id > NUM_TIMERS)
			return -EACCES;

		// Immediately, read and record the counter at request time
		if (roseline_pdata)
		{
			// Get the pointer to data for the current clock
			pdata = &(roseline_pdata->clk[id]);

			// Memcpy the current sync value
			seq = pdata->sync.seq;
			if (copy_from_user(&tmp_sync, (roseline_sync*)arg, sizeof(roseline_sync)))
				return -EACCES;

			// Copy over the sync parameters and variances
			memcpy(pdata->sync.pars,tmp_sync.pars,sizeof(pdata->sync.pars));
			memcpy(pdata->sync.vars,tmp_sync.vars,sizeof(pdata->sync.vars));
			pdata->sync.seq = seq + 1;
		}
		else 
			pr_info("Error: roseline_pdata unallocated\n");	

		break;

	// Get the syncronization parameters for a given timer (typically called by the scheduler)
	case ROSELINE_GET_PARS:

		// Check for the requested timer ID
		id = ((roseline_pars*)arg)->id;
		if (id < 0 || id > NUM_TIMERS)
			return -EACCES;

		// Immediately, read and record the counter at request time
		if (roseline_pdata)
		{
			// Get the pointer to data for the current clock
			pdata = &(roseline_pdata->clk[id]);

			// Memcpy the current timer value
			if (copy_to_user((roseline_pars*)arg, &(pdata->pars), sizeof(roseline_pars)))
				return -EACCES;
		}
		else 
			pr_info("Error: roseline_pdata unallocated\n");	

		break;

	// Set the synchronization parameters (called by the user-space timesync application)
	case ROSELINE_SET_PARS:

		// Check for the requested timer ID
		id = ((roseline_pars*)arg)->id;
		if (id < 0 || id > NUM_TIMERS)
			return -EACCES;

		// Immediately, read and record the counter at request time
		if (roseline_pdata)
		{
			// Get the pointer to data for the current clock
			pdata = &(roseline_pdata->clk[id]);

			// Memcpy the current sync parmeters
			seq = pdata->pars.seq;
			if (copy_from_user(&tmp_pars, (roseline_pars*)arg, sizeof(roseline_pars)))
				return -EACCES;
			pdata->pars.error = tmp_pars.error;		// Target error
			pdata->pars.seq = seq + 1;
		}
		else 
			pr_info("Error: roseline_pdata unallocated\n");	

		break;

	// Not a valid request
	default:
		return -EINVAL;
	}

	// Success!
	return 0;
}

// Define the file operations over th ioctl
static struct file_operations roseline_fops =
{
	.owner = THIS_MODULE,
	.open = roseline_ioctl_open,
	.release = roseline_ioctl_close,
	.unlocked_ioctl = roseline_ioctl_access
};

// Iitialise the ioctl
static int roseline_ioctl_init(const char *name)
{
	int ret;
	struct device *dev_ret;
	if ((ret = alloc_chrdev_region(&dev, FIRST_MINOR, MINOR_CNT, name)) < 0)
	{
		return ret;
	}
	cdev_init(&c_dev, &roseline_fops);
	if ((ret = cdev_add(&c_dev, dev, MINOR_CNT)) < 0)
	{
		return ret;
	} 
	if (IS_ERR(cl = class_create(THIS_MODULE, name)))
	{
		cdev_del(&c_dev);
		unregister_chrdev_region(dev, MINOR_CNT);
		return PTR_ERR(cl);
	}
	if (IS_ERR(dev_ret = device_create(cl, NULL, dev, NULL, name)))
	{
		class_destroy(cl);
		cdev_del(&c_dev);
		unregister_chrdev_region(dev, MINOR_CNT);
		return PTR_ERR(dev_ret);
	}
	return 0;
}

// Destroy the IOCTL
static void roseline_ioctl_exit(void)
{
	device_destroy(cl, dev);
	class_destroy(cl);
	cdev_del(&c_dev);
	unregister_chrdev_region(dev, MINOR_CNT);
}

// CLOCKSOURCE STUFF //////////////////////////////////////////////////////////////////////////

static cycle_t roseline_read_cycles_timer_a(struct clocksource *cs)
{
	return (cycle_t)__omap_dm_timer_read_counter(roseline_pdata->clk[IOTIMER_A].capture_timer, 
		roseline_pdata->clk[IOTIMER_A].capture_timer->posted);
}

static cycle_t roseline_read_cycles_timer_b(struct clocksource *cs)
{
	return (cycle_t)__omap_dm_timer_read_counter(roseline_pdata->clk[IOTIMER_B].capture_timer, 
		roseline_pdata->clk[IOTIMER_B].capture_timer->posted);
}

static void roseline_clocksource_init(struct roseline_clock_data *pdata)
{
	pdata->clksrc.name = pdata->timer_name;
	pdata->clksrc.rating = 299;
	switch (pdata->capt.id)
	{
		case IOTIMER_A: pdata->clksrc.read = roseline_read_cycles_timer_a; break;
		case IOTIMER_B: pdata->clksrc.read = roseline_read_cycles_timer_b; break;
	}
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

// TIMER STUFF ////////////////////////////////////////////////////////////////////////////////

// Interrupt handler for capture timers
static irqreturn_t roseline_capture_interrupt(int irq, void *data)
{
	// Prealloc
	unsigned int irq_status;
	struct roseline_clock_data *pdata;
	pdata = data;

	if (!pdata->ready) 
		return IRQ_HANDLED;

	// Prevent race conditions
	pdata->ready = 0;

	// If the OMAP_TIMER_INT_CAPTURE flag in the IRQ status is set
	irq_status = omap_dm_timer_read_status(pdata->capture_timer);
	if (irq_status & OMAP_TIMER_INT_CAPTURE)
	{
		pdata->capt.count_at_interrupt = omap_dm_timer_read_counter(pdata->capture_timer);
		pdata->capt.count_at_capture = __omap_dm_timer_read(pdata->capture_timer, 
			OMAP_TIMER_CAPTURE_REG, pdata->capture_timer->posted);
		pdata->capt.capture++;
		__omap_dm_timer_write_status(pdata->capture_timer, OMAP_TIMER_INT_CAPTURE);
	}

	// If the overflow flag in the IRQ status is set
	if (irq_status & OMAP_TIMER_INT_OVERFLOW)
	{
		// Overflow increment (capture)
		pdata->capt.overflow++;
		
		// Overflow increment (sync parameters)
		pdata->sync.seq++;								// Bump sequence number (polling)
		pdata->sync.overflow++;							// Bump overflow counter
		pdata->sync.frequency = pdata->capt.frequency;	// Save frequency

		// Force an update to see if we need to shedule a match for this count sequence
		roseline_event_scheduler(pdata, FROM_CAPTURE_OVERFLOW);

		//  IRQ dealt with
		__omap_dm_timer_write_status(pdata->capture_timer, OMAP_TIMER_INT_OVERFLOW);
	}

	// Prevent race conditions
	pdata->ready = 1;

	// Success!
	return IRQ_HANDLED;
}

// Interrupt handler for compare timers
static irqreturn_t roseline_compare_interrupt(int irq, void *data)
{
	// Prealloc
	unsigned int irq_status;
	struct roseline_clock_data *pdata;
	pdata = data;

	if (!pdata->ready) 
		return IRQ_HANDLED;

	// Prevent race conditions
	pdata->ready = 0;

	// If the OMAP_TIMER_INT_CAPTURE flag in the IRQ status is set
	irq_status = omap_dm_timer_read_status(pdata->compare_timer);

	// If the overflow flag in the IRQ status is set
	if (irq_status & OMAP_TIMER_INT_OVERFLOW)
	{
		// Force an update to see if we need to shedule a match for this count sequence
		roseline_event_scheduler(pdata, FROM_COMPARE_OVERFLOW);

		//  IRQ dealt with
		__omap_dm_timer_write_status(pdata->compare_timer, OMAP_TIMER_INT_OVERFLOW);
	}

	// A MATCH event has occured
	if (irq_status & OMAP_TIMER_INT_MATCH)
	{
		// Force an update to see if we need to shedule a match for this count sequence
		roseline_event_scheduler(pdata, FROM_COMPARE_MATCH);

		//  IRQ dealt with
		__omap_dm_timer_write_status(pdata->compare_timer, OMAP_TIMER_INT_MATCH);
	}

	// Prevent race conditions
	pdata->ready = 1;

	// Success!
	return IRQ_HANDLED;
}

// Set up the timer capture 
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


// Set up the timer compare 
static void omap_dm_timer_setup_compare(struct omap_dm_timer *timer, int timer_irqcfg)
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
	ctrl |= timer_irqcfg;
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

// Use a given clock : OMAP_TIMER_SRC_EXT_CLK, OMAP_TIMER_SRC_32_KHZ, OMAP_TIMER_SRC_SYS_CLK
static void omap_dm_timer_use_clk(struct roseline_clock_data *pdata, int clk)
{
	struct clk *gt_fclk;
	omap_dm_timer_set_source(pdata->capture_timer, clk);
	omap_dm_timer_set_source(pdata->compare_timer, clk);
	gt_fclk = omap_dm_timer_get_fclk(pdata->capture_timer);
	pdata->frequency = clk_get_rate(gt_fclk);
	switch (clk)
	{
	case OMAP_TIMER_SRC_SYS_CLK: 
		pr_info("%s switched to SYS, rate=%uHz\n", pdata->timer_name, pdata->frequency); 
		break;
	case OMAP_TIMER_SRC_32_KHZ:  
		pr_info("%s switched to RTC, rate=%uHz\n", pdata->timer_name, pdata->frequency); 
		break;
	case OMAP_TIMER_SRC_EXT_CLK: 
		pr_info("%s switched to TCLKIN, rate=%uHz\n", pdata->timer_name, pdata->frequency); 
		break;
	}  
}

// Enable the interrupt request
static void roseline_enable_irq(struct roseline_clock_data *pdata)
{
	unsigned int interrupt_mask;

	// For the capture timer
	interrupt_mask = OMAP_TIMER_INT_CAPTURE | OMAP_TIMER_INT_OVERFLOW;
	__omap_dm_timer_int_enable(pdata->capture_timer, interrupt_mask);
	pdata->capture_timer->context.tier = interrupt_mask;
	pdata->capture_timer->context.twer = interrupt_mask;

	// For the compare timer
	interrupt_mask = OMAP_TIMER_INT_MATCH | OMAP_TIMER_INT_OVERFLOW;
	__omap_dm_timer_int_enable(pdata->compare_timer, interrupt_mask);
	pdata->compare_timer->context.tier = interrupt_mask;
	pdata->compare_timer->context.twer = interrupt_mask;
}

// Initialise a timer
static int roseline_init_timer(const char *timer_name, struct roseline_clock_data *pdata, 
	struct device_node *timer_i, struct device_node *timer_o)
{
	// Used to check for successful device tree read
	const char *tmp;

	// Capture timer
	of_property_read_string_index(timer_i, "ti,hwmods", 0, &tmp);
	if (!tmp)
	{
		pr_err("capture: ti,hwmods property missing?\n");
		return -ENODEV;
	}
	pdata->capture_timer = omap_dm_timer_request_by_node(timer_i);
	if (!pdata->capture_timer)
	{
		pr_err("capture: request_by_node failed\n");
		return -ENODEV;
	}
	if (request_irq(pdata->capture_timer->irq, roseline_capture_interrupt, IRQF_TIMER, MODULE_NAME, pdata))
	{
		pr_err("capture: cannot register IRQ %d\n", pdata->capture_timer->irq);
		return -EIO;
	}

	// Compare timer
	of_property_read_string_index(timer_o, "ti,hwmods", 0, &tmp);
	if (!tmp)
	{
		pr_err("compare: ti,hwmods property missing?\n");
		return -ENODEV;
	}
	pdata->compare_timer = omap_dm_timer_request_by_node(timer_o);
	if (!pdata->compare_timer)
	{
		pr_err("compare: request_by_node failed\n");
		return -ENODEV;
	}
	if (request_irq(pdata->compare_timer->irq, roseline_compare_interrupt, IRQF_TIMER, MODULE_NAME, pdata))
	{
		pr_err("compare: cannot register IRQ %d\n", pdata->compare_timer->irq);
		return -EIO;
	}

	// Dynamically allocate memory for the timer name
	if (strlen(timer_name) < 16)
		strcpy(pdata->timer_name, timer_name);
	else
	{
		pr_err("timer name must be less than 16 characters\n");
		return -EIO;
	}

	// Success!
	return 0;
}

// Cleanup a timer
static void roseline_cleanup_timer(struct roseline_clock_data *pdata)
{
	// Free the capture timer
	if (pdata->capture_timer)
	{
	    omap_dm_timer_set_source(pdata->capture_timer, OMAP_TIMER_SRC_SYS_CLK); // In case TCLKIN is stopped during boot
	    omap_dm_timer_set_int_disable(pdata->capture_timer, OMAP_TIMER_INT_CAPTURE | OMAP_TIMER_INT_OVERFLOW);
	    free_irq(pdata->capture_timer->irq, pdata);
	    omap_dm_timer_stop(pdata->capture_timer);
	    omap_dm_timer_free(pdata->capture_timer);
	    pdata->capture_timer = NULL;
	}

	// Free the compare timer
	if (pdata->compare_timer)
	{
	    omap_dm_timer_set_source(pdata->compare_timer, OMAP_TIMER_SRC_SYS_CLK); // In case TCLKIN is stopped during boot
	    omap_dm_timer_set_int_disable(pdata->compare_timer, OMAP_TIMER_INT_MATCH | OMAP_TIMER_INT_OVERFLOW);
	    free_irq(pdata->compare_timer->irq, pdata);
	    omap_dm_timer_stop(pdata->compare_timer);
	    omap_dm_timer_free(pdata->compare_timer);
	    pdata->compare_timer = NULL;
	}
}

// Get platform data using a device tree handle
static struct roseline_platform_data *of_get_roseline_pdata(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node, *timer_i, *timer_o;
	struct roseline_platform_data *pdata;
	const __be32 *timer_phandle;
	struct of_phandle_args timer_args;
	int timer_irqcfg, timer_source;
	const char *timer_name;
	int err = 0, i = 0;

  	// Get the 
	pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata)
		return NULL;

 	// iterate over the four timers	
	for (i = 0; i < NUM_TIMERS; i++)
	{
  		// ready = false
		pdata->clk[i].ready = 0;

		// Get the correct configuration option from the device tree
		switch(i)
		{	
		case IOTIMER_A : timer_name = "iotimer_a"; break;
		case IOTIMER_B : timer_name = "iotimer_b"; break;
			default: 
			pr_err("roseline: cannot find timer name");
			err = 1;
		break;
		}

		// Get a handle to the timer definition in the device tree
		timer_phandle = of_get_property(np, timer_name, NULL);
		if (!timer_phandle)
		{
			pr_err("roseline: cannot find phandle");
			err = 1;
		} 
		if (of_parse_phandle_with_fixed_args(np, timer_name, 3, 0, &timer_args) < 0)
		{
			pr_err("roseline: could not parse timer%d arguments\n",i+4);
			err = 1;
		}

		// Get the input timer
		timer_i = of_find_node_by_phandle(be32_to_cpup(timer_phandle));
		if (!timer_i)
		{
			pr_err("roseline: INPUT timer_phandle find_node_by_phandle failed\n");
			err = 1;
		}

		// Get the output timer
		timer_o = of_find_node_by_phandle(timer_args.args[0]);
		if (!timer_o)
		{
			pr_err("roseline: OUTPUT timer_phandle find_node_by_phandle failed\n");
			err = 1;
		}

		// Initialize the timers
		if (roseline_init_timer(timer_name,&(pdata->clk[i]),timer_i,timer_o) < 0)
			err = 1;

		// Determine
		timer_irqcfg = OMAP_TIMER_CTRL_TCM_LOWTOHIGH;
		switch (timer_args.args[2])
		{
			case 0: timer_irqcfg = OMAP_TIMER_CTRL_TCM_LOWTOHIGH; break;
			case 1: timer_irqcfg = OMAP_TIMER_CTRL_TCM_HIGHTOLOW; break;
			case 2: timer_irqcfg = OMAP_TIMER_CTRL_TCM_BOTHEDGES; break;
			case 3: break;
			default: 
			pr_err("the value %d is not a valid capture IRQ configuration\n",timer_args.args[2]);
			err = 1;
			break;
		}
		
		// Setup the timers
		omap_dm_timer_setup_capture(pdata->clk[i].capture_timer, timer_irqcfg);
		omap_dm_timer_setup_compare(pdata->clk[i].compare_timer, timer_irqcfg);

  		// Setup the clock sources for each
		timer_source = OMAP_TIMER_SRC_SYS_CLK;
		switch (timer_args.args[1])
		{
			case 0: timer_source = OMAP_TIMER_SRC_EXT_CLK; break;
			case 1: timer_source = OMAP_TIMER_SRC_SYS_CLK; break;
			case 2: timer_source = OMAP_TIMER_SRC_32_KHZ; 	break;
			default: 
			pr_err("the value %d is not a valid source\n",timer_args.args[1]);
			err = 1;
			break;
		}
		omap_dm_timer_use_clk(&(pdata->clk[i]),timer_source);

		// Clear the device tree node
		of_node_put(timer_i);
	}

 	// If there's an error, clean up correctly
	if (err)
	{
		devm_kfree(&pdev->dev, pdata);	
		return NULL;
	}

  	// Return data
	return pdata;
}

static const struct of_device_id roseline_dt_ids[] = {
	{ .compatible = "roseline", },
  	{                           }
};

MODULE_DEVICE_TABLE(of, roseline_dt_ids);

// Initialise the kernel driver
static int roseline_probe(struct platform_device *pdev)
{
	const struct of_device_id *match;
	struct pinctrl *pinctrl;
	int i = 0;

  	// Try and find a device that matches "compatible: roseline"
	match = of_match_device(roseline_dt_ids, &pdev->dev);
	if (match)
		pdev->dev.platform_data = of_get_roseline_pdata(pdev);
	else
		pr_err("of_match_device failed\n");

  	// Get the platform data
	roseline_pdata = pdev->dev.platform_data;
	if (!roseline_pdata)
		return -ENODEV;

  	// Setup pin control
	pinctrl = devm_pinctrl_get_select_default(&pdev->dev);
	if (IS_ERR(pinctrl))
		pr_warning("pins are not configured from the driver\n");

  	// Set all clocks ready
	for (i = 0; i < NUM_TIMERS; i++)
		roseline_pdata->clk[i].ready = 1;

  	// Create a clock source for the given timer, allowing linux to be clocked by it
	for (i = 0; i < NUM_TIMERS; i++)
		roseline_clocksource_init(&(roseline_pdata->clk[i]));

  	// Enable interrupts on the clocks
	for (i = 0; i < NUM_TIMERS; i++)
		roseline_enable_irq(&(roseline_pdata->clk[i]));

  	// Init the ioctl
	roseline_ioctl_init("roseline");

	return 0;
}

// Remove the kernel driver
static int roseline_remove(struct platform_device *pdev)
{
  	// Kill the ioctl
	roseline_ioctl_exit();

  	// Kill the platform data
	if (roseline_pdata)
	{
		int i;
		for (i = 0; i < NUM_TIMERS; i++)
		{
			// Kill the clock source
			roseline_clocksource_cleanup(&(roseline_pdata->clk[i]));

			// Kill the timer
			roseline_cleanup_timer(&(roseline_pdata->clk[i]));
		}

    	// Free the platform data
		devm_kfree(&pdev->dev, roseline_pdata);
		pdev->dev.platform_data = NULL;
	}

  	// Free the driver data
	platform_set_drvdata(pdev, NULL);

  	// Success!
	return 0;
}

static struct platform_driver roseline_driver = {
  .probe    = roseline_probe,				// Called on module init
  .remove   = roseline_remove,				// Called on module kill
	.driver   = {
		.name = MODULE_NAME,			
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(roseline_dt_ids),
	},
};

module_platform_driver(roseline_driver);

*/