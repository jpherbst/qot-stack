/*
 * @file qot_clock_sysfs.c
 * @brief Admin sysfs interface to the QoT core
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
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/device.h>

#include "qot_internal.h"

static int bus_value = 1;

static int qot_match(struct device *dev, struct device_driver *driver) {
    return !strncmp("qot", driver->name, strlen(driver->name));
}

static struct bus_type qot_bus = {
    .name   = "qot",
    .match  = qot_match,
};

static ssize_t bus_show(struct bus_type *bus, char *buf) {
    return scnprintf(buf, PAGE_SIZE, "%d\n", bus_value);
}

static ssize_t bus_store(struct bus_type *bus, const char *buf, size_t count) {
    sscanf(buf, "%d", &bus_value);
    return sizeof(int);
}
BUS_ATTR(timeline_delete_all,  S_IRUGO | S_IWUSR, bus_show, bus_store);

qot_return_t qot_sysfs_init(struct class *qot_class) {
    int ret = -1;
    pr_info("qot_sysfs: initializing\n");
    ret = bus_register(&qot_bus);
    if (ret < 0) {
        printk(KERN_WARNING "qot_sysfs: error register bus: %d\n", ret);
        return QOT_RETURN_TYPE_ERR;
    }
    ret = bus_create_file(&qot_bus, &bus_attr_timeline_delete_all);
    if (ret < 0) {
        printk(KERN_WARNING "qot_sysfs: error creating busfile\n");
        bus_unregister(&qot_bus);
        return QOT_RETURN_TYPE_ERR;
    }
    return QOT_RETURN_TYPE_OK;
}

qot_return_t qot_sysfs_cleanup(struct class *qot_class) {
    pr_info("qot_sysfs: removing files\n");
    bus_remove_file(&qot_bus, &bus_attr_timeline_delete_all);
    bus_unregister(&qot_bus);
    return QOT_RETURN_TYPE_OK;
}

MODULE_LICENSE("GPL");
