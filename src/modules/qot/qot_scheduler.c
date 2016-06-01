/*
 * @file qot_scheduler.c
 * @brief QoT Interface to the Linux Scheduler
 * @author Sandeep D'souza
 *
 * Copyright (c) Carnegie Mellon University, 2016.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright notice,
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

#include <linux/module.h>
#include <linux/kernel.h>       // printk
#include <linux/rbtree.h>       // rbtree functionality
#include <linux/time.h>         // timespec & operations
#include <linux/slab.h>         // kmalloc, kfree
#include <linux/init.h>         // __init & __exit macros
#include <linux/timekeeping.h>  // for getnstimeofday
#include <asm/unistd.h>         // syscall values
#include <linux/freezer.h>      // for freezable_schedule
#include <linux/ptp_clock_kernel.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/spinlock.h>
#include <linux/signal.h>
#include <linux/sched.h>

#include "qot_core.h"
#include "qot_timeline.h"
#include "qot_clock.h"

// Core Time at which the interrupt will trigger a callback
timepoint_t next_interrupt_callback = {MAX_TIMEPOINT_SEC, 0};

// Scheduler subsystem spin lock
raw_spinlock_t qot_scheduler_lock;

// Sleeper data structure for the sleeping task -> encapsulates a pointer to the task struct
struct timeline_sleeper {
    struct rb_node tl_node;               // RB tree node for timeline event 
    struct qot_timeline *timeline;        // timeline this belongs to
    struct task_struct *task;             // task this belongs to
    pid_t pid;                            // task pid
    bool sleeper_active;				  // flag to show if the timeline sleeper is active and enqueued
    timepoint_t qot_expires;			  // Expiry time as per the timeline notion of time
    timeinterval_t qot_expires_interval;  // Uncertainity interval in the time estimate
    bool periodic_timer_flag;			  // flag to indicate it the timer should be periodically enqueued
    qot_timer_t timer;                    // Periodic Timer 
    int timer_counts;                     // Number of Periodic Timer callbacks completed

};

// add timeline_sleeper node to specified rb tree. tree nodes are ordered on expiry time
static int qot_timeline_event_add(struct rb_root *head, struct timeline_sleeper *sleeper) {
    struct rb_node **new = &(head->rb_node), *parent = NULL;
    int result;
    while(*new) {
        struct timeline_sleeper *this = container_of(*new, struct timeline_sleeper, tl_node);
        // order wrt expiry time
        result = timepoint_cmp(&this->qot_expires, &sleeper->qot_expires);
        parent = *new;
        if (result < 0)
            new = &((*new)->rb_left);
        else
            new = &((*new)->rb_right);
    }
    /* Add new node and rebalance tree. */
    rb_link_node(&sleeper->tl_node, parent, new);
    rb_insert_color(&sleeper->tl_node, head);
    return 0;
}

// Converts Core Time to a Remote Timeline Time -> Modify this function
static timepoint_t qot_core_to_remote(timepoint_t core_time, struct qot_timeline *timeline)
{
	timepoint_t remote_time;
	s64 nsec_time = TP_TO_nSEC(core_time);
    qot_loc2rem(timeline->index, 0, &nsec_time);
    TP_FROM_nSEC(remote_time, nsec_time);
	return remote_time;
}

// Converts Timeline Time to a Core Timeline Time -> Modify this function
static timepoint_t qot_remote_to_core(timepoint_t remote_time, struct qot_timeline *timeline)
{
	timepoint_t core_time;
	s64 nsec_time = TP_TO_nSEC(remote_time);
    qot_rem2loc(timeline->index, 0, &nsec_time);
    TP_FROM_nSEC(core_time, nsec_time);
	return core_time;
}

// Function which wakes up (for blocking waits) /signals (for timers) tasks -> Called with spinlock held
static int qot_sleeper_wakeup(struct timeline_sleeper *sleeper, struct rb_node **timeline_node) 
{    
    *timeline_node = rb_next(*timeline_node);
    if(sleeper->periodic_timer_flag == 1)
    {
    	utimepoint_t time_now;
    	struct task_struct *task;
    	struct siginfo info;
		memset(&info, 0, sizeof(struct siginfo));
		info.si_signo = SIGALRM;
		info.si_code = SI_QUEUE;

		//send signal to user process
		task = pid_task(find_vpid(sleeper->pid), PIDTYPE_PID); //find_task_by_pid(sleeper->pid);
		if(task != NULL)
		{
			send_sig_info(SIGALRM, &info, sleeper->task);
			// Remove Existing Sleeper from the RB-Tree
			rb_erase(&sleeper->tl_node, &sleeper->timeline->event_head);
	    	// Add New Sleeper with new expiry information
	    	if(sleeper->timer_counts < sleeper->timer.count || sleeper->timer.count == 0)
	    	{
	    		sleeper->timer_counts++;
	    		timepoint_add(&sleeper->qot_expires, &sleeper->timer.period);
	    		qot_timeline_event_add(&sleeper->timeline->event_head, sleeper);
	    	}
	    	else
	    	{
	    		sleeper->sleeper_active = 0;
				sleeper->periodic_timer_flag = 0;
				qot_remove_binding_timer(sleeper->pid, sleeper->timeline);
			    kfree(sleeper);
	    	}
	    }
	    else
	    {
	    	rb_erase(&sleeper->tl_node, &sleeper->timeline->event_head);
	    	sleeper->sleeper_active = 0;
			sleeper->periodic_timer_flag = 0;
			qot_remove_binding_timer(sleeper->pid, sleeper->timeline);
		    kfree(sleeper);

	    }
    	
    }
    else if(sleeper->sleeper_active == 1)
    {
	    if(sleeper->task)
	        wake_up_process(sleeper->task);
	    sleeper->task = NULL;
	    sleeper->sleeper_active = 0;
	}
    return 0;
}

// Finds the next event to be programed -> Called from Interrupt context
static timepoint_t qot_get_next_event(void)
{
	qot_return_t retval;
	qot_timeline_t *timeline = NULL;	
	unsigned long flags;

	struct rb_root *timeline_root = NULL;	
	struct rb_node *timeline_node = NULL;
	struct timeline_sleeper *sleeping_task;	
	
	timepoint_t core_expires;

	timepoint_t expires_next = {MAX_TIMEPOINT_SEC, 0};

    raw_spin_lock_irqsave(&qot_timeline_lock, flags);
	retval = qot_timeline_first(&timeline);
	// Iterate over all the timelines to find earliest reprogramming instant
	while(retval != QOT_RETURN_TYPE_ERR)
	{
		raw_spin_lock_irqsave(&timeline->rb_lock, flags);
		timeline_root = &timeline->event_head;
		timeline_node = rb_first(timeline_root);
		while(timeline_node != NULL)
		{
			sleeping_task = container_of(timeline_node, struct timeline_sleeper, tl_node);	 
			if(sleeping_task->sleeper_active == 0)
	        {
	        	timeline_node = rb_next(timeline_node);
	            sleeping_task = container_of(timeline_node, struct timeline_sleeper, tl_node);	 
	        }
	        else
	        {
	        	break;
	        }
	    }
		// Choose the n+1 th node as the first n ones have already expired
		if(timeline_node != NULL)
		{
	        core_expires = qot_remote_to_core(sleeping_task->qot_expires, timeline);       
	        // Check if a task needs to be woken up
	        if(timepoint_cmp(&core_expires, &expires_next) > 0)
	        {
	        	expires_next = core_expires;
	        }
		}
		raw_spin_unlock_irqrestore(&timeline->rb_lock, flags);
		retval = qot_timeline_next(&timeline);
		if(retval == QOT_RETURN_TYPE_ERR)
			break;
	}
	raw_spin_unlock_irqrestore(&qot_timeline_lock, flags);


	//pr_info("qot_scheduler: qot_get_next_event is %lld %llu\n", expires_next.sec, expires_next.asec);

	if(expires_next.sec < 0)
		expires_next.sec = 0;
	if(expires_next.asec < 0)
		expires_next.asec = 0;
	return expires_next;
}

// Interrupt callback to wakeup and reprogram the interrupt timer
static long scheduler_interface_interrupt(void)
{
	int retries = 0;
	qot_return_t retval;
	qot_timeline_t *timeline = NULL;	
	struct rb_root *timeline_root = NULL;	
	struct rb_node *timeline_node = NULL;
	struct timeline_sleeper *sleeping_task;
	unsigned long flags;

	timepoint_t current_core_time;
	timepoint_t current_timeline_time;

	timepoint_t next_expires = {MAX_TIMEPOINT_SEC, 0};
	
	// Get the current core time
	qot_clock_get_core_time_raw(&current_core_time);

retry:
	// Get the first timeline
    raw_spin_lock_irqsave(&qot_timeline_lock, flags);
	retval = qot_timeline_first(&timeline);
	
	// Iterate over all the timelines to wake up tasks
	while(retval != QOT_RETURN_TYPE_ERR)
	{
		current_timeline_time = qot_core_to_remote(current_core_time, timeline);
		raw_spin_lock_irqsave(&timeline->rb_lock, flags);
		timeline_root = &timeline->event_head;
		for (timeline_node = rb_first(timeline_root); timeline_node != NULL;) 
		{
	        sleeping_task = container_of(timeline_node, struct timeline_sleeper, tl_node);	        

	        // Check if a task needs to be woken up
	        if(timepoint_cmp(&sleeping_task->qot_expires, &current_timeline_time) > 0)
	        { 
	        	// Move task to runqueue
	        	qot_sleeper_wakeup(sleeping_task, &timeline_node);
	        }
	        else
	        {
	        	break;
	        }
	        //timeline_node = rb_next(timeline_node);
		}
		raw_spin_unlock_irqrestore(&timeline->rb_lock, flags);
		retval = qot_timeline_next(&timeline);
		if(retval == QOT_RETURN_TYPE_ERR)
			break;
	}
	raw_spin_unlock_irqrestore(&qot_timeline_lock, flags);

	/* Reevaluate the clock bases for the next expiry */
	next_expires = qot_get_next_event();

    /* Reprogramming necessary ? */
    raw_spin_lock_irqsave(&qot_scheduler_lock, flags);
    if (!qot_clock_program_core_interrupt(next_expires, 0, scheduler_interface_interrupt)) 
    {
    	next_interrupt_callback = next_expires;
    	raw_spin_unlock_irqrestore(&qot_scheduler_lock, flags);
        return 0;
    }
    raw_spin_unlock_irqrestore(&qot_scheduler_lock, flags);
    /*
     * The next timer was already expired due to:
     * - tracing
     * - long lasting callbacks
     * - being scheduled away when running in a VM
     *
     * We need to prevent that we loop forever in the hrtimer
     * interrupt routine. We give it 3 attempts to avoid
     * overreacting on some spurious event.
     *
     */
    qot_clock_get_core_time_raw(&current_core_time);

    if (++retries < 3)
        goto retry;
    /* Reprogram the timer */
    raw_spin_lock_irqsave(&qot_scheduler_lock, flags);
    qot_clock_program_core_interrupt(next_expires, 1, scheduler_interface_interrupt);
    next_interrupt_callback = next_expires;
    raw_spin_unlock_irqrestore(&qot_scheduler_lock, flags);
    return 0;
}

// If the new node being programmed has to be woken up before already existing nodes, then reprogram the interrupt
static int qot_sleeper_start_expires(struct timeline_sleeper *sleeper, timepoint_t core_time_expiry)
{
	int retval = 0;
	unsigned long flags;
	timepoint_t time_now;

	qot_clock_get_core_time_raw(&time_now);
    if(timepoint_cmp(&core_time_expiry, &time_now) > 0)
        return QOT_RETURN_TYPE_ERR;

	if(timepoint_cmp(&core_time_expiry, &next_interrupt_callback) > 0 && sleeper->sleeper_active == 1)
	{
	    raw_spin_lock_irqsave(&qot_scheduler_lock, flags);
	    retval = qot_clock_program_core_interrupt(core_time_expiry, 0, scheduler_interface_interrupt);
	    if(!retval)
	    {
	        next_interrupt_callback = core_time_expiry;
	    }
	    raw_spin_unlock_irqrestore(&qot_scheduler_lock, flags);

	}
	return retval;
}

// initializes the qot timeline sleeper structure -> grab a spinlock before initializing
static void qot_init_sleeper(struct timeline_sleeper *sl, struct task_struct *task, struct qot_timeline *timeline, utimepoint_t *expires)
{
    sl->task = task;
    sl->pid  = task->pid;
    sl->timeline = timeline;
    sl->sleeper_active = 1; 
    sl->periodic_timer_flag = 0;
	sl->qot_expires = expires->estimate;
	sl->qot_expires_interval = expires->interval;
    qot_timeline_event_add(&timeline->event_head, sl);
}

// initializes the qot timeline sleeper structure -> grab a spinlock before initializing
static void qot_init_timer_sleeper(struct timeline_sleeper *sl, struct task_struct *task, struct qot_timeline *timeline, timelength_t *period, timepoint_t *start_offset, int count)
{
    timelength_t elapsed_time;
    timepoint_t core_time;
    timepoint_t current_timeline_time;

    u64 elapsed_ns = 0;
    u64 period_ns = 0;
    u64 num_periods = 0;
    u64 p_remainder = 0;

    // Populate essential variables
    sl->task = task;
    sl->pid  = task->pid;
    sl->timeline = timeline;
    sl->sleeper_active = 1;
    sl->periodic_timer_flag = 1;
    sl->timer.period = *period;
    sl->timer.start_offset = *start_offset;
    sl->timer.count = count;

    // Current Core Time
    qot_clock_get_core_time_raw(&core_time);

    // Current Timeline Time
    current_timeline_time = qot_core_to_remote(core_time, timeline);

    // Check Start Offset
    if(timepoint_cmp(start_offset, &current_timeline_time) < 0)
    {
        sl->qot_expires = *start_offset;
    }
    else 
    {
        // Calculate Next Wakeup Time
        timepoint_diff(&elapsed_time, &current_timeline_time, start_offset);
        elapsed_ns = TL_TO_nSEC(elapsed_time);
        period_ns = TL_TO_nSEC(sl->timer.period);
        num_periods = div64_u64_rem(elapsed_ns, period_ns, &p_remainder);
        if(p_remainder != 0)
            num_periods++;
        elapsed_ns = period_ns*num_periods;
        TL_FROM_nSEC(elapsed_time, elapsed_ns);
        timepoint_add(start_offset, &elapsed_time);
        sl->qot_expires = *start_offset;
     }
     // Start Offset Contains the Value when the timer must Start
     sl->timer_counts = 1;
     qot_timeline_event_add(&timeline->event_head, sl);
     return;
}

// Puts the task to sleep on a timeline 
int qot_attosleep(utimepoint_t *expiry_time, struct qot_timeline *timeline) 
{
    int retval = 0;
    struct timeline_sleeper sleep_timer;
    unsigned long flags;

    timepoint_t core_time_expiry;
    
    // Initialize the SLEEPER structure 
    raw_spin_lock_irqsave(&timeline->rb_lock, flags);
    qot_init_sleeper(&sleep_timer, current, timeline, expiry_time);
    raw_spin_unlock_irqrestore(&timeline->rb_lock, flags);

    core_time_expiry = qot_remote_to_core(expiry_time->estimate, timeline);
    do {
        set_current_state(TASK_INTERRUPTIBLE);
        retval = qot_sleeper_start_expires(&sleep_timer, core_time_expiry);
        if (retval != 0)
            sleep_timer.task = NULL;

        if (likely(sleep_timer.task)) 
        {
            freezable_schedule();
        }
    } while (sleep_timer.task && !signal_pending(current));
    
    raw_spin_lock_irqsave(&timeline->rb_lock, flags);
    rb_erase(&sleep_timer.tl_node, &sleep_timer.timeline->event_head);
    raw_spin_unlock_irqrestore(&timeline->rb_lock, flags);
    
     __set_current_state(TASK_RUNNING);
    return 0;
}

// Creates a periodic timer on a timeline 
int qot_timer_create(timelength_t *period, timepoint_t *start_offset, int count, qot_timer_t **timer, struct qot_timeline *timeline) 
{
    int retval = 0;
    struct timeline_sleeper *sleep_timer;
    unsigned long flags;

    timepoint_t core_time_expiry;

    sleep_timer = (struct timeline_sleeper*) kzalloc(sizeof(struct timeline_sleeper), GFP_KERNEL);
    if(sleep_timer == NULL)
    {
    	return QOT_RETURN_TYPE_ERR;
    }
    pr_info("Sleep Timer Created\n");
    // Initialize the SLEEPER structure 
    raw_spin_lock_irqsave(&timeline->rb_lock, flags);
    qot_init_timer_sleeper(sleep_timer, current, timeline, period, start_offset, count);
    raw_spin_unlock_irqrestore(&timeline->rb_lock, flags);

    core_time_expiry = qot_remote_to_core(*start_offset, timeline);
    retval = qot_sleeper_start_expires(sleep_timer, core_time_expiry);
    if (retval != 0)
    {
        pr_info("Wrong Retval\n");
        raw_spin_lock_irqsave(&timeline->rb_lock, flags);
	    rb_erase(&sleep_timer->tl_node, &sleep_timer->timeline->event_head);
	    kfree(sleep_timer);
	    raw_spin_unlock_irqrestore(&timeline->rb_lock, flags);
	    return QOT_RETURN_TYPE_ERR;
    }
    else
    {
    	*timer = &sleep_timer->timer;
    }
    return QOT_RETURN_TYPE_OK;
}

// destroys a Timer 
int qot_timer_destroy(qot_timer_t *timer, struct qot_timeline *timeline) 
{
	struct timeline_sleeper *sleep_timer;
	unsigned long flags;
	if(timer == NULL)
	{
		return QOT_RETURN_TYPE_ERR;
	}
	raw_spin_lock_irqsave(&timeline->rb_lock, flags);
	sleep_timer = container_of(timer, struct timeline_sleeper, timer);	
	if(sleep_timer)
	{
		sleep_timer->sleeper_active = 0;
		sleep_timer->periodic_timer_flag = 0;
	    rb_erase(&sleep_timer->tl_node, &sleep_timer->timeline->event_head);
	    kfree(sleep_timer);
	}
	raw_spin_unlock_irqrestore(&timeline->rb_lock, flags);
    return QOT_RETURN_TYPE_OK;
}

/* Updates timeline nodes waiting on a queue when a time change happens, called by the set_time adj_time functions*/
void qot_scheduler_update(qot_timeline_t *timeline)
{
	struct rb_root *timeline_root = NULL;	
	struct rb_node *timeline_node = NULL;
	struct timeline_sleeper *sleeping_task;	
	unsigned long flags;
	timepoint_t core_expires;
	timepoint_t current_core_time;

	timepoint_t expires_next = next_interrupt_callback;

	qot_clock_get_core_time_raw(&current_core_time);

	raw_spin_lock_irqsave(&timeline->rb_lock, flags);
	timeline_root = &timeline->event_head;
	timeline_node = rb_first(timeline_root);
	while (timeline_node != NULL)
	{
        sleeping_task = container_of(timeline_node, struct timeline_sleeper, tl_node);	 
        core_expires = qot_remote_to_core(sleeping_task->qot_expires, timeline);       
        // Check if a task needs to be woken up
        if(timepoint_cmp(&core_expires, &current_core_time) > 0)
        {
        	qot_sleeper_wakeup(sleeping_task, &timeline_node);
        }
        else if(timepoint_cmp(&core_expires, &expires_next) > 0)
        {
        	expires_next = core_expires;
        	break;
        }
	}
	raw_spin_unlock_irqrestore(&timeline->rb_lock, flags);

	if(expires_next.sec < 0)
		expires_next.sec = 0;
	if(expires_next.asec < 0)
		expires_next.asec = 0;
	qot_clock_get_core_time_raw(&current_core_time);
	if(timepoint_cmp(&current_core_time, &expires_next) > 0 && timepoint_cmp(&expires_next, &next_interrupt_callback) > 0)
	{
		raw_spin_lock_irqsave(&qot_scheduler_lock, flags);
		qot_clock_program_core_interrupt(expires_next, 1, scheduler_interface_interrupt);
		raw_spin_unlock_irqrestore(&qot_scheduler_lock, flags);
	}
	return;
}

/* Cleanup the timeline subsystem */
void qot_scheduler_cleanup(struct class *qot_class)
{
	/* TODO */
}

/* Initialize the timeline subsystem */
qot_return_t qot_scheduler_init(struct class *qot_class)
{
    /* TODO */
    raw_spin_lock_init(&qot_scheduler_lock);
    return QOT_RETURN_TYPE_OK;
}

MODULE_LICENSE("GPL");


