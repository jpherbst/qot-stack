/*
 * @file qot_core.h
 * @brief Interfaces used by clocks to interact with core
 * @author Andrew Symington
 *
 * Copyright (c) Regents of the University of California, 2015.
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

#include "qot_clock.h"
#include "qot_timeline.h"

#define DEVICE_NAME "timeline"

/* PRIVATE */

static dev_t timeline_devt;
static struct class *timeline_class;

/* A map of all timelines in the system */
static DEFINE_IDR(qot_timelines_map);

static spinlock_t qot_timelines_lock;

/* Private data for a timeline, not visible outside this code      */
typedef struct timeline_impl {
    qot_timeline_t info;        /* Timeline info                   */
    int index;                  /* IDR index for timeline          */
    dev_t devid;                /* Device ID                       */
    struct device *dev;         /* Device                          */
    struct posix_clock clock;   /* POSIX clock for this timeline   */
    s32 dialed_frequency;       /* Discipline: dialed frequency    */
    s64 last;                   /* Discipline: last cycle count of */
    s64 mult;                   /* Discipline: ppb multiplier      */
    s64 nsec;                   /* Discipline: global time offset  */
    spinlock_t lock;            /* Protects driver time registers  */
    struct idr idr_bindings;    /* IDR for trackng bindings        */
    struct list_head head_res;  /* Head pointing to resolution     */
    struct list_head head_low;  /* Head pointing to accuracy (low) */
    struct list_head head_upp;  /* Head pointing to accuracy (upp) */
} timeline_impl_t;

/* Private data for a binding, not visible outside this code       */
typedef struct binding_impl {
    qot_binding_t info;         /* Binding information             */
    timeline_impl_t *parent;    /* Parent timeline                 */
    struct list_head list_res;  /* Head pointing to resolution     */
    struct list_head list_low;  /* Head pointing to accuracy (low) */
    struct list_head list_upp;  /* Head pointing to accuracy (upp) */
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
    /* Allocate an ID for this binding */
    idr_preload(GFP_KERNEL);
    spin_lock_irqsave(&timeline_impl->lock, flags);
    binding_impl->info.id = idr_alloc(&timeline_impl->idr_bindings,
        (void*) binding_impl,0,0,GFP_NOWAIT);
    spin_unlock_irqrestore(&timeline_impl->lock, flags);
    idr_preload_end();
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
    /* Remove the binding from the parallel lists */
    if (!binding_impl)
        return;
    list_del(&binding_impl->list_res);
    list_del(&binding_impl->list_low);
    list_del(&binding_impl->list_upp);
    /* Remove the IDR reservation */
    if (binding_impl->parent)
        idr_remove(&binding_impl->parent->idr_bindings,binding_impl->info.id);
    /* Free the memory */
    kfree(binding_impl);
}

/* Dicipline operations */

static int qot_timeline_chdev_adjfreq(struct posix_clock *pc, s32 ppb)
{
    s64 ns;
    unsigned long flags;
    timeline_impl_t *timeline = container_of(pc,timeline_impl_t,clock);
    spin_lock_irqsave(&timeline->lock, flags);
    ns = qot_clock_get_core_time();
    timeline->nsec += (ns - timeline->last)
        + div_s64(timeline->mult * (ns - timeline->last),1000000000ULL);
    timeline->mult += ppb;
    timeline->last  = ns;
    spin_unlock_irqrestore(&timeline->lock, flags);
    return 0;
}

static int qot_timeline_chdev_adjtime(struct posix_clock *pc, s64 delta)
{
    s64 ns;
    unsigned long flags;
    timeline_impl_t *timeline = container_of(pc,timeline_impl_t,clock);
    spin_lock_irqsave(&timeline->lock, flags);
    ns = qot_core_time();
    timeline->nsec += (ns - timeline->last)
        + div_s64(timeline->mult * (ns - timeline->last),1000000000ULL) + delta;
    timeline->last = ns;
    spin_unlock_irqrestore(&timeline->lock, flags);
    return 0;
}

static s32 qot_timeline_chdev_ppm_to_ppb(long ppm)
{
    s64 ppb = 1 + ppm;
    ppb *= 125;
    ppb >>= 13;
    return (s32) ppb;
}

/* File operations operations */

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
    unsigned long flags;
    timeline_impl_t *timeline = container_of(pc,timeline_impl_t,clock);
    spin_lock_irqsave(&timeline->lock, flags);
    timeline->last = qot_core_time();
    timeline->nsec = timespec64_to_ns(ts);
    spin_unlock_irqrestore(&timeline->lock, flags);
    return 0;
}

static int qot_timeline_chdev_gettime(struct posix_clock *pc,
    struct timespec *tp)
{
    s64 ns;
    unsigned long flags;
    timeline_impl_t *timeline = container_of(pc,timeline_impl_t,clock);
    spin_lock_irqsave(&timeline->lock, flags);
    ns = qot_core_time();
    timeline->nsec += (ns - timeline->last)
        + div_s64(timeline->mult * (ns - timeline->last),1000000000ULL);
    timeline->last = ns;
    *ts = ns_to_timespec64(timeline->nsec);
    spin_unlock_irqrestore(&timeline->lock, flags);
    return 0;
}

static int qot_timeline_chdev_adjtime(struct posix_clock *pc, struct timex *tx)
{
    timeline_impl_t *timeline = container_of(pc,timeline_impl_t,clock);
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
        err = qot_timeline_chdev_adjtime(pc, delta);
    } else if (tx->modes & ADJ_FREQUENCY) {
        s32 ppb = qot_timeline_chdev_ppm_to_ppb(tx->freq);
        if (ppb > ops->max_adj || ppb < -ops->max_adj)
            return -ERANGE;
        err = qot_timeline_chdev_adjfreq(pc, ppb);
        timeline->dialed_frequency = tx->freq;
    } else if (tx->modes == 0) {
        tx->freq = timeline->dialed_frequency;
        err = 0;
    }
    return err;
}

static int qot_timeline_chdev_open(struct posix_clock *pc, fmode_t fmode)
{
    /* TODO: Privately allocated data */
	return 0;
}

static int qot_timeline_chdev_release(struct posix_clock *pc)
{
    /* TODO: Privately allocated data */
    return 0;
}

static long qot_timeline_chdev_ioctl(struct posix_clock *pc, unsigned int cmd,
    unsigned long arg)
{
    struct timespec tp;
    qot_timeline_t msgt;
    qot_binding_t msgb;
    timepoint_t msgp;
    utimepoint_t msgu;
    binding_impl_t *binding_impl = NULL;
    timeline_impl_t *timeline_impl = container_of(pc,timeline_impl_t,clock);
    if (!timeline_impl) {
        pr_err("qot_timeline_chdev: cannot find timeline\n");
        return -EACCES;
    }
    switch (cmd) {
    /* Get information about this timeline */
    case TIMELINE_GET_INFO:
        if (copy_to_user((qot_timeline_t*)arg, &timeline_impl->info, sizeof(qot_timeline_t)))
            return -EACCES;
        break;
    /* Bind to this timeline */
    case TIMELINE_BIND_JOIN:
        if (copy_from_user(&msgb, (qot_binding_t*)arg, sizeof(qot_binding_t)))
            return -EACCES;
        binding_impl = qot_binding_add(timeline_impl, &msgb);
        if (!binding_impl)
            return -EACCES;
        /* Copy the ID back to the user */
        if (copy_to_user((qot_binding_t*)arg, &binding_impl->info, sizeof(qot_binding_t)))
            return -EACCES;
        break;
    /* Unbind from this timeline */
    case TIMELINE_BIND_LEAVE:
        if (copy_from_user(&msgb, (qot_binding_t*)arg, sizeof(qot_binding_t)))
            return -EACCES;
        /* Try and find the binding associated with this id */
        binding_impl = idr_find(&timeline_impl->idr_bindings, msgb.id);
        if (!binding_impl)
            return -EACCES;
        qot_binding_del(binding_impl);
        break;
    /* Update binding parameters */
    case TIMELINE_BIND_UPDATE:
        if (copy_from_user(&msgb, (qot_binding_t*)arg, sizeof(qot_binding_t)))
            return -EACCES;
        /* Try and find the binding associated with this id */
        binding_impl = idr_find(&timeline_impl->idr_bindings, msgb.id);
        if (!binding_impl)
            return -EACCES;
        /* Copy over the new data */
        memcpy(&binding_impl->info, &msgb, sizeof(qot_binding_t));
        /* Delete and add to resort list */
        list_del(&binding_impl->list_res);
        list_del(&binding_impl->list_low);
        list_del(&binding_impl->list_upp);
        qot_insert_list_acc(binding_impl, &timeline_impl->head_res);
        qot_insert_list_res(binding_impl, &timeline_impl->head_low);
        qot_insert_list_res(binding_impl, &timeline_impl->head_upp);
        break;
    /* Convert a core time to a timeline */
    case TIMELINE_CORE_TO_REMOTE:
        break;
    /* Convert a core time to a timeline */
    case TIMELINE_REMOTE_TO_CORE:
        break;
    /* Get the current time */
    case TIMELINE_GET_TIME_NOW:
        /* Get the current time */
        if (qot_timeline_chdev_gettime(pc,&tp));
            return -EACCES;

        /* Convert the time estimate to an uncertain timepoint */
        utimepoint_from_timespec(&msgu,&tp);

        /* Add the time error introduced by the clock query latency */
        utimepoint_add(&msgu,core_get_latency());
        /* Add the time error introduced by synchronization */
        utimepoint_add(&msgu,qot_timeline_sync_error());
        /* Copy the ID back to the user */
        if (copy_to_user((utimepoint_t*)arg, &msgu, sizeof(utimepoint_t)))
            return -EACCES;
        break;
    /* Blocking sleep until a given point in time */
    case TIMELINE_SLEEP_UNTIL:
        break;
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
	.owner		    = THIS_MODULE,
	.clock_adjtime	= qot_timeline_chdev_adjtime,
	.clock_gettime	= qot_timeline_chdev_gettime,
	.clock_getres	= qot_timeline_chdev_getres,
	.clock_settime	= qot_timeline_chdev_settime,
	.ioctl		    = qot_timeline_chdev_ioctl,
	.open		    = qot_timeline_chdev_open,
    .release        = qot_timeline_chdev_release,
	.poll		    = qot_timeline_chdev_poll,
	.read		    = qot_timeline_chdev_read,
};

static void qot_timeline_chdev_delete(struct posix_clock *pc)
{
    struct list_head *pos, *q;
    binding_impl_t *binding_impl;
    timeline_impl_t *timeline_impl = container_of(pc, timeline_impl_t, clock);
    /* Remove all attached timeline_impl */
    list_for_each_safe(pos, q, &timeline_impl.head_res){
        binding_impl = list_entry(pos, binding_impl_t, res_list);
        qot_binding_del(binding_impl);
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
    if (!name)
        goto fail_noname;
    /* Allocate a major number for device */
    major = MAJOR(timeline_devt);
    /* Allocate some memory for the timeline and copy name */
    timeline_impl = kzalloc(sizeof(timeline_impl_t), GFP_KERNEL);
    if (!timeline_impl) {
        pr_err("qot_timeline: cannot allocate memory for timeline_impl");
        goto fail_memoryalloc;
    }
    memcpy(timeline_impl->info,info,sizeof(qot_timeline_t));
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
    /* Update the index before returning OK */
    return timeline_impl->index;

fail_posixclock:
    idr_remove(&qot_timelines_map, timeline_impl->index);
fail_idasimpleget:
    kfree(timeline_impl);
fail_memoryalloc:
fail_noname:
    return -1;  /* Negative IDR values are not valid */
}

/* Destroy the timeline based on an index */
qot_return_t qot_timeline_chdev_unregister(int index)
{
    timeline_impl_t *timeline_impl = idr_find(&qot_timelines_map, index);
    if (!timeline_impl)
        return QOT_RETURN_TYPE_ERR;
    /* Remove the posix clock */
    posix_clock_unregister(&timeline_impl->clock);
    /* Delete the character device - also deletes IDR */
    device_destroy(timeline_class, timeline_impl->devid);
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

