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

#include "qot_internal.h"

/* The current clock set to core (initially this will be NULL)          */
//static clk_t *core = NULL;

/* PRIVATE FUNCTIONS */

/* INTERNAL FUNCTIONS */

/* Get the next timeline in the set */
qot_return_t qot_core_timeline_next(qot_timeline_t *timeline) {
    return QOT_RETURN_TYPE_ERR;
}

/* Get information about a timeline */
qot_return_t qot_core_timeline_get_info(qot_timeline_t *timeline) {
    return QOT_RETURN_TYPE_ERR;
}

/* Delete an existing binding from a timeline */
qot_return_t qot_core_timeline_create(qot_timeline_t *timeline) {
    return QOT_RETURN_TYPE_OK;
}

/* Get the next clock in the list */
qot_return_t qot_core_clock_next(qot_clock_t *clk) {
    return QOT_RETURN_TYPE_ERR;
}

/* Get information about a clock */
qot_return_t qot_core_clock_get_info(qot_clock_t *clk) {
    return QOT_RETURN_TYPE_ERR;
}

/* Put the specified clock to sleep */
qot_return_t qot_core_clock_sleep(qot_clock_t *clk) {
    return QOT_RETURN_TYPE_ERR;
}

/* Wake up the specified clock */
qot_return_t qot_core_clock_wake(qot_clock_t *clk) {
    return QOT_RETURN_TYPE_ERR;
}

/* Switch core to run from this clock */
qot_return_t qot_core_clock_switch(qot_clock_t *clk) {
    return QOT_RETURN_TYPE_ERR;
}

/* EXPORTED SYMBOLS */

/* Register a clock with the QoT stack */
qot_return_t qot_clock_register(qot_clock_impl_t *impl) {
    return QOT_RETURN_TYPE_ERR;
}
EXPORT_SYMBOL(qot_clock_register);

/* Unregister a clock with the QoT stack */
qot_return_t qot_clock_unregister(qot_clock_impl_t *impl) {
    return QOT_RETURN_TYPE_ERR;
}
EXPORT_SYMBOL(qot_clock_unregister);

/* Notify the QoT stack that the clock properties have changed */
qot_return_t qot_clock_property_update(qot_clock_impl_t *impl) {
    return QOT_RETURN_TYPE_ERR;
}
EXPORT_SYMBOL(qot_clock_property_update);

/* MODULE INITIALIZATION */

/* Initialize the QoT core */
static int qot_init(void) {
    int ret;
    ret = qot_chardev_adm_init();
    if (ret) {
        pr_err("qot_core: problem calling qot_chardev_adm_init\n");
        goto fail_adm;
    }
    ret = qot_chardev_usr_init();
    if (ret) {
        pr_err("qot_core: problem calling qot_chardev_usr_init\n");
        goto fail_usr;
    }
    return 0;
fail_usr:
    qot_chardev_adm_cleanup();
fail_adm:
	return 1;
}

/* Cleanup the QoT core */
static void qot_cleanup(void) {
    if (qot_chardev_adm_cleanup())
        pr_err("qot_core: problem cleaning up qot_chardev_adm_init\n");
    if (qot_chardev_usr_cleanup())
        pr_err("qot_core: problem cleaning up qot_chardev_usr_init\n");
}

module_init(qot_init);
module_exit(qot_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andrew Symington <asymingt@ucla.edu>");
MODULE_DESCRIPTION("QoT (core)");