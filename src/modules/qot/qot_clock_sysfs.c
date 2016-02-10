/*
 * @file qot_sysfs.c
 * @brief A SYSFS based introspection of qot-core
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
#include <linux/printk.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/string.h>

#include "qot_sysfs.h"

static struct kobject *qot_kobject;
static int foo;

static ssize_t foo_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
        return sprintf(buf, "%d\n", foo);
}

static ssize_t foo_store(struct kobject *kobj, struct kobj_attribute *attr, char *buf, size_t count) {
        sscanf(buf, "%du", &foo);
        return count;
}

static struct kobj_attribute foo_attribute =__ATTR(foo, 0660, foo_show, foo_store);

int qot_sysfs_init(void) {

#ifdef CONFIG_SYSFS
    int error = 0;
    qot_kobject = kobject_create_and_add("qot",kernel_kobj);
    if(!qot_kobject) {
        pr_err("kobject_create_and_add failed\n");
        return -ENOMEM;
    }
    error = sysfs_create_file(qot_kobject, &foo_attribute.attr);
    if (error)
        pr_err("sysfs_create_group failed\n");
#endif

    return error;
}

void qot_sysfs_cleanup(void) {

#ifdef CONFIG_SYSFS
    kobject_put(qot_kobject);
#endif

}