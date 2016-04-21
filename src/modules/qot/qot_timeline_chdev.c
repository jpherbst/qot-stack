/*
 * @file qot_core.h
 * @brief Interfaces used by clocks to interact with core
 * @author Andrew Symington, Sandeep D'souza and Fatima Anwar
 *
 * Copyright (c) Regents of the University of California, 2015.
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

#include <linux/idr.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/posix-clock.h>
#include <linux/pps_kernel.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/spinlock.h>
#include <linux/sched.h>

#include "qot_admin.h"
#include "qot_clock.h"
#include "qot_timeline.h"
#include "qot_scheduler.h"

#define DEVICE_NAME "timeline"

/* PRIVATE */

static dev_t timeline_devt;
static struct class *timeline_class;

/* A map of all timelines in the system */
static DEFINE_IDR(qot_timelines_map);

static spinlock_t qot_timelines_lock;

/* Private data for a timeline, not visible outside this code      */
typedef struct timeline_impl {
    qot_timeline_t *info;       /* Timeline info                   */
    int index;                  /* IDR index for timeline          */
    dev_t devid;                /* Device ID                       */
    struct device *dev;         /* Device                          */
    struct posix_clock clock;   /* POSIX clock for this timeline   */
    u32 max_adj;                /* Discipline: maximum adjust      */
    s32 dialed_frequency;       /* Discipline: dialed frequency    */
    s64 last;                   /* Discipline: last cycle count of */
    s64 mult;                   /* Discipline: ppb multiplier      */
    s64 nsec;                   /* Discipline: global time offset  */
    s64 m_nsec;                 /* Discipline: global time for master  */
    s64 u_mult;                 /* Discipline: upper bound on ppb  */
    s64 l_mult;                 /* Discipline: lower bound on ppb  */
    spinlock_t lock;            /* Protects driver time registers  */
    struct idr idr_bindings;    /* IDR for trackng bindings        */
    struct list_head head_res;  /* Head pointing to resolution     */
    struct list_head head_low;  /* Head pointing to accuracy (low) */
    struct list_head head_upp;  /* Head pointing to accuracy (upp) */
    struct rb_root root;        /* Root of the RB-Tree of bindings */
} timeline_impl_t;

/* Private data for a binding, not visible outside this code       */
typedef struct binding_impl {
    qot_binding_t info;         /* Binding information                            */
    timeline_impl_t *parent;    /* Parent timeline                                */
    struct list_head list_res;  /* Head pointing to resolution                    */
    struct list_head list_low;  /* Head pointing to accuracy (low)                */
    struct list_head list_upp;  /* Head pointing to accuracy (upp)                */
    int pid;                    /* Tracks the thread which created the binding    */
    struct rb_node node;        /* Node on the RB Tree of bindings for a timeline */
} binding_impl_t;

/* Binding structure management */

/* Insert a new binding into the resolution list */
static inline void qot_insert_list_res(binding_impl_t *binding_list,
    struct list_head *head)
{
    binding_impl_t *bind_obj;
    list_for_each_entry(bind_obj, head, list_res) {
        if (timelength_cmp(&bind_obj->info.demand.resolution,
            &binding_list->info.demand.resolution)) {
            list_add_tail(&binding_list->list_res, &bind_obj->list_res);
            return;
        }
    }
    list_add_tail(&binding_list->list_res, head);
}

/* Insert a new binding into the accuracy lower list */
static inline void qot_insert_list_low(binding_impl_t *binding_list,
    struct list_head *head)
{
    binding_impl_t *bind_obj;
    list_for_each_entry(bind_obj, head, list_low) {
        if (timelength_cmp(&bind_obj->info.demand.accuracy.above,
            &binding_list->info.demand.accuracy.above)) {
            list_add_tail(&binding_list->list_low, &bind_obj->list_low);
            return;
        }
    }
    list_add_tail(&binding_list->list_low, head);
}

/* Insert a new binding into the accuracy upper list */
static inline void qot_insert_list_upp(binding_impl_t *binding_list,
    struct list_head *head)
{
    binding_impl_t *bind_obj;
    list_for_each_entry(bind_obj, head, list_upp) {
        if (timelength_cmp(&bind_obj->info.demand.accuracy.below,
            &binding_list->info.demand.accuracy.below)) {
            list_add_tail(&binding_list->list_upp, &bind_obj->list_upp);
            return;
        }
    }
    list_add_tail(&binding_list->list_upp, head);
}

/* Binding RB-Tree Management */

/* Find a binding using the pid of the thread which created the binding */
binding_impl_t *find_binding(struct rb_root *root, int pid)
{
    struct rb_node *node = root->rb_node;

    while (node) 
    {
        binding_impl_t *binding = container_of(node, binding_impl_t, node);
        int result;

        if(pid < binding->pid)
           result = -1;
        else if (pid > binding->pid)
           result = 1;
        else
           result = 0;

        if (result < 0)
            node = node->rb_left;
        else if (result > 0)
            node = node->rb_right;
        else
            return binding;
    }
    return NULL;
}

/* Insert a binding into the RB-Tree Corresponding to the timeline */
int insert_binding(struct rb_root *root, binding_impl_t *binding)
{
    struct rb_node **new = &(root->rb_node), *parent = NULL;

    /* Figure out where to put new node */
    while (*new) 
    {
        binding_impl_t *this = container_of(*new, binding_impl_t, node);
        int result; // = strcmp(data->keystring, this->keystring);
          
        if(binding->pid < this->pid)
            result = -1;
        else if (binding->pid > this->pid)
            result = 1;
        else
            result = 0;

        parent = *new;
        if (result < 0)
            new = &((*new)->rb_left);
        else if (result > 0)
            new = &((*new)->rb_right);
        else
            return QOT_RETURN_TYPE_ERR;
    }

    /* Add new node and rebalance tree. */
    rb_link_node(&binding->node, parent, new);
    rb_insert_color(&binding->node, root);

    return QOT_RETURN_TYPE_OK;
}

/* Add a new binding to a specified timeline */
static binding_impl_t *qot_binding_add(timeline_impl_t *timeline_impl,
    qot_binding_t *info)
{
    unsigned long flags;
    binding_impl_t *binding_impl = NULL;
    if (!timeline_impl || !info)
        return NULL;
    binding_impl = kzalloc(sizeof(binding_impl_t), GFP_KERNEL);
    if (!binding_impl)
        return NULL;
    /* Cache info and a pointer to the parent */
    memcpy(&binding_impl->info,info,sizeof(qot_binding_t));
    binding_impl->parent = timeline_impl;

    // /* Allocate an ID for this binding */
    // idr_preload(GFP_KERNEL);
    // spin_lock_irqsave(&timeline_impl->lock, flags);
    // binding_impl->info.id = idr_alloc(&timeline_impl->idr_bindings,
    //     (void*) binding_impl,0,0,GFP_NOWAIT);
    // spin_unlock_irqrestore(&timeline_impl->lock, flags);
    // idr_preload_end();

    /* Store the context (pid of the task) from which this binding has been invoked -> Sandeep*/
    binding_impl->pid = current->pid;

    /* Add the binding to the RB-Tree corresponding to the timeline using the pid of the task as id */
    spin_lock_irqsave(&timeline_impl->lock, flags);
    /* Check if a binding has already been created before */
    if(insert_binding(&timeline_impl->root, binding_impl))
    {
        spin_unlock_irqrestore(&timeline_impl->lock, flags);
        return NULL;
    }
    binding_impl->info.id = current->pid;
    spin_unlock_irqrestore(&timeline_impl->lock, flags);
    /* Insert its metrics into the parent timeline's linked list */
    qot_insert_list_res(binding_impl, &timeline_impl->head_res);
    qot_insert_list_low(binding_impl, &timeline_impl->head_low);
    qot_insert_list_upp(binding_impl, &timeline_impl->head_upp);
    /* Success */
    return binding_impl;
}

/* Remove a binding from a specified timeline */
static void qot_binding_del(binding_impl_t *binding_impl)
{
    unsigned long flags;
    /* Remove the binding from the parallel lists */
    if (!binding_impl)
        return;
    list_del(&binding_impl->list_res);
    list_del(&binding_impl->list_low);
    list_del(&binding_impl->list_upp);
    /* Remove the IDR reservation */
    // if (binding_impl->parent)
    //     idr_remove(&binding_impl->parent->idr_bindings,binding_impl->info.id);
    /* Remove the binding from the RB-Tree */
    if (binding_impl->parent)
    {
        spin_lock_irqsave(&binding_impl->parent->lock, flags);
        rb_erase(&binding_impl->node, &binding_impl->parent->root);
        spin_unlock_irqrestore(&binding_impl->parent->lock, flags);
    }
    /* Free the memory */
    kfree(binding_impl);
}

// CLOCK OPERATIONS //////////////////////////////////////////////////////////////////

/* 
    In the first instance we'll represent the relationship between the slave clock 
    and the master clock in a linear way. If we had really high precision floating
    point math then we could do something like this:
                        
                                1.0 + error
    master time at last discipline  |
                |                   |         core ns elapsed since last disciplined
        ________|_________   _______|______   _________________|________________
        T = timeline->nsec + timeline->skew * (driver->read() - timeline->last);

    We'll reduce this to the following...

        T = timeline->nsec 
          +                   (driver->read() - timeline->last) // Number of ns
          + timeline->mult  * (driver->read() - timeline->last) // PPB correction 

*/

// BASIC TIME PROJECTION FUNCTIONS /////////////////////////////////////////////

qot_return_t qot_loc2rem(int index, int period, s64 *val)
{
    timeline_impl_t *timeline_impl = idr_find(&qot_timelines_map, index);
    
    if(timeline_impl == NULL)
        return QOT_RETURN_TYPE_ERR;

    if (period)
        *val += div_s64(timeline_impl->mult * (*val),1000000000ULL);
    else
    {
        *val -= (s64) timeline_impl->last;
        *val  = timeline_impl->nsec + (*val) + div_s64(timeline_impl->mult * (*val),1000000000ULL);
    }
    return QOT_RETURN_TYPE_OK;
}

qot_return_t qot_rem2loc(int index, int period, s64 *val)
{
    timeline_impl_t *timeline_impl = idr_find(&qot_timelines_map, index);
    
    if(timeline_impl == NULL)
        return QOT_RETURN_TYPE_ERR;

    if (period)
        *val = div_u64((u64)(*val), (u64) (timeline_impl->mult + 1000000000ULL))*1000000000ULL ;
    else
    {
        u32 rem;
        u64 diff = (u64)(*val - timeline_impl->nsec);
        u64 quot = div_u64_rem(diff, (timeline_impl->mult + 1000000000ULL), &rem);
        *val = timeline_impl->last + (quot * 1000000000ULL) + rem; 
    }

    return QOT_RETURN_TYPE_OK;
}


/* Discipline operations */

static int qot_timeline_chdev_adj_adjfreq(struct posix_clock *pc, s32 ppb)
{
    utimepoint_t utp;
    s64 ns;
    unsigned long flags;
    timeline_impl_t *timeline_impl = container_of(pc,timeline_impl_t,clock);
    spin_lock_irqsave(&timeline_impl->lock, flags);
    if (qot_clock_get_core_time(&utp))
    {
    	spin_unlock_irqrestore(&timeline_impl->lock, flags);
        return 1;
    }
    ns = TP_TO_nSEC(utp.estimate);
    timeline_impl->nsec += (ns - timeline_impl->last)
        + div_s64(timeline_impl->mult * (ns - timeline_impl->last),1000000000ULL);
    timeline_impl->mult = ppb;
    timeline_impl->last  = ns;
    spin_unlock_irqrestore(&timeline_impl->lock, flags);
    qot_scheduler_update(timeline_impl->info);
    return 0;
}

static int qot_timeline_chdev_adj_adjtime(struct posix_clock *pc, s64 delta)
{
    utimepoint_t utp;
    s64 ns;
    unsigned long flags;
    timeline_impl_t *timeline_impl = container_of(pc,timeline_impl_t,clock);
    spin_lock_irqsave(&timeline_impl->lock, flags);
    if (qot_clock_get_core_time(&utp))
    {
    	spin_unlock_irqrestore(&timeline_impl->lock, flags);
        return 1;
    }

    ns = TP_TO_nSEC(utp.estimate);
    timeline_impl->nsec += (ns - timeline_impl->last)
        + div_s64(timeline_impl->mult * (ns - timeline_impl->last),1000000000ULL) + delta;
    timeline_impl->last = ns;
    spin_unlock_irqrestore(&timeline_impl->lock, flags);
    qot_scheduler_update(timeline_impl->info);
    return 0;
}

static s32 qot_timeline_chdev_ppm_to_ppb(long ppm)
{
    s64 ppb = 1 + ppm;
    ppb *= 125;
    ppb >>= 13;
    return (s32) ppb;
}

/* File operations */

static int qot_timeline_chdev_getres(struct posix_clock *pc,
    struct timespec *tp)
{
    tp->tv_sec  = 0;
    tp->tv_nsec = 1;
    return 0;
}

static int qot_timeline_chdev_settime(struct posix_clock *pc,
    const struct timespec *tp)
{
    utimepoint_t utp;
    s64 ns;
    unsigned long flags;
    timeline_impl_t *timeline_impl = container_of(pc,timeline_impl_t,clock);
    spin_lock_irqsave(&timeline_impl->lock, flags);
    if (qot_clock_get_core_time(&utp))
    {
    	spin_unlock_irqrestore(&timeline_impl->lock, flags);
        return 1;
    }

    ns = TP_TO_nSEC(utp.estimate);
    timeline_impl->last = ns;
    timeline_impl->nsec = timespec_to_ns(tp);
    spin_unlock_irqrestore(&timeline_impl->lock, flags);
    qot_scheduler_update(timeline_impl->info);
    return 0;
}

static int qot_timeline_chdev_gettime(struct posix_clock *pc,
    struct timespec *tp)
{
    utimepoint_t utp;
    s64 ns;
    unsigned long flags;
    timeline_impl_t *timeline_impl = container_of(pc,timeline_impl_t,clock);
    spin_lock_irqsave(&timeline_impl->lock, flags);
    if (qot_clock_get_core_time(&utp))
    {
    	spin_unlock_irqrestore(&timeline_impl->lock, flags);
        return 1;
    }

    ns = TP_TO_nSEC(utp.estimate);
    timeline_impl->nsec += (ns - timeline_impl->last)
        + div_s64(timeline_impl->mult * (ns - timeline_impl->last),1000000000ULL);
    timeline_impl->last = ns;
    *tp = ns_to_timespec(timeline_impl->nsec);
    spin_unlock_irqrestore(&timeline_impl->lock, flags);
    return 0;
}

static int qot_timeline_chdev_adjtime(struct posix_clock *pc, struct timex *tx)
{
    timeline_impl_t *timeline_impl = container_of(pc,timeline_impl_t,clock);
    int err = -EOPNOTSUPP;
    if (tx->modes & ADJ_SETOFFSET) {
        struct timespec ts;
        ktime_t kt;
        s64 delta;
        ts.tv_sec = tx->time.tv_sec;
        ts.tv_nsec = tx->time.tv_usec;
        if (!(tx->modes & ADJ_NANO))
            ts.tv_nsec *= 1000;
        if ((unsigned long) ts.tv_nsec >= NSEC_PER_SEC)
            return -EINVAL;
        kt = timespec_to_ktime(ts);
        delta = ktime_to_ns(kt);
        err = qot_timeline_chdev_adj_adjtime(pc, delta);
    } else if (tx->modes & ADJ_FREQUENCY) {
        s32 ppb = qot_timeline_chdev_ppm_to_ppb(tx->freq);
        //if (ppb > timeline_impl->max_adj || ppb < -timeline_impl->max_adj)
            //return -ERANGE;
        err = qot_timeline_chdev_adj_adjfreq(pc, ppb);
        timeline_impl->dialed_frequency = tx->freq;
    } else if (tx->modes == 0) {
        tx->freq = timeline_impl->dialed_frequency;
        err = 0;
    }
    return err;
}

static int qot_timeline_chdev_open(struct posix_clock *pc, fmode_t fmode)
{
    /* TODO: Privately allocated data */
    pr_info("qot_timeline_chdev: Open IOCTL for timeline\n");
    return 0;
}

static int qot_timeline_chdev_release(struct posix_clock *pc)
{
    /* TODO: Privately allocated data */
    return 0;
}

static long qot_timeline_chdev_ioctl(struct posix_clock *pc, unsigned int cmd, unsigned long arg)
{    
    qot_bounds_t bounds;
    utimepoint_t utp;
    stimepoint_t stp;
    qot_binding_t msgb;
    timepoint_t tp;
    s64 coretime;
    s64 timelinetime;
    s64 u_timelinetime;
    s64 l_timelinetime;
    s64 loctime;
    binding_impl_t *binding_impl = NULL;
    timeline_impl_t *timeline_impl = container_of(pc,timeline_impl_t,clock);

    utimepoint_t wait_until_time;
    int wait_until_retval;

    if (!timeline_impl) {
        pr_err("qot_timeline_chdev: cannot find timeline\n");
        return -EACCES;
    }
    switch (cmd) {
    /* Get information about this timeline */
    case TIMELINE_GET_INFO:
        if (copy_to_user((qot_timeline_t*)arg, timeline_impl->info, sizeof(qot_timeline_t)))
            return -EACCES;
        
        break;
    /* Get information about this timeline's requirements */
    case TIMELINE_GET_BINDING_INFO:
        // find the binding with minimum resolution (need to satisfy tightest req.)
        binding_impl = list_entry((&timeline_impl->head_res)->next, binding_impl_t, list_res);
        msgb.demand.resolution = binding_impl->info.demand.resolution;
        if (!binding_impl)
            return -EACCES;

        // find the binding with minimum lower accuracy (need to satisfy tightest req.)
        binding_impl = list_entry((&timeline_impl->head_low)->next, binding_impl_t, list_low);
        msgb.demand.accuracy.below = binding_impl->info.demand.accuracy.below;
        if (!binding_impl)
            return -EACCES;

        // find the binding with minimum upper accuracy (need to satisfy tightest req.)
        binding_impl = list_entry((&timeline_impl->head_upp)->next, binding_impl_t, list_upp);
        msgb.demand.accuracy.above = binding_impl->info.demand.accuracy.above;
        if (!binding_impl)
            return -EACCES;

        /* Copy the binding back to the user */
        if (copy_to_user((qot_binding_t*)arg, &msgb, sizeof(qot_binding_t)))
            return -EACCES;

        break;
    /* Bind to this timeline */
    case TIMELINE_BIND_JOIN:
        pr_info("qot_timeline_chdev: Binding to timeline\n");
        if (copy_from_user(&msgb, (qot_binding_t*)arg, sizeof(qot_binding_t)))
        {
            pr_err("qot_timeline_chdev: error in copy from user\n");
            return -EACCES;
        }
        binding_impl = qot_binding_add(timeline_impl, &msgb);
        if (!binding_impl)
        {
            pr_info("qot_timeline_chdev: Could not bind to timeline\n");
            return -EACCES;
        }
        pr_info("qot_timeline_chdev: Bound to timeline\n");
        /* Copy the ID back to the user */
        if (copy_to_user((qot_binding_t*)arg, &binding_impl->info, sizeof(qot_binding_t)))
            return -EACCES;
        break;
    /* Unbind from this timeline */
    case TIMELINE_BIND_LEAVE:
        if (copy_from_user(&msgb, (qot_binding_t*)arg, sizeof(qot_binding_t)))
            return -EACCES;
        /* Try and find the binding associated with this id */
        //binding_impl = idr_find(&timeline_impl->idr_bindings, msgb.id);
        binding_impl = find_binding(&timeline_impl->root, current->pid);
        if (!binding_impl)
            return -EACCES;
        pr_info("qot_timeline_chdev: Unbinding from timeline\n");
        qot_binding_del(binding_impl);
        break;
    /* Update binding parameters */
    case TIMELINE_BIND_UPDATE:
        if (copy_from_user(&msgb, (qot_binding_t*)arg, sizeof(qot_binding_t)))
            return -EACCES;
        /* Try and find the binding associated with this id */
        //binding_impl = idr_find(&timeline_impl->idr_bindings, msgb.id);
        binding_impl = find_binding(&timeline_impl->root, current->pid);

        if (!binding_impl)
            return -EACCES;
        pr_info("qot_timeline_chdev: Update the binding\n");

        /* Copy over the new data */
        memcpy(&binding_impl->info, &msgb, sizeof(qot_binding_t));
        /* Delete and add to resort list */
        list_del(&binding_impl->list_res);
        list_del(&binding_impl->list_low);
        list_del(&binding_impl->list_upp);
        qot_insert_list_res(binding_impl, &timeline_impl->head_res);
        qot_insert_list_low(binding_impl, &timeline_impl->head_low);
        qot_insert_list_upp(binding_impl, &timeline_impl->head_upp);
        break;
    /* Setting the upper and lower bound on timeline's drift */
    case TIMELINE_SET_SYNC_UNCERTAINTY:
        if (copy_from_user(&bounds, (qot_bounds_t*)arg, sizeof(qot_bounds_t)))
            return -EACCES;
        timeline_impl->u_mult = (s32) bounds.u_drift;
        timeline_impl->l_mult = (s32) bounds.l_drift;
        timeline_impl->m_nsec = (s64) bounds.m_nsec;
        break;
    /* Convert a core time to a timeline */
    case TIMELINE_CORE_TO_REMOTE:
        if (copy_from_user(&stp, (stimepoint_t*)arg, sizeof(stimepoint_t)))
            return -EACCES;

        // convert from core time to timeline reference of time
        coretime = TP_TO_nSEC(stp.estimate);
        timelinetime = coretime;
        qot_loc2rem(timeline_impl->index, 0, &timelinetime);
        TP_FROM_nSEC(stp.estimate, timelinetime);

        // find upper bound on core time
        if(timeline_impl->mult > 0){
            u_timelinetime = timeline_impl->nsec + (coretime - timeline_impl->last) + div_s64(timeline_impl->u_mult * (coretime - timeline_impl->last),1000000000ULL)+timeline_impl->m_nsec;
            l_timelinetime = timeline_impl->nsec + (coretime - timeline_impl->last) + div_s64(timeline_impl->l_mult * (coretime - timeline_impl->last),1000000000ULL)-(7*timeline_impl->m_nsec);
        }else{
            u_timelinetime = timeline_impl->nsec + (coretime - timeline_impl->last) + div_s64(timeline_impl->u_mult * (coretime - timeline_impl->last),1000000000ULL)+(7*timeline_impl->m_nsec);
            l_timelinetime = timeline_impl->nsec + (coretime - timeline_impl->last) + div_s64(timeline_impl->l_mult * (coretime - timeline_impl->last),1000000000ULL)-timeline_impl->m_nsec;
        }
/*      if(u_timelinetime > timelinetime)
            TL_FROM_nSEC(utp.interval.above, u_timelinetime - timelinetime);
        else
            TL_FROM_nSEC(utp.interval.above, 0);
            //TL_FROM_nSEC(utp.interval.above, timelinetime - u_timelinetime);

        // find lower bound on core time
        //l_timelinetime = timeline_impl->nsec + (coretime - timeline_impl->last) + div_s64(timeline_impl->l_mult * (coretime - timeline_impl->last),1000000000ULL)+timeline_impl->m_nsec;
        if(timelinetime > l_timelinetime)
            TL_FROM_nSEC(utp.interval.below, timelinetime - l_timelinetime);
        else
            TL_FROM_nSEC(utp.interval.below, 0);
            //TL_FROM_nSEC(utp.interval.below, l_timelinetime - timelinetime);
*/
        TP_FROM_nSEC(stp.u_estimate, u_timelinetime);
        TP_FROM_nSEC(stp.l_estimate, l_timelinetime);

        if (copy_to_user((stimepoint_t*)arg, &stp, sizeof(stimepoint_t)))
            return -EACCES;
        break;
    /* Convert timeline time to core time*/
    case TIMELINE_REMOTE_TO_CORE:
        if (copy_from_user(&tp, (timepoint_t*)arg, sizeof(timepoint_t)))
            return -EACCES;

        // convert from timeline reference to core of time
        loctime = TP_TO_nSEC(tp);
        qot_rem2loc(timeline_impl->index, 0, &loctime);
        TP_FROM_nSEC(tp, loctime);

        if (copy_to_user((timepoint_t*)arg, &tp, sizeof(timepoint_t)))
            return -EACCES;
        break;
    /* Get the current core time */
    case TIMELINE_GET_CORE_TIME_NOW:
        if (qot_clock_get_core_time(&utp))
    		return -EACCES;
        // TODO: Latency estimates are not being added for now...
        /* Add the latency due to the OS query */
        //qot_admin_add_latency(&utp);
        /* Add the latency due to the CORE clock */
        //qot_tsync_add_latency(&msgu.interval);
        /* Add the latency due to the CORE clock */
        if (copy_to_user((utimepoint_t*)arg, &utp, sizeof(utimepoint_t)))
            return -EACCES;
        break;
    /* Get the current timeline time */
    case TIMELINE_GET_TIME_NOW:
        if (qot_clock_get_core_time(&utp))
            return -EACCES;
        // convert from core time to timeline reference of time
        coretime = TP_TO_nSEC(utp.estimate);
        qot_loc2rem(timeline_impl->index, 0, &coretime);
        TP_FROM_nSEC(utp.estimate, coretime);
        // TODO: Latency estimates are not being added for now...
        /* Add the latency due to the OS query */
        //qot_admin_add_latency(&utp);
        /* Add the latency due to the CORE clock */
        //qot_tsync_add_latency(&msgu.interval);
        /* Add the latency due to the CORE clock */
        if (copy_to_user((utimepoint_t*)arg, &utp, sizeof(utimepoint_t)))
            return -EACCES;
        break;
    default:
        return -EINVAL;
    }
    return 0;
}

static unsigned int qot_timeline_chdev_poll(struct posix_clock *pc, struct file *fp,
    poll_table *wait)
{
    return 0;
}

static ssize_t qot_timeline_chdev_read(struct posix_clock *pc, uint rdflags,
    char __user *buf, size_t cnt)
{
    return 0;
}

/* File operations for a given timeline */
static struct posix_clock_operations qot_timeline_chdev_ops = {
    .owner          = THIS_MODULE,
    .clock_adjtime  = qot_timeline_chdev_adjtime,
    .clock_gettime  = qot_timeline_chdev_gettime,
    .clock_getres   = qot_timeline_chdev_getres,
    .clock_settime  = qot_timeline_chdev_settime,
    .ioctl          = qot_timeline_chdev_ioctl,
    .open           = qot_timeline_chdev_open,
    .release        = qot_timeline_chdev_release,
    .poll           = qot_timeline_chdev_poll,
    .read           = qot_timeline_chdev_read,
};

static void qot_timeline_chdev_delete(struct posix_clock *pc)
{
    struct list_head *pos, *q;
    binding_impl_t *binding_impl;
    timeline_impl_t *timeline_impl = container_of(pc, timeline_impl_t, clock);
    /* Remove all attached timeline_impl */
    if (!list_empty(&timeline_impl->head_res)) {
        list_for_each_safe(pos, q, &timeline_impl->head_res){
            binding_impl = list_entry(pos, binding_impl_t, list_res);
            qot_binding_del(binding_impl);
        }
    }
    /* Remove the timeline_impl */
    idr_remove(&qot_timelines_map, timeline_impl->index);
    kfree(timeline_impl);
}

/* PUBLIC */

/* Create the timeline and return an index that identifies it uniquely */
int qot_timeline_chdev_register(qot_timeline_t *info)
{
    timeline_impl_t *timeline_impl;
    int major;
    if (!info)
        goto fail_noinfo;
    /* Allocate a major number for device */
    major = MAJOR(timeline_devt);
    /* Allocate some memory for the timeline and copy name */
    timeline_impl = kzalloc(sizeof(timeline_impl_t), GFP_KERNEL);
    if (!timeline_impl) {
        pr_err("qot_timeline: cannot allocate memory for timeline_impl");
        goto fail_memoryalloc;
    }
    timeline_impl->info = info;
    //memcpy(&timeline_impl->info,info,sizeof(qot_timeline_t));
    /* Draw the next integer X for /dev/timelineX */
    idr_preload(GFP_KERNEL);
    spin_lock(&qot_timelines_lock);
    timeline_impl->index = idr_alloc(&qot_timelines_map,
        (void*) timeline_impl, 0, 0, GFP_NOWAIT);
    spin_unlock(&qot_timelines_lock);
    idr_preload_end();
    if (timeline_impl->index < 0) {
        pr_err("qot_timeline: cannot get next integer");
        goto fail_idasimpleget;
    }
    /* Assign an ID and create the character device for this timeline */
    timeline_impl->devid = MKDEV(major, timeline_impl->index);
    timeline_impl->dev = device_create(timeline_class, NULL,
        timeline_impl->devid, timeline_impl, "timeline%d", timeline_impl->index);
    /* Setup the PTP clock for this timeline */
    timeline_impl->clock.ops = qot_timeline_chdev_ops;
    timeline_impl->clock.release = qot_timeline_chdev_delete;
    if (posix_clock_register(&timeline_impl->clock, timeline_impl->devid)) {
        pr_err("qot_timeline: cannot register POSIX clock");
        goto fail_posixclock;
    }
    /* Create the sysfs interface into the timeline */
	if (qot_timeline_sysfs_init(timeline_impl->dev)) {
		pr_err("qot_timeline: cannot register sysfs interface");
		goto fail_sysfs;
	}
    
    /* added by Fatima START */
    // INITIALIZE SYNC PARAMETERS
    timeline_impl->mult = 0;
    timeline_impl->last = 0;
    timeline_impl->nsec = 0;
    timeline_impl->m_nsec = 0;
    timeline_impl->u_mult = 0;
    timeline_impl->l_mult = 0;
    timeline_impl->dialed_frequency = 0;
    timeline_impl->max_adj = 1000000;
    /* added by Fatima END */

    timeline_impl->info->index = timeline_impl->index;

	/* Initialize our list heads  to track binding order */
	INIT_LIST_HEAD(&timeline_impl->head_res);
	INIT_LIST_HEAD(&timeline_impl->head_low);
	INIT_LIST_HEAD(&timeline_impl->head_upp);
    /* Update the index before returning OK */
    pr_info("qot_timeline_chdev: Timeline %d chdev created name is %s\n", info->index, info->name);

    return timeline_impl->index;

fail_sysfs:
    posix_clock_unregister(&timeline_impl->clock);
fail_posixclock:
    idr_remove(&qot_timelines_map, timeline_impl->index);
fail_idasimpleget:
    kfree(timeline_impl);
fail_memoryalloc:
fail_noinfo:
    return -1;  /* Negative IDR values are not valid */
}

/* Destroy the timeline based on an index */
qot_return_t qot_timeline_chdev_unregister(int index, bool admin_flag)
{
    timeline_impl_t *timeline_impl = idr_find(&qot_timelines_map, index);
    if (!timeline_impl)
        return QOT_RETURN_TYPE_ERR;
    /* If the Admin Flag is not set Check if no binding is there */
    if(rb_first(&timeline_impl->root) != NULL && admin_flag == 0)
    {    
        return QOT_RETURN_TYPE_ERR;
    }
    /* Clean up the sysfs interface */
    qot_timeline_sysfs_cleanup(timeline_impl->dev);
    /* Delete the character device - also deletes IDR */
    device_destroy(timeline_class,timeline_impl->devid);
    /* Remove the posix clock */
    posix_clock_unregister(&timeline_impl->clock);
    return QOT_RETURN_TYPE_OK;
}

/* Cleanup the timeline system */
void qot_timeline_chdev_cleanup(struct class *qot_class)
{
    /* Remove the character device region */
    unregister_chrdev_region(timeline_devt, MINORMASK + 1);
    /* Shut down the IDR subsystem */
    idr_destroy(&qot_timelines_map);
}

/* Initialize the timeline system */
qot_return_t qot_timeline_chdev_init(struct class *qot_class)
{
    int err;
    /* Copy the device class to help with future chdev allocations */
    if (!qot_class)
        return QOT_RETURN_TYPE_ERR;
    timeline_class = qot_class;
    /* Try and allocate a character device region to hold all /dev/timelines */
    err = alloc_chrdev_region(&timeline_devt, 0, MINORMASK + 1, DEVICE_NAME);
    if (err < 0) {
        pr_err("ptp: failed to allocate device region\n");
    }
    spin_lock_init(&qot_timelines_lock);
    return QOT_RETURN_TYPE_OK;
}

MODULE_LICENSE("GPL");

