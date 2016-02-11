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
#include <linux/cdev.h>
#include <linux/capability.h>
#include <linux/slab.h>

#include "qot_internal.h"

/*
static ssize_t ptp_pin_show(struct device *dev, struct device_attribute *attr,
    char *page) {

    struct ptp_clock *ptp = dev_get_drvdata(dev);
    unsigned int func, chan;
    int index;

    index = ptp_pin_name2index(ptp, attr->attr.name);
    if (index < 0)
        return -EINVAL;

    if (mutex_lock_interruptible(&ptp->pincfg_mux))
        return -ERESTARTSYS;

    func = ptp->info->pin_config[index].func;
    chan = ptp->info->pin_config[index].chan;

    mutex_unlock(&ptp->pincfg_mux);

    return snprintf(page, PAGE_SIZE, "%u %u\n", func, chan);
}

static ssize_t ptp_pin_store(struct device *dev, struct device_attribute *attr,
    const char *buf, size_t count) {
    struct ptp_clock *ptp = dev_get_drvdata(dev);
    unsigned int func, chan;
    int cnt, err, index;

    cnt = sscanf(buf, "%u %u", &func, &chan);
    if (cnt != 2)
        return -EINVAL;

    index = ptp_pin_name2index(ptp, attr->attr.name);
    if (index < 0)
        return -EINVAL;

    if (mutex_lock_interruptible(&ptp->pincfg_mux))
        return -ERESTARTSYS;
    err = ptp_set_pinfunc(ptp, index, func, chan);
    mutex_unlock(&ptp->pincfg_mux);
    if (err)
        return err;

    return count;
}
*/

DEVICE_ATTR(timeline_remove_all,    0600,   NULL, NULL);
DEVICE_ATTR(timeline_remove,        0600,   NULL, NULL);
DEVICE_ATTR(timeline_create,        0600,   NULL, NULL);
DEVICE_ATTR(timeline_list,          0600,   NULL, NULL);
DEVICE_ATTR(timeline_info,          0600,   NULL, NULL);
DEVICE_ATTR(clock_list,             0600,   NULL, NULL);
DEVICE_ATTR(clock_info,             0600,   NULL, NULL);
DEVICE_ATTR(clock_core,             0600,   NULL, NULL);

static struct attribute *qot_admin_attrs[] = {
    &dev_attr_timeline_remove_all.attr,
    &dev_attr_timeline_remove.attr,
    &dev_attr_timeline_create.attr,
    &dev_attr_timeline_list.attr,
    &dev_attr_timeline_info.attr,
    &dev_attr_clock_list.attr,
    &dev_attr_clock_info.attr,
    &dev_attr_clock_core.attr,
    NULL,
};

static const struct attribute_group qot_admin_group = {
    .attrs = qot_admin_attrs,
};

void qot_admin_sysfs_cleanup(struct device *qot_device) {
    sysfs_remove_group(&qot_device->kobj, &qot_admin_group);
}

qot_return_t qot_admin_sysfs_init(struct device *qot_device) {
    if (!qot_device)
        return QOT_RETURN_TYPE_ERR;
    sysfs_create_group(&qot_device->kobj, &qot_admin_group);
    return QOT_RETURN_TYPE_OK;
}
