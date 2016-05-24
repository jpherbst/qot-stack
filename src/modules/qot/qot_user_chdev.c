/*
 * @file qot_user_chdev.c
 * @brief User ioctl interface (/dev/qotusr) to the QoT core
 * @author Andrew Symington and Sandeep D'souza
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

#include <linux/module.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/rbtree.h>
#include <linux/sched.h>

#include "qot_user.h"
#include "qot_timeline.h"
#include "qot_scheduler.h"
#include "qot_clock.h"

#define DEVICE_NAME "qotusr"

/* Internal event type */
typedef struct event {
    qot_event_t info;                   /* The event type          */
    struct list_head list;              /* The list of events head */
} event_t;

/* Information that allows us to maintain parallel cchardev connections */
typedef struct qot_user_chdev_con {
    struct rb_node node;                /* Red-black tree node  */
    struct file *fileobject;            /* File object          */
    wait_queue_head_t wq;               /* Wait queue           */
    int event_flag;                     /* Data ready flag      */
    struct list_head event_list;        /* Event list           */
    struct semaphore list_sem;           /* Event list semaphore */
} qot_user_chdev_con_t;

/* Information required to open a character device */
static struct device *qot_device = NULL;
static int qot_major;

/* Root of the red-black tree used to store parallel connections */
static struct rb_root qot_user_chdev_con_root = RB_ROOT;

// TIME PROJECTION FOR PERIODIC OUT REPROGRAMMING ///////////////////////////////////////
static s64 qot_perout_convert(qot_perout_t *perout)
{
    s64 period = TL_TO_nSEC(perout->period);
    qot_rem2loc(perout->timeline.index, 1, &period);
    return period;
}

/* Free memory used by a connection */
static void qot_user_chdev_con_free(qot_user_chdev_con_t *con)
{
    event_t *event;
    struct list_head *item, *tmp;
    list_for_each_safe(item, tmp, &con->event_list) {
        event = list_entry(item, event_t, list);    /* Get the event  */
        list_del(item);                             /* Delte the item */
        kfree(event);                               /* Free up memory */
    }
}

/* Search for the connection corresponding to a given fileobject */
static qot_user_chdev_con_t *qot_user_chdev_con_search(struct file *f)
{
    int result;
    qot_user_chdev_con_t *con;
    struct rb_node *node = qot_user_chdev_con_root.rb_node;
    while (node) {
        con = container_of(node, qot_user_chdev_con_t, node);
        result = f - con->fileobject;
        if (result < 0)
            node = node->rb_left;
        else if (result > 0)
            node = node->rb_right;
        else
            return con;
    }
    return NULL;
}

/* Insert a new connection into the data structure (0 = OK, 1 = EXISTS) */
static void qot_user_chdev_con_insert(qot_user_chdev_con_t *con)
{
    int result;
    qot_user_chdev_con_t *tmp;
    struct rb_node **new = &(qot_user_chdev_con_root.rb_node), *parent = NULL;
    while (*new) {
        tmp = container_of(*new, qot_user_chdev_con_t, node);
        result = con->fileobject - tmp->fileobject;
        parent = *new;
        if (result < 0)                 /* Traverse left    */
            new = &((*new)->rb_left);
        else if (result > 0)            /* Traverse right   */
            new = &((*new)->rb_right);
        else {                          /* Special case: already exists */
            rb_replace_node(*new, &con->node, &qot_user_chdev_con_root);
            qot_user_chdev_con_free(tmp);
            return;
        }
    }
    rb_link_node(&con->node, parent, new);
    rb_insert_color(&con->node, &qot_user_chdev_con_root);
}

/* Remove a connection from the red-black tree */
static int qot_user_chdev_con_remove(qot_user_chdev_con_t *con)
{
    if (!con) {
        pr_err("qot_user_chdev: could not find ioctl connection\n");
        return 1;
    }
    rb_erase(&con->node, &qot_user_chdev_con_root);
    return 0;
}

/* Notify all connections about a newly created timeline */
static int qot_user_timeline_create_notify(qot_timeline_t *timeline)
{
    struct rb_node *con_node = NULL;
    struct qot_user_chdev_con *con;
    event_t *event;
    con_node = rb_first(&qot_user_chdev_con_root);
    while(con_node != NULL)
    {
        con = container_of(con_node, struct qot_user_chdev_con, node);   
        event = kzalloc(sizeof(event_t), GFP_KERNEL);
        if (!event) {
            pr_warn("qot_user_chdev: failed to allocate event\n");
            continue;
        }
        event->info.type = QOT_EVENT_TIMELINE_CREATE;
        strncpy(event->info.data,timeline->name, QOT_MAX_NAMELEN);
        if(down_interruptible(&con->list_sem))
            return -ERESTARTSYS;
        list_add_tail(&event->list, &con->event_list);
        con->event_flag = 1;
        up(&con->list_sem);
        wake_up_interruptible(&con->wq);
        con_node = rb_next(con_node);
    }
    return 0;
}

/* chardev ioctl open callback implementation */
static int qot_user_chdev_ioctl_open(struct inode *i, struct file *f)
{
    qot_timeline_t *timeline;
    event_t *event;
    unsigned long flags;
    /* Create a new connection */
    qot_user_chdev_con_t *con =
        kzalloc(sizeof(qot_user_chdev_con_t), GFP_KERNEL);
    if (!con) {
        pr_err("qot_user_chdev: failed to allocate memory for connection\n");
        return -ENOMEM;
    }
    INIT_LIST_HEAD(&con->event_list);
    init_waitqueue_head(&con->wq);
    con->fileobject = f;
    con->event_flag = 0;
    sema_init(&con->list_sem, 1);

    /* Insert the connection into the red-black tree */
    qot_user_chdev_con_insert(con);

    /* Notify the connection (by polling) of all existing timelines */
    timeline = NULL;
    if (qot_timeline_first(&timeline)==QOT_RETURN_TYPE_OK) {
        raw_spin_lock_irqsave(&qot_timeline_lock, flags);
        do {
            event = kzalloc(sizeof(event_t), GFP_KERNEL);
            if (!event) {
                pr_warn("qot_user_chdev: failed to allocate event\n");
                continue;
            }
            event->info.type = QOT_EVENT_TIMELINE_CREATE;
            strncpy(event->info.data,timeline->name,QOT_MAX_NAMELEN);
            if(down_interruptible(&con->list_sem))
                return -ERESTARTSYS;
            list_add_tail(&event->list, &con->event_list);
            up(&con->list_sem);
        } while (qot_timeline_next(&timeline)==QOT_RETURN_TYPE_OK);
        raw_spin_unlock_irqrestore(&qot_timeline_lock, flags);
        con->event_flag = 1;
        wake_up_interruptible(&con->wq);
    }
    pr_info("qot_user_chdev_ioctl_open: /dev/qotusr file opened\n");
    return 0;
}

/* chardev ioctl close callback implementation */
static int qot_user_chdev_ioctl_close(struct inode *i, struct file *f)
{
    qot_user_chdev_con_t *con = qot_user_chdev_con_search(f);
    if (!con) {
        pr_err("qot_user_chdev: could not find ioctl connection\n");
        return -ENOMEM;
    }
    qot_user_chdev_con_remove(con);
    qot_user_chdev_con_free(con);
    return 0;
}

/* chardev ioctl open access implementation */
static long qot_user_chdev_ioctl_access(struct file *f, unsigned int cmd,
    unsigned long arg)
{
    int retval;
    event_t *event;
    qot_event_t msge;
    qot_timeline_t msgt;
    qot_timeline_t *timeline = NULL;

    qot_sleeper_t sleeper;
    u64 coretime;
    int wait_until_retval;

    qot_perout_t perout;
    timepoint_t core_start;
    timepoint_t core_period;
    s64 period, start;

    qot_user_chdev_con_t *con = qot_user_chdev_con_search(f);
    if (!con)
        return -EACCES;
 
    switch (cmd) {
    /* Get the next event in the queue for this connection */
    case QOTUSR_GET_NEXT_EVENT:
        if (list_empty(&con->event_list)) {
            con->event_flag = 0;
            return -EACCES;
        }
        event = list_entry(con->event_list.next, event_t, list);
        memcpy(&msge,&event->info,sizeof(qot_event_t));
        list_del(&event->list);
        kfree(event);
        if (copy_to_user((qot_event_t*)arg, &msge, sizeof(qot_event_t)))
            return -EACCES;
        break;
    /* Get information about a timeline */
    case QOTUSR_GET_TIMELINE_INFO:
        if (copy_from_user(&msgt, (qot_timeline_t*)arg, sizeof(qot_timeline_t)))
            return -EACCES;
        timeline = &msgt;
        if (qot_timeline_get_info(&timeline))
            return -EACCES;
        msgt = *timeline;
        if (copy_to_user((qot_timeline_t*)arg, &msgt, sizeof(qot_timeline_t)))
            return -EACCES;
        break;
    /* Create a timeline */
    case QOTUSR_CREATE_TIMELINE:
        if (copy_from_user(&msgt, (qot_timeline_t*)arg, sizeof(qot_timeline_t)))
            return -EACCES;
        retval = qot_timeline_create(&msgt);
        // Check if the timeline already exists or some other problem in creating it */
        if (retval== -QOT_RETURN_TYPE_ERR)
            return QOT_RETURN_TYPE_ERR;     // Timeline Exists 
        else if (retval)
            return -EACCES;
        // Notify all connections of the newly created "local" timeline
        qot_user_timeline_create_notify(&msgt);
        pr_info("qot_usr_chdev: Timeline %d created name is %s\n", msgt.index, msgt.name);
        if (copy_to_user((qot_timeline_t*)arg, &msgt, sizeof(qot_timeline_t)))
            return -EACCES;
        break;
    /* Try to Destroy a timeline */
    case QOTUSR_DESTROY_TIMELINE:
        if (copy_from_user(&msgt, (qot_timeline_t*)arg, sizeof(qot_timeline_t)))
            return -EACCES;
        if(qot_timeline_remove(&msgt, 0))
            return QOT_RETURN_TYPE_ERR;
        break;
    /* Wait until a time on a timeline reference */
    case QOTUSR_WAIT_UNTIL:
        if (copy_from_user(&sleeper, (qot_sleeper_t*)arg, sizeof(qot_sleeper_t)))
            return -EACCES;
        timeline = &sleeper.timeline;

        // Check if the timeline exists
        if (qot_timeline_get_info(&timeline))
            return -EACCES;

        // Wait until the required time
        wait_until_retval = qot_attosleep(&sleeper.wait_until_time, timeline);
        qot_clock_get_core_time(&sleeper.wait_until_time);

        // convert from core time to timeline reference of time
        coretime = TP_TO_nSEC(sleeper.wait_until_time.estimate);
        qot_loc2rem(timeline->index, 0, &coretime);
        TP_FROM_nSEC(sleeper.wait_until_time.estimate, coretime);

        /* Send the time at which the node woke up back to user */
        if (copy_to_user((qot_sleeper_t*)arg, &sleeper, sizeof(qot_sleeper_t)))
            return -EACCES;
        return wait_until_retval;
    case QOTUSR_OUTPUT_COMPARE_ENABLE:
        if (copy_from_user(&perout, (qot_perout_t*)arg, sizeof(qot_perout_t)))
            return -EACCES;

        timeline = &perout.timeline;

        // Check if the timeline exists
        if (qot_timeline_get_info(&timeline))
            return -EACCES;

        perout.timeline = *timeline;
        // Period Conversions
        period = (s64)TL_TO_nSEC(perout.period);
        qot_rem2loc(perout.timeline.index, 1, &period);
        TP_FROM_nSEC(core_period, period);

        // Start Conversions
        start = (s64)TP_TO_nSEC(perout.start);
        qot_rem2loc(perout.timeline.index, 1, &start);
        TP_FROM_nSEC(core_start, start);
        // Program the periodic output
        if(qot_clock_program_output_compare(&core_start, &core_period, &perout, 1, qot_perout_convert))
        {
            return -EACCES;
        }
        break;
    case QOTUSR_OUTPUT_COMPARE_DISABLE:
        if (copy_from_user(&perout, (qot_perout_t*)arg, sizeof(qot_perout_t)))
            return -EACCES;
        timeline = &perout.timeline;

        // Check if the timeline exists
        if (qot_timeline_get_info(&timeline))
            return -EACCES;

        perout.timeline = *timeline;
        // Disable the periodic output
        if(qot_clock_program_output_compare(&core_start, &core_period, &perout, 0, qot_perout_convert))
            return -EACCES;
        break;
    default:
        return -EINVAL;
    }
    return 0;
}

static unsigned int qot_user_chdev_poll(struct file *f, poll_table *wait)
{
    unsigned int mask = 0;
    qot_user_chdev_con_t *con = qot_user_chdev_con_search(f);
    if (con) {
        poll_wait(f, &con->wq, wait);
        if (con->event_flag && !list_empty(&con->event_list))
            mask |= POLLIN;
    }
    return mask;
}

static struct file_operations qot_user_chdev_fops = {
    .owner = THIS_MODULE,
    .open = qot_user_chdev_ioctl_open,
    .release = qot_user_chdev_ioctl_close,
    .unlocked_ioctl = qot_user_chdev_ioctl_access,
    .poll = qot_user_chdev_poll,
};

qot_return_t qot_user_chdev_init(struct class *qot_class)
{
    pr_info("qot_user_chdev: initializing\n");
    qot_major = register_chrdev(0,DEVICE_NAME,&qot_user_chdev_fops);
    if (qot_major < 0) {
        pr_err("qot_user_chdev: cannot register device\n");
        goto failed_chrdevreg;
    }
    qot_device = device_create(qot_class, NULL, MKDEV(qot_major, 0),
        NULL, DEVICE_NAME);
    if (IS_ERR(qot_device)) {
        pr_err("qot_user_chdev: cannot create device\n");
        goto failed_devreg;
    }
    return QOT_RETURN_TYPE_OK;
failed_devreg:
    unregister_chrdev(qot_major, DEVICE_NAME);
failed_chrdevreg:
    return QOT_RETURN_TYPE_ERR;
}

void qot_user_chdev_cleanup(struct class *qot_class)
{
    qot_user_chdev_con_t *con;
    struct rb_node *node;

    /* Clean up the character devices */
    pr_info("qot_user_chdev: cleaning up character devices\n");
    device_destroy(qot_class, MKDEV(qot_major, 0));
    unregister_chrdev(qot_major, DEVICE_NAME);

    /* Clean up the connections */
    pr_info("qot_user_chdev: flushing connections\n");
    while ((node = rb_first(&qot_user_chdev_con_root))) {
        con = container_of(node, qot_user_chdev_con_t, node);
        qot_user_chdev_con_remove(con);    /* Remove from red-black tree */
        qot_user_chdev_con_free(con);      /* Free memory */
    }
}

MODULE_LICENSE("GPL");