/*
 * @file qot_timeline.c
 * @brief Kernel HRTIMER Interface to manage QoT timelines
 * @author Sandeep D'souza and Adwait Dongare
 *
 * Copyright (c) Carnegie Mellon University, 2015. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright notice,
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
 */

// include files
#include <linux/module.h>
#include <linux/kernel.h>       // printk
#include <linux/rbtree.h>       // rbtree functionality
#include <linux/hrtimer.h>      // hrtimer functions and data structures
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
#include <linux/timecounter.h>

// Local includes
#include "qot_internal.h"

// data structures
// Sleeper data structure for the sleeping task -> encapsulates a pointer to the task struct
struct timeline_sleeper {
    struct rb_node tl_node;             // RB tree node for timeline event
    struct qot_timeline *timeline;      // timeline this belongs to
    struct hrtimer timer;               // hrtimer
    struct task_struct *task;           // task this belongs to
    ktime_t qot_expires;
};


// For now here, should go into the timeline_struct
struct timespec epsilon = {0,1000000};      // the allowable timing error between event request & execution


// function definitions

asmlinkage long sys_timeline_nanosleep(char __user *timeline_id, struct timespec __user *exp_time);
asmlinkage long sys_set_offset(char __user *timeline_id, s64 __user *off);
asmlinkage long sys_print_timeline(char __user *uuid);

// Functions to interface with the HRTIMER subsystem
static int timeline_event_add(struct rb_root *head, struct timeline_sleeper *sleeper);
static enum hrtimer_restart interface_wakeup(struct hrtimer *timer);
static void interface_init_sleeper(struct timeline_sleeper *sl, struct task_struct *task, struct qot_timeline *timeline, ktime_t expires);
static ktime_t translate_time(struct timeline_sleeper *sleeper);

// Functions to interface with the HRTIMER subsystem and the QoT API
int interface_update(struct rb_root *timeline_root);
int interface_nanosleep(struct timespec *exp_time, struct qot_timeline *timeline);

/* Function Definitions */

// add timeline_sleeper node to specified rb tree. tree nodes are ordered on expiry time
static int timeline_event_add(struct rb_root *head, struct timeline_sleeper *sleeper) {
    struct rb_node **new = &(head->rb_node), *parent = NULL;
    int result;
    while(*new) {
        struct timeline_sleeper *this = container_of(*new, struct timeline_sleeper, tl_node);
        // order wrt expiry time
        result = ktime_compare(sleeper->qot_expires, this->qot_expires);
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

// Translates time from the global timeline notion of time to system time
static ktime_t translate_time(struct timeline_sleeper *sleeper) {
    ktime_t new;
    struct qot_timeline *timeline;

    s64 qot_expiry;
    s64 new_nstime;
    timeline = sleeper->timeline;
    qot_expiry = ktime_to_ns(sleeper->qot_expires);
    new_nstime = div_s64((qot_expiry - timeline->nsec),(1+(div_s64(timeline->mult, 1000000000ULL)))) + timeline->last;
    new = ns_to_ktime((u64) new_nstime);

    printk(KERN_INFO "[update time]: %lld -> %lld\n", sleeper->timer._softexpires.tv64, new.tv64);
    return new;
}

/* hrtimer callback wakes up a task*/
static enum hrtimer_restart interface_wakeup(struct hrtimer *timer) {
    struct timeline_sleeper *t;
    struct task_struct *task;
    t = container_of(timer, struct timeline_sleeper, timer);
    task = t->task;
    t->task = NULL;
    if(task)
        wake_up_process(task);

    return HRTIMER_NORESTART;
}

/*initializes the hrtimer sleeper structure*/
static void interface_init_sleeper(struct timeline_sleeper *sl, struct task_struct *task, struct qot_timeline *timeline, ktime_t expires)
{
    sl->timer.function = interface_wakeup;
    sl->task = task;
    sl->timeline = timeline;
    sl->qot_expires = expires;
    timeline_event_add(&timeline->event_head, sl);
}

/* Updates hrtimer softexpires when a time change happens, called by the set_time adj_time functions*/
int interface_update(struct rb_root *timeline_root) {
    ktime_t new_softexpires;
    ktime_t current_sys_time;

    struct hrtimer *timer;
    struct timeline_sleeper *sleeping_task;
    struct task_struct *task;
    int missed_events = 0; /* Returns a non zero number incase a task has to be woken up midway the mod of the non zero number is the number of tasks which had to be woken up */
    int ret;

    struct rb_node *timeline_node = NULL;
    /*Get the current system time*/
    current_sys_time = ktime_get_real();

    timeline_node = rb_first(timeline_root);

    for (timeline_node = rb_first(timeline_root); timeline_node != NULL;) {
        sleeping_task = container_of(timeline_node, struct timeline_sleeper, tl_node);
        timer = &sleeping_task->timer;
        task = sleeping_task->task;

        new_softexpires = translate_time(sleeping_task);

        printk(KERN_INFO "[interface_update] task %d updated: t_exp %lld -> %lld\n", task->pid, timer->_softexpires.tv64, new_softexpires.tv64);

        timeline_node = rb_next(timeline_node);     // change node pointer to next node for next iteration

        // check if our time has expired
        if(ktime_compare(new_softexpires, current_sys_time) <= 0) {
            // we missed our event. wake up process & let it know
            printk(KERN_INFO "[interface_update] task %d missed due to changed notion of time\n", task->pid);
            interface_wakeup(timer);            // this is a callback function. NEEDED
            missed_events++;                    // increment number of missed events
        }
        else
        {
            // reprogram timer with new value
            printk(KERN_INFO "[interface_update] task %d timer reprogrammed\n", task->pid);
            if(hrtimer_active(timer)) {     // cancel old if active
                hrtimer_cancel(timer);
            }
            hrtimer_init_on_stack(timer, CLOCK_REALTIME, HRTIMER_MODE_ABS);
            timer->function = interface_wakeup;
            hrtimer_set_expires(timer, new_softexpires);       // set new expiry time
            ret = hrtimer_start_expires(timer, HRTIMER_MODE_ABS);   // start new values
        }
    }
    return -missed_events;
}

// Puts the task to sleep on a timeline using the HRTIMER Interface
int interface_nanosleep(struct timespec *exp_time, struct qot_timeline *timeline)
{

    struct timespec t_now, delta;
    struct timeline_sleeper sleep_timer;
    struct qot_timeline *tl = timeline;
    ktime_t sys_time_expiry;

    // Initialize the HRTIMER and the sleeper structure

    hrtimer_init_on_stack(&sleep_timer.timer, CLOCK_REALTIME, HRTIMER_MODE_ABS);
    spin_lock(&sleep_timer.timeline->rb_lock);
    interface_init_sleeper(&sleep_timer, current, tl, timespec_to_ktime(*exp_time));
    spin_unlock(&sleep_timer.timeline->rb_lock);
    sys_time_expiry = translate_time(&sleep_timer);
    hrtimer_set_expires(&sleep_timer.timer, sys_time_expiry);

    do {
        set_current_state(TASK_INTERRUPTIBLE);
        hrtimer_start_expires(&sleep_timer.timer, HRTIMER_MODE_ABS);
        if (!hrtimer_active(&sleep_timer.timer))
            sleep_timer.task = NULL;

        if (likely(sleep_timer.task)) {

            //printk(KERN_INFO"[sys_timeline_nanosleep] task %d sleeping at %ld.%09lu\n", current->pid, t_now.tv_sec, t_now.tv_nsec);
            freezable_schedule();
        }

        if(!(sleep_timer.task && !signal_pending(sleep_timer.task)))
        {
            spin_lock(&sleep_timer.timeline->rb_lock);
            rb_erase(&sleep_timer.tl_node, &sleep_timer.timeline->event_head);
            spin_unlock(&sleep_timer.timeline->rb_lock);
        }
        hrtimer_cancel(&sleep_timer.timer);
    } while (sleep_timer.task && !signal_pending(sleep_timer.task));

     __set_current_state(TASK_RUNNING);

    // get system time for comparision
    getnstimeofday(&t_now);
    printk(KERN_INFO"[sys_timeline_nanosleep] task %d woke up at %ld.%09lu\n", current->pid, t_now.tv_sec, t_now.tv_nsec);

    // check if t_now - t_wake > epsilon
    delta = timespec_sub(t_now, *exp_time);
    if(timespec_compare(&delta, &epsilon) <= 0) {
        return 0;
    } else {
        //printk(KERN_INFO "[sys_timeline_nanosleep] task %d waited too long\n", current->pid);
        return -EBADE;
    }
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("CMU");
MODULE_DESCRIPTION("QoT HRTIMER Interface");







