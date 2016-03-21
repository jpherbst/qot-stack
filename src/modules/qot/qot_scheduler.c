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

#include "qot_core.h"
#include "qot_timeline.h"
#include "qot_clock.h"

// Core Time at which the interrupt will trigger a callback
timepoint_t next_interrupt_callback = {TIMEPOINT_MAX_SEC, TIMEPOINT_MAX_ASEC};

// Scheduler subsystem spin lock
spinlock_t qot_scheduler_lock;

// Sleeper data structure for the sleeping task -> encapsulates a pointer to the task struct
struct timeline_sleeper {
    struct rb_node tl_node;               // RB tree node for timeline event 
    struct qot_timeline *timeline;        // timeline this belongs to
    struct task_struct *task;             // task this belongs to
    bool sleeper_active;				  // flag to show if the timeline sleeper is active and enqueued
    timepoint_t qot_expires;			  // Expiry time as per the timeline notion of time
    timeinterval_t qot_expires_interval;  // Uncertainity interval in the time estimate
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
	remote_time = core_time;
	return remote_time;
}

// Converts Timeline Time to a Core Timeline Time -> Modify this function
static timepoint_t qot_remote_to_core(timepoint_t remote_time, struct qot_timeline *timeline)
{
	timepoint_t core_time;
	core_time = remote_time;
	return core_time;
}

// Function which wakes up tasks
static int qot_sleeper_wakeup(struct timeline_sleeper *sleeper) 
{    
    if(sleeper->task)
        wake_up_process(sleeper->task);
    //pr_info("qot_scheduler:qot_sleeper_wakeup Task %d waking up\n", sleeper->task->pid);

    sleeper->task = NULL;
    sleeper->sleeper_active = 0;
    return 0;
}

// Finds the next event to be programed
static timepoint_t qot_get_next_event(void)
{
	qot_return_t retval;
	qot_timeline_t *timeline = NULL;	

	struct rb_root *timeline_root = NULL;	
	struct rb_node *timeline_node = NULL;
	struct timeline_sleeper *sleeping_task;	
	
	timepoint_t core_expires;

	timepoint_t expires_next = {TIMEPOINT_MAX_SEC, TIMEPOINT_MAX_ASEC};

	retval = qot_timeline_first(&timeline);
	// Iterate over all the timelines to find earliest reprogramming instant
	while(retval != QOT_RETURN_TYPE_ERR)
	{
		timeline_root = &timeline->event_head;
		timeline_node = rb_first(timeline_root);
		if(timeline_node != NULL)
		{
			sleeping_task = container_of(timeline_node, struct timeline_sleeper, tl_node);	 
			if(sleeping_task->sleeper_active == 0)
	        {
	        	timeline_node = rb_next(timeline_node);
	            sleeping_task = container_of(timeline_node, struct timeline_sleeper, tl_node);	 

	        }
	    }
		// Choose the second node as the first one has already expired
		if(timeline_node != NULL)
		{
	        core_expires = qot_remote_to_core(sleeping_task->qot_expires, timeline);       
	        // Check if a task needs to be woken up
	        if(timepoint_cmp(&core_expires, &expires_next) > 0)
	        {
	        	expires_next = core_expires;
	        }
		}
		retval = qot_timeline_next(&timeline);
		if(retval == QOT_RETURN_TYPE_ERR)
			break;
	}

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

	utimepoint_t current_core_time;
	timepoint_t current_timeline_time;

	timepoint_t next_expires = {TIMEPOINT_MAX_SEC, TIMEPOINT_MAX_ASEC};

	//raw_spin_lock(&core_lock);
	// Get the current core time
	//pr_info("qot_scheduler: scheduler_interface_interrupt reading core time\n");
	qot_clock_get_core_time(&current_core_time);
	//pr_info("qot_scheduler: scheduler_interface_interrupt core time was read\n");

retry:
	// Get the first timeline
	retval = qot_timeline_first(&timeline);
	
	// Iterate over all the timelines to wake up tasks
	while(retval != QOT_RETURN_TYPE_ERR)
	{
		current_timeline_time = qot_core_to_remote(current_core_time.estimate, timeline);
		timeline_root = &timeline->event_head;
		//pr_info("qot_scheduler: scheduler_interface_interrupt check timeline %d\n", timeline->index);
		for (timeline_node = rb_first(timeline_root); timeline_node != NULL;) 
		{
	        sleeping_task = container_of(timeline_node, struct timeline_sleeper, tl_node);	        

	        // Check if a task needs to be woken up
	        if(timepoint_cmp(&sleeping_task->qot_expires, &current_timeline_time) > 0)
	        { 
	        	// Move task to runqueue
	        	qot_sleeper_wakeup(sleeping_task);
	        }
	        else
	        {
	        	break;
	        }
	        timeline_node = rb_next(timeline_node);
		}
		retval = qot_timeline_next(&timeline);
		if(retval == QOT_RETURN_TYPE_ERR)
			break;
	}
	/* Reevaluate the clock bases for the next expiry */
	next_expires = qot_get_next_event();
	//raw_spin_unlock(&core_unlock);
    /* Reprogramming necessary ? */
    if (!qot_clock_program_core_interrupt(next_expires, scheduler_interface_interrupt)) 
    {
    	next_interrupt_callback = next_expires;
        return 0;
    }
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
    qot_clock_get_core_time(&current_core_time);

    if (++retries < 3)
        goto retry;
    /* Reprogram the timer */
    qot_clock_program_core_interrupt(next_expires, scheduler_interface_interrupt);
    next_interrupt_callback = next_expires;
    return 0;
}

// If the new node being programmed has to be woken up before already existing nodes, then reprogram the interrupt
static int qot_sleeper_start_expires(struct timeline_sleeper *sleeper, timepoint_t core_time_expiry)
{
	int retval = 0;
	unsigned long flags;
	utimepoint_t time_now;

	qot_clock_get_core_time(&time_now);
    if(timepoint_cmp(&core_time_expiry, &time_now.estimate) > 0)
        return QOT_RETURN_TYPE_ERR;

	//pr_info("qot_scheduler:qot_sleeper_start_expires Task %d going to sleep\n", sleeper->task->pid);
	spin_lock_irqsave(&qot_scheduler_lock, flags);
	//pr_info("qot_scheduler:qot_sleeper_start_expires Task %d going to sleep after lock\n", sleeper->task->pid);
	if(timepoint_cmp(&core_time_expiry, &next_interrupt_callback) > 0 && sleeper->sleeper_active == 1)
	{
	    next_interrupt_callback = core_time_expiry;
	    //pr_info("qot_scheduler:qot_sleeper_start_expires Task %d reprogramming\n", sleeper->task->pid);
	    retval = qot_clock_program_core_interrupt(core_time_expiry, scheduler_interface_interrupt);
	    //pr_info("qot_scheduler:qot_sleeper_start_expires Task %d programmed\n", sleeper->task->pid);

	}
	spin_unlock_irqrestore(&qot_scheduler_lock, flags);
	//pr_info("qot_scheduler:qot_sleeper_start_expires Task %d returning\n", sleeper->task->pid);
	return retval;
}

// initializes the qot timeline sleeper structure grab a spinlock before initializing
static void qot_init_sleeper(struct timeline_sleeper *sl, struct task_struct *task, struct qot_timeline *timeline, utimepoint_t *expires)
{
    sl->task = task;
    sl->timeline = timeline;
    sl->qot_expires = expires->estimate;
    sl->qot_expires_interval = expires->interval;
    sl->sleeper_active = 1;
    qot_timeline_event_add(&timeline->event_head, sl);
}

// Puts the task to sleep on a timeline 
int qot_attosleep(utimepoint_t *expiry_time, struct qot_timeline *timeline) 
{
    int retval = 0;
    struct timeline_sleeper sleep_timer;
    unsigned long flags;

    timepoint_t core_time_expiry;
    
    //pr_info("qot_scheduler:qot_attosleep Task %d trying to sleep until %lld %llu\n", current->pid, expiry_time->estimate.sec, expiry_time->estimate.asec);

    // Initialize the SLEEPER structure 
    spin_lock_irqsave(&timeline->rb_lock, flags);
    qot_init_sleeper(&sleep_timer, current, timeline, expiry_time);
    spin_unlock_irqrestore(&timeline->rb_lock, flags);
    //pr_info("qot_scheduler:qot_attosleep Task %d trying to sleep passed lock %lld %llu\n", current->pid, expiry_time->estimate.sec, expiry_time->estimate.asec);

    core_time_expiry = qot_remote_to_core(expiry_time->estimate, timeline);
    do {
        set_current_state(TASK_INTERRUPTIBLE);
        //pr_info("qot_scheduler:qot_attosleep Task %d trying to sleep passed lock 2 %lld %llu\n", current->pid, expiry_time->estimate.sec, expiry_time->estimate.asec);
        retval = qot_sleeper_start_expires(&sleep_timer, core_time_expiry);
        if (retval != 0)
            sleep_timer.task = NULL;
        //pr_info("qot_scheduler:qot_attosleep Task %d going to sleep\n", sleep_timer.task->pid);
        if (likely(sleep_timer.task)) 
        {
            freezable_schedule();
        }
    } while (sleep_timer.task && !signal_pending(current));
    
    spin_lock_irqsave(&timeline->rb_lock, flags);
    rb_erase(&sleep_timer.tl_node, &sleep_timer.timeline->event_head);
    spin_unlock_irqrestore(&timeline->rb_lock, flags);
    
     __set_current_state(TASK_RUNNING);
    return 0;
}
EXPORT_SYMBOL(qot_attosleep);

/* Updates timeline nodes waiting on a queue when a time change happens, called by the set_time adj_time functions*/
void qot_scheduler_update(void)
{
	qot_return_t retval;
	qot_timeline_t *timeline = NULL;	
	struct rb_root *timeline_root = NULL;	
	struct rb_node *timeline_node = NULL;
	struct timeline_sleeper *sleeping_task;	
	unsigned long flags;
	timepoint_t core_expires;

	timepoint_t expires_next = next_interrupt_callback;

	retval = qot_timeline_first(&timeline);
	
	// Iterate over all the timelines to find earliest reprogramming instant
	while(retval != QOT_RETURN_TYPE_ERR)
	{
		spin_lock_irqsave(&timeline->rb_lock, flags);
		timeline_root = &timeline->event_head;
		timeline_node = rb_first(timeline_root);
		if(timeline_node != NULL)
		{
	        sleeping_task = container_of(timeline_node, struct timeline_sleeper, tl_node);	 
	        core_expires = qot_remote_to_core(sleeping_task->qot_expires, timeline);       
	        // Check if a task needs to be woken up
	        if(timepoint_cmp(&core_expires, &expires_next) > 0)
	        {
	        	expires_next = core_expires;
	        }
		}
		spin_unlock_irqrestore(&timeline->rb_lock, flags);

		retval = qot_timeline_next(&timeline);
		if(retval == QOT_RETURN_TYPE_ERR)
			break;
	}

	if(expires_next.sec < 0)
		expires_next.sec = 0;
	if(expires_next.asec < 0)
		expires_next.asec = 0;
	spin_lock_irqsave(&qot_scheduler_lock, flags);
	qot_clock_program_core_interrupt(expires_next, scheduler_interface_interrupt);

	spin_unlock_irqrestore(&qot_scheduler_lock, flags);
	return;
}
EXPORT_SYMBOL(qot_scheduler_update);

/* Cleanup the timeline subsystem */
void qot_scheduler_cleanup(struct class *qot_class)
{
	/* TODO */
}

/* Initialize the timeline subsystem */
qot_return_t qot_scheduler_init(struct class *qot_class)
{
    /* TODO */
    spin_lock_init(&qot_scheduler_lock);
    return QOT_RETURN_TYPE_OK;
}

MODULE_LICENSE("GPL");


