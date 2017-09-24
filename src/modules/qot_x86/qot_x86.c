/*
 * @file qot_x86.c
 * @brief Driver that exposes x86 (or any architecture lacking pins) as a PTP clock for the QoT stack
 * @author Sandeep D'souza
 *
 * Copyright (c) Carnegie Mellon University, 2017.
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
#include <linux/version.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/hrtimer.h>

/* We are going to export the whol x86 as a PTP clock */
#include <linux/ptp_clock_kernel.h>

/*QoT Core Clock Registration*/
#include "../qot/qot_core.h"


/* Useful definitions */
#define MODULE_NAME 			"qot_x86"

// PLATFORM DATA ///////////////////////////////////////////////////////////////

struct qot_x86_data;

// Global Pointer for quick access
static struct qot_x86_data *qot_x86_data_ptr = NULL;

// Global starting offset of core clock
s64 offset;

// Scheduler Interface
struct qot_x86_sched_interface {
	struct qot_x86_data *parent;					/* Pointer to parent      */
	struct hrtimer timer;                           /* HRTIMER for scheduling */
	long (*callback)(void);                         /* QoT Callback           */
};

// Platform Data Structure
struct qot_x86_data {
	raw_spinlock_t lock;						/* Protects timer registers */
	struct ptp_clock *clock;					/* PTP clock */
	struct ptp_clock_info info;					/* PTP clock info */
	struct qot_clock_impl *qot_x86_impl_info; 	/* QoT Info */
	struct qot_x86_sched_interface core_sched;	/* Scheduler Interface */
};

// PTP CLOCK FUNCTIONALITY //////////////////////////////////////////////////////////
static int qot_x86_adjfreq(struct ptp_clock_info *ptp, s32 ppb)
{
	// Core Clock is Strictly Monotonic and Raw (no frequency adjustments or jumps)
	return -EOPNOTSUPP;
}

static int qot_x86_adjtime(struct ptp_clock_info *ptp, s64 delta)
{
	// Core Clock is Strictly Monotonic and Raw (no frequency adjustments or jumps)
	return -EOPNOTSUPP;
}

static int qot_x86_gettime(struct ptp_clock_info *ptp, struct timespec64 *ts)
{
	struct timespec raw_monotonic_ts;
	unsigned long flags;

	struct qot_x86_data *pdata = container_of(
    	ptp, struct qot_x86_data, info);
	raw_spin_lock_irqsave(&pdata->lock, flags);
	// Grab a raw monotonic timestamp from the clocksource
	//getrawmonotonic(&raw_monotonic_ts);
	ktime_get_ts(&raw_monotonic_ts);
	raw_spin_unlock_irqrestore(&pdata->lock, flags);
	*ts = timespec_to_timespec64(timespec_add(raw_monotonic_ts, ns_to_timespec(offset)));
	return 0;
}


static int qot_x86_settime(struct ptp_clock_info *ptp, const struct timespec64 *ts)
{
	// Core Clock is Strictly Monotonic and Raw (no frequency adjustments or jumps)
	return -EOPNOTSUPP;
}

static int qot_x86_enable(struct ptp_clock_info *ptp, struct ptp_clock_request *rq, int on)
{
	// Platform without pins does not support this functionality 
	// Extend function for platforms which support pins
	return -EOPNOTSUPP;
}

static int qot_x86_verify(struct ptp_clock_info *ptp, unsigned int pin,
                             enum ptp_pin_function func, unsigned int chan)
{
	// Platform without pins does not support this functionality 
	// Extend function for platforms which support pins
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

static struct ptp_clock_info qot_x86_info = {
	.owner		= THIS_MODULE,
	.name		= "Generic timer",
	.max_adj	= 1000000,
	.n_pins		= 0,
	.n_alarm    = 0,	/* Will be changed by probe */
	.n_ext_ts	= 0,	/* Will be changed by probe */
	.n_per_out  = 0,	/* Will be changed by probe */
	.pps		= 0,
	.adjfreq	= qot_x86_adjfreq,
	.adjtime	= qot_x86_adjtime,
	.gettime64	= qot_x86_gettime,
	.settime64	= qot_x86_settime,
	.enable		= qot_x86_enable,
	.verify     = qot_x86_verify,
};

// CORE TIME READ FUNCTIONALITY///////////////////////////////////////////////////////
static timepoint_t qot_x86_read_time(void)
{
	s64 ns;
	struct timespec raw_monotonic_ts;
	timepoint_t time_now;
	unsigned long flags;
	struct qot_x86_data *pdata;
	pdata = qot_x86_data_ptr;

	raw_spin_lock_irqsave(&pdata->lock, flags);
	// Grab a raw monotonic timestamp from the clocksource
	//getrawmonotonic(&raw_monotonic_ts);
	ktime_get_ts(&raw_monotonic_ts);
	raw_spin_unlock_irqrestore(&pdata->lock, flags);
	ns = offset + timespec_to_ns(&raw_monotonic_ts);
	TP_FROM_nSEC(time_now, (s64)ns);
	return time_now;
}

static long qot_x86_timeline_enable_compare(timepoint_t *core_start, timelength_t *core_period, qot_perout_t *perout, qot_return_t (*callback)(qot_perout_t *perout_ret, timepoint_t *event_core_timestamp, timepoint_t *next_event), int on)
{
	// No HW Pins, not supported
	return -EOPNOTSUPP;
}

// SCHEDULER INTERFACE TIMER ///////////////////////////////////////////////////
// Interface function to qot_core, used to program the scheduler interface interrupt using a timepoint_t value
static long qot_x86_program_sched_interrupt(timepoint_t expiry, int force, long (*callback)(void))
{
	unsigned long flags;
	struct qot_x86_sched_interface *interface;
	u64 ns;
	struct timespec ns_ts;
	u64 expiry_ns;
	struct qot_x86_data *pdata;
	
	pdata = qot_x86_data_ptr;
	interface = &pdata->core_sched;
	expiry_ns = TP_TO_nSEC(expiry);
	raw_spin_lock_irqsave(&pdata->lock, flags);
	//getrawmonotonic(&ns_ts);
	ktime_get_ts(&ns_ts);
	raw_spin_unlock_irqrestore(&pdata->lock, flags);
	ns = (u64)timespec_to_ns(&ns_ts);

	// Check if expiry is not behind current time Else return error code
	if(expiry_ns <= ns)
	{
		return -EINVAL;
	}
	interface->callback = callback;
	// May need to consider using pinned timers
	hrtimer_start_range_ns(&interface->timer, ns_to_ktime(expiry_ns-offset), 0ULL, HRTIMER_MODE_ABS);
	return 0;
}

// Interface function to qot_core, used to cancel a programmed interrupt
static long qot_x86_cancel_sched_interrupt(void)
{
	struct qot_x86_sched_interface *interface;
	interface = &qot_x86_data_ptr->core_sched;
	hrtimer_start_range_ns(&interface->timer, ns_to_ktime(KTIME_MAX), 0ULL, HRTIMER_MODE_ABS);
	return 0;
}

// Sched Timer Interrupt (uses HRTIMER Callback)
static enum hrtimer_restart qot_x86_sched_interface_interrupt(struct hrtimer *timer)
{
	struct qot_x86_sched_interface *interface = &qot_x86_data_ptr->core_sched;
	/* Call the QoT Scheduler Routine to Wakeup Tasks*/
	if(interface->callback)
		interface->callback();
	return HRTIMER_NORESTART;
}

// POWER MANAGEMENT FUNCTIONS /////////////////////////////////////////////////////////

// Puts the Oscillator into Sleep Mode -> this is a stub
static long qot_x86_sleep(void)
{
	return 0;
}

// Restarts the oscillator from sleep -> this is a stub
static long qot_x86_wake(void)
{
	return 0;
}


// MODULE CLEANUP /////////////////////////////////////////////////////////

static void qot_x86_cleanup(struct qot_x86_data *pdata)
{
	pr_info("qot_x86: Cleaning up...\n");
	if (pdata) {
		/* Remove the PTP clock */
		ptp_clock_unregister(pdata->clock);
	}
}

// QoT PLATFORM CLOCK ABSTRACTIONS //////////////////////////////////////////////////////////////
struct qot_clock qot_x86_properties = { // Does not contain accurate values meeds to be filled TODO
	.name = "qot_x86",
	.nom_freq_nhz = 24000000000, // Incorrect -> Depends on the clocksource being used
	.nom_freq_nwatt = 1000, // Rough guess -> Change
};

struct qot_clock_impl qot_x86_impl_info = {
	.read_time = qot_x86_read_time,
	.program_interrupt =  qot_x86_program_sched_interrupt,
	.cancel_interrupt = qot_x86_cancel_sched_interrupt,
	.enable_compare = qot_x86_timeline_enable_compare,
	.sleep = qot_x86_sleep,
	.wake = qot_x86_wake
};

/// MODULE INITIALIZATION
static struct qot_x86_data *qot_x86_initialize(struct platform_device *pdev)
{
	//struct device *dev = &pdev->dev;
	struct qot_x86_data *pdata = NULL;

	/* Try allocate platform data */
	pr_info("qot_x86: Allocating platform data...\n");
	pdata = devm_kzalloc(&pdev->dev, sizeof(struct qot_x86_data), GFP_KERNEL);
	if (!pdata)
		goto err;

	/* Initialize a Global Variable for easy reference later */
	qot_x86_data_ptr = pdata;

	/* Initialize spin lock for protecting time registers */
	pr_info("qot_x86: Initializing spinlock...\n");
	raw_spin_lock_init(&pdata->lock);

	/* Initialize Platform Clock Data Structures */
	pdata->info           = qot_x86_info;
	pdata->info.n_pins    = 0;
	pdata->info.n_ext_ts  = 0;
	pdata->info.n_per_out = 0;

	/* Initialize Scheduler Interface Data Structures*/
	pdata->core_sched.callback = NULL;
	pdata->core_sched.parent   = pdata;
	hrtimer_init(&pdata->core_sched.timer, CLOCK_MONOTONIC, HRTIMER_MODE_ABS);
	pdata->core_sched.timer.function = qot_x86_sched_interface_interrupt;

	/* Initialize Pointer to QoT Clock Data*/
	pdata->qot_x86_impl_info = &qot_x86_impl_info;

	/* Initialize a PTP clock */
	pr_info("qot_x86: Initializing PTP clock...\n");
	pdata->clock = ptp_clock_register(&pdata->info, &pdev->dev);
	if (IS_ERR(pdata->clock)) {
		pr_err("qot_x86: problem creating PTP interface\n");
		goto err;
	}

	qot_x86_properties.phc_id = ptp_clock_index(pdata->clock);
	pr_info("qot_x86: PTP clock id is %d\n", qot_x86_properties.phc_id);
	
	// Setting a global offset w.r.t clock_realtime (synced up to UTC if NTP is running)
	offset = ktime_to_ns(ktime_sub(ktime_get_real(),ktime_get())); // Set this to 0 to remove
	pr_info("qot_x86: Start offset is %lld\n", offset);
	/* Return the platform data */
	return pdata;

err:
	qot_x86_cleanup(pdata);
	return NULL;
}



// MODULE LOADING AND UNLOADING  ///////////////////////////////////////////////
static int qot_x86_probe(struct platform_device *pdev)
{
	timelength_t read_lat_estimate;		/* Estimate of Read Latency */
	timeinterval_t read_lat_interval;	/* Uncertainty interval around Read Latency */
	timelength_t int_lat_estimate;		/* Estimate of Interrupt Latency */
	timeinterval_t int_lat_interval;	/* Uncertainty interval around Interrupt Latency */

	// Initialize the platform device data
	if (pdev->dev.platform_data)
		qot_x86_cleanup(pdev->dev.platform_data);
	pdev->dev.platform_data = qot_x86_initialize(pdev);
	if (!pdev->dev.platform_data)
		return -ENODEV;

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
	qot_x86_properties.read_latency.estimate = read_lat_estimate;
	qot_x86_properties.read_latency.interval = read_lat_interval;
	
	qot_x86_properties.interrupt_latency.estimate = int_lat_estimate;
	qot_x86_properties.interrupt_latency.interval = int_lat_interval;

	qot_x86_impl_info.info = qot_x86_properties;

	/* QoT PTP Clock Info */
	qot_x86_impl_info.ptpclk = qot_x86_info;
	if(qot_register(&qot_x86_impl_info))
		return -EACCES;
	return 0;
}

static int qot_x86_remove(struct platform_device *pdev)
{
	if(qot_unregister(&qot_x86_impl_info))
		return -EACCES;
	if (pdev->dev.platform_data) {
		qot_x86_cleanup(pdev->dev.platform_data);
		devm_kfree(&pdev->dev, pdev->dev.platform_data);
		pdev->dev.platform_data = NULL;
	}
	platform_set_drvdata(pdev, NULL);
	return 0;
}

static struct platform_driver qot_x86_driver = {
	.probe    = qot_x86_probe,
	.remove   = qot_x86_remove,
	.driver   = {
		.name = MODULE_NAME,
		.owner = THIS_MODULE,
	},
};

static struct platform_device qot_x86_device = {
	.name = MODULE_NAME,
};

int qot_x86_init(void)
{
    /* Registering with Kernel */
    platform_driver_register(&qot_x86_driver);
    platform_device_register(&qot_x86_device);
    return 0;
}

void qot_x86_exit(void)
{
    /* Unregistering from Kernel */
    platform_device_unregister(&qot_x86_device);
    platform_driver_unregister(&qot_x86_driver);
    return;
}

module_init(qot_x86_init);
module_exit(qot_x86_exit);

//module_platform_driver(qot_x86_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sandeep Dsouza");
MODULE_DESCRIPTION("Linux x86 QoT Clock Driver for x86");
MODULE_VERSION("0.1.0");
