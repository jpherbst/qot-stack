/*
 * @file qot_timeline_sysfs.c
 * @brief Admin sysfs interface to a given timeline
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

#include "qot_timeline.h"

static ssize_t name_show(struct device *dev, struct device_attribute *attr,
	char *buf)
{
 	return scnprintf(buf, PAGE_SIZE, "%s\n", "default");
}
DEVICE_ATTR(name, 0444, name_show, NULL);

static struct attribute *qot_timeline_attrs[] = {
    &dev_attr_name.attr,
    NULL,
};

static const struct attribute_group qot_timeline_group = {
    .attrs = qot_timeline_attrs,
};

void qot_timeline_sysfs_cleanup(struct device *qot_device)
{
    sysfs_remove_group(&qot_device->kobj, &qot_timeline_group);
}

qot_return_t qot_timeline_sysfs_init(struct device *qot_device)
{
    if (!qot_device)
        return QOT_RETURN_TYPE_ERR;
    if (sysfs_create_group(&qot_device->kobj, &qot_timeline_group))
        return QOT_RETURN_TYPE_ERR;
    return QOT_RETURN_TYPE_OK;
}
