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
	struct device_attribute *attr, const char *buf, size_t count) {
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
	struct device_attribute *attr, const char *buf, size_t count) {
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
	struct device_attribute *attr, const char *buf, size_t count) {
	qot_timeline_t timeline;
	if (qot_timeline_first(&timeline)) {
	 	do {
			if (qot_timeline_remove(&timeline)) {
				pr_err("qot_admin_sysfs: could not remove a timeline\n");
				return -EINVAL;
			}
	 	} while (!qot_timeline_next(&timeline));
	}
    return count;
}
DEVICE_ATTR(timeline_remove_all, 0600, NULL, timeline_remove_all_store);

/* List all timelines */
static ssize_t timeline_list_show(struct device *dev,
	struct device_attribute *attr, char *buf) {
	qot_timeline_t timeline;
	if (qot_timeline_first(&timeline)) {
	 	do {
	 		/* Print to buffer */
	 	} while (!qot_timeline_next(&timeline));
	}
    return 0;
}
DEVICE_ATTR(timeline_list, 0600, timeline_list_show, NULL);

/* List all clocks */
static ssize_t clock_list_show(struct device *dev,
	struct device_attribute *attr, char *buf) {
	qot_clock_t clk;
	if (qot_clock_first(&clk)) {
	 	do {

	 	} while (!qot_clock_next(&clk));
	}
    return 0;
}
DEVICE_ATTR(clock_list, 0600, clock_list_show, NULL);

/* List all timelines */
static ssize_t clock_core_show(struct device *dev,
    struct device_attribute *attr, char *buf) {
    return 0;
}
static ssize_t clock_core_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count) {
    return count;
}
DEVICE_ATTR(clock_core, 0600, clock_core_show, clock_core_store);

static struct attribute *qot_admin_attrs[] = {
    &dev_attr_timeline_remove_all.attr,
    &dev_attr_timeline_remove.attr,
    &dev_attr_timeline_create.attr,
    &dev_attr_timeline_list.attr,
    &dev_attr_clock_list.attr,
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
