/*
 * @file qot_core.c
 * @brief The Quality of Time (QoT) core kernel module
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
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/rbtree.h>

/* Project submodules */
#include "qot_clock.h"
#include "qot_timeline.h"
#include "qot_scheduler.h"
#include "qot_admin.h"
#include "qot_user.h"

/* All device drivers must be registered with this class to appear in sysfs */
#define CLASS_NAME "qot"

/* Class to hold all QoT devices */
static struct class *qot_class = NULL;

/* Exported functions */

/* Register a clock with the QoT stack */
qot_return_t qot_register(qot_clock_impl_t *impl)
{
    if (qot_clock_register(impl))
        return QOT_RETURN_TYPE_ERR;
    return QOT_RETURN_TYPE_OK;
}
EXPORT_SYMBOL(qot_register);

/* Unregister a clock with the QoT stack */
qot_return_t qot_unregister(qot_clock_impl_t *impl)
{
    if (qot_clock_unregister(impl))
        return QOT_RETURN_TYPE_ERR;
    return QOT_RETURN_TYPE_OK;
}
EXPORT_SYMBOL(qot_unregister);

/* Notify the QoT stack that the clock properties have changed */
qot_return_t qot_update(qot_clock_impl_t *impl)
{
    if (qot_clock_update(impl))
        return QOT_RETURN_TYPE_ERR;
    return QOT_RETURN_TYPE_OK;
}
EXPORT_SYMBOL(qot_update);

/* Internal functions */

/* Initialize the QoT core */
static int qot_init(void)
{
    qot_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(qot_class)) {
        pr_err("qot_chardev_usr: cannot create device class\n");
        goto failed_classreg;
    }
    if (qot_clock_init(qot_class)) {
        pr_err("qot_core: problem calling qot_clock_init\n");
        goto fail_clock;
    }
    if (qot_timeline_init(qot_class)) {
        pr_err("qot_core: problem calling qot_timeline_init\n");
        goto fail_timeline;
    }
    if (qot_scheduler_init(qot_class)) {
        pr_err("qot_core: problem calling qot_scheduler_init\n");
        goto fail_scheduler;
    }
    if (qot_admin_init(qot_class)) {
        pr_err("qot_core: problem calling qot_admin_init\n");
        goto fail_admin;
    }
    if (qot_user_init(qot_class)) {
        pr_err("qot_core: problem calling qot_user_init\n");
        goto fail_user;
    }
    return 0;
fail_user:
    qot_admin_cleanup(qot_class);
fail_admin:
    qot_scheduler_cleanup(qot_class);
fail_scheduler:
    qot_timeline_cleanup(qot_class);
fail_timeline:
    qot_clock_cleanup(qot_class);
fail_clock:
    class_destroy(qot_class);
failed_classreg:
	return -EIO;
}

/* Cleanup the QoT core */
static void qot_cleanup(void)
{
    qot_user_cleanup(qot_class);
    qot_admin_cleanup(qot_class);
    qot_scheduler_cleanup(qot_class);
    qot_timeline_cleanup(qot_class);
    qot_clock_cleanup(qot_class);
    class_destroy(qot_class);
}

module_init(qot_init);
module_exit(qot_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("RoseLine Project");
MODULE_DESCRIPTION("Quality of Time (QoT)");
