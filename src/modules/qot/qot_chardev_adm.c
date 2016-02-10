/*
 * @file qot_chardev_adm.c
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

#include "qot_internal.h"

/* Information required to open a character device */
static dev_t dev;
static struct cdev c_dev;
static struct class *cl;
static struct device *dev_ret;

/* Information that allows us to maintain parallel cchardev connections */
typedef  struct chardev_adm_con {
    struct rb_node node;                /* Red-black tree node */
    struct file *fileobject;            /* File object         */
    wait_queue_head_t wq;               /* Wait queue          */
    int event_flag;                     /* Data ready flag     */
    struct list_head event_list;        /* Event list          */
} chardev_adm_con_t;

/* Root of the red-black tree used to store parallel connections */
static struct rb_root qot_chardev_adm_con_root = RB_ROOT;

/* Free memory used by a connection */
static void qot_chardev_adm_con_free(chardev_adm_con_t *con) {
    event_t *event;
    struct list_head *item, *tmp;
    list_for_each_safe(item, tmp, &con->event_list) {
        event = list_entry(item, event_t, list);    /* Get the event  */
        list_del(item);                             /* Delte the item */
        kfree(event);                               /* Free up memory */
    }
}

/* Search for the connection corresponding to a given fileobject */
static chardev_adm_con_t *qot_chardev_adm_con_search(struct file *f) {
    int result;
    chardev_adm_con_t *con;
    struct rb_node *node = qot_chardev_adm_con_root.rb_node;
    while (node) {
        con = container_of(node, chardev_adm_con_t, node);
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
static void qot_chardev_adm_con_insert(chardev_adm_con_t *con) {
    int result;
    chardev_adm_con_t *tmp;
    struct rb_node **new = &(qot_chardev_adm_con_root.rb_node), *parent = NULL;
    while (*new) {
        tmp = container_of(*new, chardev_adm_con_t, node);
        result = con->fileobject - tmp->fileobject;
        parent = *new;
        if (result < 0)                 /* Traverse left    */
            new = &((*new)->rb_left);
        else if (result > 0)            /* Traverse right   */
            new = &((*new)->rb_right);
        else {                          /* Special case: already exists */
            rb_replace_node(*new, &con->node, &qot_chardev_adm_con_root);
            qot_chardev_adm_con_free(tmp);
            return;
        }
    }
    rb_link_node(&con->node, parent, new);
    rb_insert_color(&con->node, &qot_chardev_adm_con_root);
}

/* Remove a connection from the red-black tree */
static int qot_chardev_adm_con_remove(chardev_adm_con_t *con) {
    if (!con) {
        pr_err("qot_chardev_adm: could not find ioctl connection\n");
        return 1;
    }
    rb_erase(&con->node, &qot_chardev_adm_con_root);
    return 0;
}

/* chardev ioctl open callback implementation */
static int qot_chardev_adm_ioctl_open(struct inode *i, struct file *f) {
    qot_clock_t *clk;
    event_t *event;

    /* Create a new connection */
    chardev_adm_con_t *con =
        kzalloc(sizeof(chardev_adm_con_t), GFP_KERNEL);
    if (!con)
        goto fail2;
    INIT_LIST_HEAD(&con->event_list);
    init_waitqueue_head(&con->wq);
    con->fileobject = f;
    con->event_flag = 0;

    /* Insert the connection into the red-black tree */
    qot_chardev_adm_con_insert(con);

    /* Notify the connection (by polling) of all existing timelines */
    clk = NULL;
    while (qot_core_clock_next(clk)) {
        event = kzalloc(sizeof(event_t), GFP_KERNEL);
        if (!event)
            goto fail1;
        event->info.type = QOT_EVENT_CLOCK_CREATE;
        strncpy(event->info.data,clk->name,QOT_MAX_NAMELEN);
        list_add_tail(&event->list, &con->event_list);
    }
    con->event_flag = 1;
    wake_up_interruptible(&con->wq);

    /* Success */
    return 0;

fail1:
    /* Connection allocated, couln't be added to red-black tree */
    qot_chardev_adm_con_free(con);
fail2:
    /* Connection allocation issue */
    return -ENOMEM;
}

/* chardev ioctl close callback implementation */
static int qot_chardev_adm_ioctl_close(struct inode *i, struct file *f) {
    chardev_adm_con_t *con = qot_chardev_adm_con_search(f);
    if (!con) {
        pr_err("qot_chardev_adm: could not find ioctl connection\n");
        return -ENOMEM;
    }
    qot_chardev_adm_con_remove(con);
    qot_chardev_adm_con_free(con);
    return 0;
}

/* chardev ioctl open access implementation */
static long qot_chardev_adm_ioctl_access(struct file *f, unsigned int cmd,
    unsigned long arg) {
    event_t *event;
    qot_event_t msge;
    qot_clock_t msgc;
    chardev_adm_con_t *con = qot_chardev_adm_con_search(f);
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
        if (qot_core_clock_get_info(&msgc))
            return -EACCES;
        if (copy_to_user((qot_clock_t*)arg, &msgc, sizeof(qot_clock_t)))
            return -EACCES;
        break;
    /* Tell a clock to sleep */
    case QOTADM_SET_CLOCK_SLEEP:
        if (copy_from_user(&msgc, (qot_clock_t*)arg, sizeof(qot_clock_t)))
            return -EACCES;
        if (qot_core_clock_sleep(&msgc))
            return -EACCES;
        break;
    /* Tell a clock to wake up */
    case QOTADM_SET_CLOCK_WAKE:
        if (copy_from_user(&msgc, (qot_clock_t*)arg, sizeof(qot_clock_t)))
            return -EACCES;
        if (qot_core_clock_wake(&msgc))
            return -EACCES;
        break;
    /* Switch core to a specific clock */
    case QOTADM_SET_CLOCK_ACTIVE:
        if (copy_from_user(&msgc, (qot_clock_t*)arg, sizeof(qot_clock_t)))
            return -EACCES;
        if (qot_core_clock_switch(&msgc))
            return -EACCES;
        break;
    default:
        return -EINVAL;
    }
    return 0;
}

static unsigned int qot_chardev_adm_poll(struct file *f, poll_table *wait) {
    unsigned int mask = 0;
    chardev_adm_con_t *con = qot_chardev_adm_con_search(f);
    if (con) {
        poll_wait(f, &con->wq, wait);
        if (con->event_flag && !list_empty(&con->event_list))
            mask |= POLLIN;
    }
    return mask;
}

static struct file_operations qot_chardev_adm_fops = {
    .owner = THIS_MODULE,
    .open = qot_chardev_adm_ioctl_open,
    .release = qot_chardev_adm_ioctl_close,
    .unlocked_ioctl = qot_chardev_adm_ioctl_access,
    .poll = qot_chardev_adm_poll,
};

qot_return_t qot_chardev_adm_init(void) {
    int ret;
    pr_info("qot_chardev_adm: initializing\n");
    if ((ret = alloc_chrdev_region(&dev, 0, 1, "qotadm")) < 0)
        return ret;
    cdev_init(&c_dev, &qot_chardev_adm_fops);
    if ((ret = cdev_add(&c_dev, dev, 1)) < 0)
        return ret;
    if (IS_ERR(cl = class_create(THIS_MODULE, "qotadm"))) {
        cdev_del(&c_dev);
        unregister_chrdev_region(dev, 1);
        return PTR_ERR(cl);
    }
    if (IS_ERR(dev_ret = device_create(cl, NULL, dev, NULL, "qotadm"))) {
        class_destroy(cl);
        cdev_del(&c_dev);
        unregister_chrdev_region(dev, 1);
        return PTR_ERR(dev_ret);
    }
    return QOT_RETURN_TYPE_OK;
}

qot_return_t qot_chardev_adm_cleanup(void) {
    chardev_adm_con_t *con;
    struct rb_node *node;

    /* Clean up the character devices */
    pr_info("qot_chardev_adm: cleaning up character devices\n");
    device_destroy(cl, dev);
    class_destroy(cl);
    cdev_del(&c_dev);
    unregister_chrdev_region(dev, 1);

    /* Clean up the connections */
    pr_info("qot_chardev_adm: flushing connections\n");
    while ((node = rb_first(&qot_chardev_adm_con_root))) {
        con = container_of(node, chardev_adm_con_t, node);
        qot_chardev_adm_con_remove(con);    /* Remove from red-black tree */
        qot_chardev_adm_con_free(con);      /* Free memory */
    }
    /* Success */
    return QOT_RETURN_TYPE_OK;
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andrew Symington <asymingt@ucla.edu>");
MODULE_DESCRIPTION("QoT (admin interface)");
