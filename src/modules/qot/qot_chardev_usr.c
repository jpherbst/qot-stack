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

#include "qot_internal.h"

static dev_t dev;
static struct cdev c_dev;
static struct class *cl;
static struct device *dev_ret;

static int qot_chardev_usr_ioctl_open(struct inode *i, struct file *f) {
    return 0;
}

static int qot_chardev_usr_ioctl_close(struct inode *i, struct file *f) {
    return 0;
}

static long qot_chardev_usr_ioctl_access(struct file *f, unsigned int cmd,
    unsigned long arg) {
    return 0;
}

static unsigned int qot_chardev_usr_poll(struct file *f, poll_table *wait) {
    return 0;
}

static struct file_operations qot_chardev_usr_fops = {
    .owner = THIS_MODULE,
    .open = qot_chardev_usr_ioctl_open,
    .release = qot_chardev_usr_ioctl_close,
    .unlocked_ioctl = qot_chardev_usr_ioctl_access,
    .poll = qot_chardev_usr_poll,
};

int qot_chardev_usr_init(void) {
    int ret;
    pr_info("qot_chardev_usr: initializing\n");
    if ((ret = alloc_chrdev_region(&dev, 0, 1, "qotusr")) < 0)
        return ret;
    cdev_init(&c_dev, &qot_chardev_usr_fops);
    if ((ret = cdev_add(&c_dev, dev, 1)) < 0)
        return ret;
    if (IS_ERR(cl = class_create(THIS_MODULE, "qotusr"))) {
        cdev_del(&c_dev);
        unregister_chrdev_region(dev, 1);
        return PTR_ERR(cl);
    }
    if (IS_ERR(dev_ret = device_create(cl, NULL, dev, NULL, "qotusr"))) {
        class_destroy(cl);
        cdev_del(&c_dev);
        unregister_chrdev_region(dev, 1);
        return PTR_ERR(dev_ret);
    }
    return 0;
}

void qot_chardev_usr_cleanup(void) {
    pr_info("qot_chardev_usr: cleaning up\n");
    device_destroy(cl, dev);
    class_destroy(cl);
    cdev_del(&c_dev);
    unregister_chrdev_region(dev, 1);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andrew Symington <asymingt@ucla.edu>");
MODULE_DESCRIPTION("QoT (user interface)");