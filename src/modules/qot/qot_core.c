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

/* Modular components of QoT core */
#include "qot_core.h"
#include "qot_chardev_adm.h"
#include "qot_chardev_usr.h"

struct qot_core {
	u64 placeholder;
};

qot_return_t qot_clock_register(struct qot_platform_clock_info *info) {
    return QOT_RETURN_TYPE_ERR;
}
EXPORT_SYMBOL(qot_clock_register);

qot_return_t qot_clock_unregister(struct qot_platform_clock_info *info) {
    return QOT_RETURN_TYPE_ERR;
}
EXPORT_SYMBOL(qot_clock_unregister);

qot_return_t qot_clock_property_update(struct qot_platform_clock_info *info) {
    return QOT_RETURN_TYPE_ERR;
}
EXPORT_SYMBOL(qot_clock_property_update);

static int __init qot_init(void) {
    int ret;
    ret = qot_chardev_adm_init(void);
    if (ret) {
        pr_err("qot_core: problem calling qot_chardev_adm_init\n");
        goto fail_adm;
    }
    ret = qot_chardev_usr_init(void);
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

static void __exit qot_cleanup(void) {
    qot_chardev_adm_cleanup();
    qot_chardev_usr_cleanup();
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andrew Symington <asymingt@ucla.edu>");
MODULE_DESCRIPTION("QoT (core)");