/*
 * @file qot_chardev_adm.c
 * @brief Administrative ioctl interface (/dev/qotadm) to the QoT core
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

#include "qot_internal.h"

static dev_t dev;
static struct cdev c_dev;
static struct class *cl;
static struct device *dev_ret;

// Data structure for storing timeline events
struct qot_chardev_adm_event {
    struct qot_event data;
    struct list_head list;
};

/* Information that allows us to maintain parallel cchardev connections */
struct qot_chardev_adm_con {
    struct rb_node node;                /* Red-black tree node */
    struct file *fileobject;            /* File object         */
    wait_queue_head_t wq;               /* Wait queue          */
    int event_flag;                     /* Data ready flag     */
    struct list_head event_list;        /* Event list          */
    qot_clock_t *clk;                   /* The current clock   */
};

/* Root of the red-black tree used to store parallel connections */
static struct rb_root qot_chardev_adm_con_root = RB_ROOT;

/* Search for the connection corresponding to a given fileobject */
static struct qot_chardev_adm_con *qot_chardev_adm_con_search(
    struct file *fileobject) {
    int result;
    struct qot_chardev_adm_con *con;
    struct rb_node *node = qot_chardev_adm_con_root.rb_node;
    while (node) {
        con = container_of(node, struct qot_chardev_adm_con, node);
        result = fileobject - con->fileobject;
        if (result < 0)
            node = node->rb_left;
        else if (result > 0)
            node = node->rb_right;
        else
            return con;
    }
    return NULL;
}

/* Insert a new connection into the data structure */
static int qot_chardev_adm_con_insert(struct qot_chardev_adm_con *data) {
    int result;
    struct qot_chardev_adm_con *con;
    struct rb_node **new = &(qot_chardev_adm_con_root.rb_node), *parent = NULL;
    while (*new) {
        con = container_of(*new, struct qot_chardev_adm_con, node);
        result = data->fileobject - con->fileobject;
        parent = *new;
        if (result < 0)
            new = &((*new)->rb_left);
        else if (result > 0)
            new = &((*new)->rb_right);
        else
            return 0;
    }
    rb_link_node(&data->node, parent, new);
    rb_insert_color(&data->node, &qot_chardev_adm_con_root);
    return 1;
}

/* Remove a binding from the red-black tree */
static int qot_chardev_adm_con_remove(struct qot_chardev_adm_con *data) {
    if (!data) {
        pr_err("qot_chardev_adm: could not find ioctl connection\n");
        return 1;
    }
    rb_erase(&data->node, &qot_chardev_adm_con_root);
    kfree(data);
    return 0;
}

/* chardev ioctl open callback implementation */
static int qot_chardev_adm_ioctl_open(struct inode *i, struct file *f) {
    struct qot_chardev_adm_con *con =
        kzalloc(sizeof(struct qot_chardev_adm_con), GFP_KERNEL);
    if (!con)
        return -ENOMEM;
    init_waitqueue_head(&con->wq);
    con->fileobject = f;
    con->event_flag = 0;
    if (qot_chardev_adm_con_insert(con))
        return 0;
    kfree(con);
    return 0;
}

/* chardev ioctl close callback implementation */
static int qot_chardev_adm_ioctl_close(struct inode *i, struct file *f) {
    struct qot_chardev_adm_con *con = qot_chardev_adm_con_search(f);
    if (!con) {
        pr_err("qot_chardev_adm: could not find ioctl connection\n");
        return -ENOMEM;
    }
    qot_chardev_adm_con_remove(con);
    return 0;
}

/* chardev ioctl open access implementation */
static long qot_chardev_adm_ioctl_access(struct file *f, unsigned int cmd,
    unsigned long arg) {
    qot_clock_t clk;
    struct qot_chardev_adm_con *con = qot_chardev_adm_con_search(f);
    if (!con)
        return -EACCES;
    switch (cmd) {
    /* Get clock information by name */
    case QOTADM_GET_CLOCK_INFO:
        if (copy_from_user(&clk, (qot_clock_t*)arg, sizeof(qot_clock_t)))
            return -EACCES;
        if (qot_core_clock_get_info(&clk))
            return -EACCES;
        if (copy_to_user((qot_clock_t*)arg, &clk, sizeof(qot_clock_t)))
            return -EACCES;
        break;
    /* Tell a clock to sleep */
    case QOTADM_SET_CLOCK_SLEEP:
        if (copy_from_user(&clk, (qot_clock_t*)arg, sizeof(qot_clock_t)))
            return -EACCES;
        if (qot_core_clock_sleep(&clk))
            return -EACCES;
        break;
    /* Tell a clock to wake up */
    case QOTADM_SET_CLOCK_WAKE:
        if (copy_from_user(&clk, (qot_clock_t*)arg, sizeof(qot_clock_t)))
            return -EACCES;
        if (qot_core_clock_wake(&clk))
            return -EACCES;
        break;
    /* Switch core to a specific clock */
    case QOTADM_SET_CLOCK_ACTIVE:
        if (copy_from_user(&clk, (qot_clock_t*)arg, sizeof(qot_clock_t)))
            return -EACCES;
        if (qot_core_clock_switch(&clk))
            return -EACCES;
        break;
    default:
        return -EINVAL;
    }
    return 0;
}

static unsigned int qot_chardev_adm_poll(struct file *f, poll_table *wait) {
    unsigned int mask = 0;
    struct qot_chardev_adm_con *con = qot_chardev_adm_con_search(f);
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
    pr_info("qot_chardev_adm: cleaning up\n");
    device_destroy(cl, dev);
    class_destroy(cl);
    cdev_del(&c_dev);
    unregister_chrdev_region(dev, 1);
    return QOT_RETURN_TYPE_OK;
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andrew Symington <asymingt@ucla.edu>");
MODULE_DESCRIPTION("QoT (admin interface)");