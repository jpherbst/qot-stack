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

#include "qot_admin.h"
#include "qot_clock.h"
#include "qot_timeline.h"

/*
   Only *general* functions are supported:
   Timelines : R: list, W: create, remove, remove all
   Clocks    : R: list, R/w: core (view and switch)
   Introspection of the timelines and clocks is done on the devices
*/

/* Create a timeline */
static ssize_t timeline_create_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
    qot_timeline_t timeline;
    int cnt = sscanf(buf, "%s", timeline.name);
    if (cnt != 1) {
    	pr_err("qot_admin_sysfs: could not capture the timeline name\n");
        return -EINVAL;
    }
    if (qot_timeline_create(&timeline)) {
    	pr_err("qot_admin_sysfs: could not create a timeline\n");
    	return -EINVAL;
    }
    return count;
}
DEVICE_ATTR(timeline_create, 0600, NULL, timeline_create_store);

/* Destroy a timeline */
static ssize_t timeline_remove_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
    qot_timeline_t timeline;
    int cnt = sscanf(buf, "%s", timeline.name);
    if (cnt != 1) {
    	pr_err("qot_admin_sysfs: could not capture the timeline name\n");
        return -EINVAL;
    }
	if (qot_timeline_remove(&timeline)) {
		pr_err("qot_admin_sysfs: could not remove a timeline\n");
		return -EINVAL;
	}
    return count;
}
DEVICE_ATTR(timeline_remove, 0600, NULL, timeline_remove_store);

/* Remove all timelines */
static ssize_t timeline_remove_all_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	qot_timeline_remove_all();
    return count;
}
DEVICE_ATTR(timeline_remove_all, 0600, NULL, timeline_remove_all_store);

/* List all timelines */
static ssize_t clock_core_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
    return 0;
}
static ssize_t clock_core_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
    return count;
}
DEVICE_ATTR(clock_core, 0600, clock_core_show, clock_core_store);

/* List all timelines */
static ssize_t os_latency_usec_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
    u64 est, below, above;
    utimelength_t timelength;
    if (qot_admin_get_os_latency(&timelength))
        return -EINVAL;
    est = TO_uSEC(timelength.estimate);
    below = TO_uSEC(timelength.interval.below);
    above = TO_uSEC(timelength.interval.above);
    return scnprintf(buf, PAGE_SIZE, "%llu %llu %llu\n", est, below, above);
}
static ssize_t os_latency_usec_store(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    u64 est, below, above;
    utimelength_t timelength;
    int cnt = sscanf(buf, "%llu %llu %llu", est, below, above);
    switch (cnt) {
    case 1:
        below = 0;
    case 2:
        above = below;
    case 3:
        INIT_uSEC(timelength.estimate,estimate);
        INIT_uSEC(timelength.below,below);
        INIT_uSEC(timelength.above,above);
        if (qot_admin_set_os_latency(&timelength))
            return -EINVAL;
        break;
    default:
        pr_err("qot_admin_sysfs: could not capture the os latency\n");
        return -EINVAL;
    }
    return count;
}
DEVICE_ATTR(os_latency_usec, 0600, os_latency_usec_show, os_latency_usec_store);

static struct attribute *qot_admin_attrs[] = {
    &dev_attr_timeline_remove_all.attr,
    &dev_attr_timeline_remove.attr,
    &dev_attr_timeline_create.attr,
    &dev_attr_clock_core.attr,
    &dev_attr_os_latency_usec.attr,
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
    if (sysfs_create_group(&qot_device->kobj, &qot_admin_group))
        return QOT_RETURN_TYPE_ERR;
    return QOT_RETURN_TYPE_OK;
}
