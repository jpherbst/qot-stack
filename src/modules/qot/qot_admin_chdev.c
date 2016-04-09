/*
 * @file qot_admin_chdev.c
 * @brief Admin ioctl interface (/dev/qotadm) to the QoT core
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

#include <linux/module.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/rbtree.h>
#include <linux/sched.h>

#include "qot_admin.h"
#include "qot_clock.h"
#include "qot_timeline.h"

#define DEVICE_NAME "qotadm"

/* Internal event type */
typedef struct event {
    qot_event_t info;            /* The event type                      */
    struct list_head list;       /* The list of events head             */
} event_t;

/* Information that allows us to maintain parallel cchardev connections */
typedef struct qot_admin_chdev_con {
    struct rb_node node;                /* Red-black tree node */
    struct file *fileobject;            /* File object         */
    wait_queue_head_t wq;               /* Wait queue          */
    int event_flag;                     /* Data ready flag     */
    struct list_head event_list;        /* Event list          */
} qot_admin_chdev_con_t;

/* Information required to open a character device */
static struct device *qot_device = NULL;
static int qot_major;

/* Root of the red-black tree used to store parallel connections */
static struct rb_root qot_admin_chdev_con_root = RB_ROOT;

/* Free memory used by a connection */
static void qot_admin_chdev_con_free(qot_admin_chdev_con_t *con)
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
static qot_admin_chdev_con_t *qot_admin_chdev_con_search(struct file *f)
{
    int result;
    qot_admin_chdev_con_t *con;
    struct rb_node *node = qot_admin_chdev_con_root.rb_node;
    while (node) {
        con = container_of(node, qot_admin_chdev_con_t, node);
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
static void qot_admin_chdev_con_insert(qot_admin_chdev_con_t *con)
{
    int result;
    qot_admin_chdev_con_t *tmp;
    struct rb_node **new = &(qot_admin_chdev_con_root.rb_node), *parent = NULL;
    while (*new) {
        tmp = container_of(*new, qot_admin_chdev_con_t, node);
        result = con->fileobject - tmp->fileobject;
        parent = *new;
        if (result < 0)                 /* Traverse left    */
            new = &((*new)->rb_left);
        else if (result > 0)            /* Traverse right   */
            new = &((*new)->rb_right);
        else {                          /* Special case: already exists */
            rb_replace_node(*new, &con->node, &qot_admin_chdev_con_root);
            qot_admin_chdev_con_free(tmp);
            return;
        }
    }
    rb_link_node(&con->node, parent, new);
    rb_insert_color(&con->node, &qot_admin_chdev_con_root);
}

/* Remove a connection from the red-black tree */
static int qot_admin_chdev_con_remove(qot_admin_chdev_con_t *con)
{
    if (!con) {
        pr_err("qot_admin_chdev: could not find ioctl connection\n");
        return 1;
    }
    rb_erase(&con->node, &qot_admin_chdev_con_root);
    return 0;
}

/* chardev ioctl open callback implementation */
static int qot_admin_chdev_ioctl_open(struct inode *i, struct file *f)
{
    qot_clock_t clk;
    event_t *event;

    /* Create a new connection */
    qot_admin_chdev_con_t *con =
        kzalloc(sizeof(qot_admin_chdev_con_t), GFP_KERNEL);
    if (!con) {
        pr_err("qot_user_chdev: failed to allocate memory for connection\n");
        return -ENOMEM;
    }
    INIT_LIST_HEAD(&con->event_list);
    init_waitqueue_head(&con->wq);
    con->fileobject = f;
    con->event_flag = 0;

    /* Insert the connection into the red-black tree */
    qot_admin_chdev_con_insert(con);

    /* Notify the connection (by polling) of all existing timelines */
    if (qot_clock_first(&clk)==QOT_RETURN_TYPE_OK) {
        do {
            event = kzalloc(sizeof(event_t), GFP_KERNEL);
            if (!event) {
                pr_warn("qot_user_chdev: failed to allocate event\n");
                continue;
            }
            event->info.type = QOT_EVENT_CLOCK_CREATE;
            strncpy(event->info.data,clk.name,QOT_MAX_NAMELEN);
            list_add_tail(&event->list, &con->event_list);
        } while (qot_clock_next(&clk)==QOT_RETURN_TYPE_OK);
        con->event_flag = 1;
        wake_up_interruptible(&con->wq);
    }
    return 0;
}

/* chardev ioctl close callback implementation */
static int qot_admin_chdev_ioctl_close(struct inode *i, struct file *f)
{
    qot_admin_chdev_con_t *con = qot_admin_chdev_con_search(f);
    if (!con) {
        pr_err("qot_admin_chdev: could not find ioctl connection\n");
        return -ENOMEM;
    }
    qot_admin_chdev_con_remove(con);
    qot_admin_chdev_con_free(con);
    return 0;
}

/* chardev ioctl open access implementation */
static long qot_admin_chdev_ioctl_access(struct file *f, unsigned int cmd,
    unsigned long arg)
{
    event_t *event;
    qot_event_t msge;
    qot_clock_t msgc;
    utimelength_t msgt;
    timepoint_t msgtp;
    qot_admin_chdev_con_t *con = qot_admin_chdev_con_search(f);
    if (!con)
        return -EACCES;
    switch (cmd) {
    /* Get the next event in the queue for this connection */
    case QOTADM_GET_NEXT_EVENT:
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
    /* Get clock information by name */
    case QOTADM_GET_CLOCK_INFO:
        if (copy_from_user(&msgc, (qot_clock_t*)arg, sizeof(qot_clock_t)))
            return -EACCES;
        if (qot_clock_get_info(&msgc))
            return -EACCES;
        if (copy_to_user((qot_clock_t*)arg, &msgc, sizeof(qot_clock_t)))
            return -EACCES;
        break;
    /* Tell a clock to sleep */
    case QOTADM_SET_CLOCK_SLEEP:
        if (copy_from_user(&msgc, (qot_clock_t*)arg, sizeof(qot_clock_t)))
            return -EACCES;
        if (qot_clock_sleep(&msgc))
            return -EACCES;
        break;
    /* Tell a clock to wake up */
    case QOTADM_SET_CLOCK_WAKE:
        if (copy_from_user(&msgc, (qot_clock_t*)arg, sizeof(qot_clock_t)))
            return -EACCES;
        if (qot_clock_wake(&msgc))
            return -EACCES;
        break;
    /* Switch core to a specific clock */
    case QOTADM_SET_CLOCK_ACTIVE:
        if (copy_from_user(&msgc, (qot_clock_t*)arg, sizeof(qot_clock_t)))
            return -EACCES;
        if (qot_clock_switch(&msgc))
            return -EACCES;
        break;
    /* Set OS Clock Read Latency */
    case QOTADM_SET_OS_LATENCY:
        if (copy_from_user(&msgt, (utimelength_t*)arg, sizeof(utimelength_t)))
            return -EACCES;
        if (qot_admin_set_latency(&msgt))
            return -EACCES;
        break;
    /* Get OS Clock Read Latency */
    case QOTADM_GET_OS_LATENCY:
        if (qot_admin_get_latency(&msgt))
            return -EACCES;
        if (copy_to_user((utimelength_t*)arg, &msgt, sizeof(utimelength_t)))
            return -EACCES;
        break;
    /* Returns Core time without any uncertainity estimates */
    case QOTADM_GET_CORE_TIME_RAW:
        if (qot_clock_get_core_time_raw(&msgtp))
            return -EACCES;
        if (copy_to_user((timepoint_t*)arg, &msgtp, sizeof(timepoint_t)))
            return -EACCES;
        break;
    default:
        return -EINVAL;
    }
    return 0;
}

static unsigned int qot_admin_chdev_poll(struct file *f, poll_table *wait)
{
    unsigned int mask = 0;
    qot_admin_chdev_con_t *con = qot_admin_chdev_con_search(f);
    if (con) {
        poll_wait(f, &con->wq, wait);
        if (con->event_flag && !list_empty(&con->event_list))
            mask |= POLLIN;
    }
    return mask;
}

static struct file_operations qot_admin_chdev_fops = {
    .owner = THIS_MODULE,
    .open = qot_admin_chdev_ioctl_open,
    .release = qot_admin_chdev_ioctl_close,
    .unlocked_ioctl = qot_admin_chdev_ioctl_access,
    .poll = qot_admin_chdev_poll,
};

qot_return_t qot_admin_chdev_init(struct class *qot_class)
{
    pr_info("qot_admin_chdev: initializing\n");
    qot_major = register_chrdev(0,DEVICE_NAME,&qot_admin_chdev_fops);
    if (qot_major < 0) {
        pr_err("qot_admin_chdev: cannot register device\n");
        goto failed_chrdevreg;
    }
    qot_device = device_create(qot_class, NULL, MKDEV(qot_major, 0),
        NULL, DEVICE_NAME);
    if (IS_ERR(qot_device)) {
        pr_err("qot_admin_chdev: cannot create device\n");
        goto failed_devreg;
    }
    if (qot_admin_sysfs_init(qot_device)) {
        pr_err("qot_admin_chdev: cannot create sysfs interface\n");
        goto failed_sysfsreg;
    }
    return QOT_RETURN_TYPE_OK;
failed_sysfsreg:
    device_destroy(qot_class, MKDEV(qot_major, 0));
failed_devreg:
    unregister_chrdev(qot_major, DEVICE_NAME);
failed_chrdevreg:
    return QOT_RETURN_TYPE_ERR;
}

void qot_admin_chdev_cleanup(struct class *qot_class)
{
    qot_admin_chdev_con_t *con;
    struct rb_node *node;

    /* Remove the sysfs interface to this device */
    pr_info("qot_admin_chdev: cleaning up sysfs interface\n");
    qot_admin_sysfs_cleanup(qot_device);

    /* Clean up the character devices */
    pr_info("qot_admin_chdev: cleaning up character devices\n");
    device_destroy(qot_class, MKDEV(qot_major, 0));
    unregister_chrdev(qot_major, DEVICE_NAME);

    /* Clean up the connections */
    pr_info("qot_admin_chdev: flushing connections\n");
    while ((node = rb_first(&qot_admin_chdev_con_root))) {
        con = container_of(node, qot_admin_chdev_con_t, node);
        qot_admin_chdev_con_remove(con);    /* Remove from red-black tree */
        qot_admin_chdev_con_free(con);      /* Free memory */
    }
}

MODULE_LICENSE("GPL");