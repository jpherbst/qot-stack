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

/* Internal timeline type */
typedef struct timeline {
    qot_timeline_t info;        /* Timeline information                 */
    struct rb_node node;        /* Red-black tree indexes by name       */
} timeline_t;

/* Internal clock type */
typedef struct clk {
    qot_clock_impl_t impl;      /* Driver implementation               */
    struct rb_node node;        /* Red-black tree indexes by name      */
} clk_t;

/* The current clock set to core (initially this will be NULL)          */
static clk_t *core = NULL;

/* Root of the red-black tree used to store timelines */
static struct rb_root qot_core_timeline_root = RB_ROOT;

/* Root of the red-black tree used to store clocks */
static struct rb_root qot_core_clock_root = RB_ROOT;

/* Class to hold all QoT devices */
static struct class *qot_class = NULL;

/* PRIVATE FUNCTIONS */

/* Search for a timeline given by a name */
static timeline_t *qot_core_find_timeline(char *name) {
    int result;
    timeline_t *timeline;
    struct rb_node *node = qot_core_timeline_root.rb_node;
    while (node) {
        timeline = container_of(node, timeline_t, node);
        result = strcmp(name, timeline->info.name);
        if (result < 0)
            node = node->rb_left;
        else if (result > 0)
            node = node->rb_right;
        else
            return timeline;
    }
    return NULL;
}

// Insert a timeline into our data structure
static qot_return_t qot_core_insert_timeline(timeline_t *timeline) {
    int result;
    timeline_t *target;
    struct rb_node **new = &(qot_core_timeline_root.rb_node), *parent = NULL;
    while (*new) {
        target = container_of(*new, timeline_t, node);
        result = strcmp(timeline->info.name, target->info.name);
        parent = *new;
        if (result < 0)
            new = &((*new)->rb_left);
        else if (result > 0)
            new = &((*new)->rb_right);
        else
            return QOT_RETURN_TYPE_ERR;
    }
    rb_link_node(&timeline->node, parent, new);
    rb_insert_color(&timeline->node, &qot_core_timeline_root);
    return QOT_RETURN_TYPE_OK;
}

/* Search for a timeline given by a name */
static clk_t *qot_core_find_clock(char *name) {
    int result;
    clk_t *clk;
    struct rb_node *node = qot_core_clock_root.rb_node;
    while (node) {
        clk = container_of(node, clk_t, node);
        result = strcmp(name, clk->impl.info.name);
        if (result < 0)
            node = node->rb_left;
        else if (result > 0)
            node = node->rb_right;
        else
            return clk;
    }
    return NULL;
}

// Insert a timeline into our data structure
static qot_return_t qot_core_insert_clock(clk_t *clk) {
    int result;
    clk_t *target;
    struct rb_node **new = &(qot_core_clock_root.rb_node), *parent = NULL;
    while (*new) {
        target = container_of(*new, clk_t, node);
        result = strcmp(clk->impl.info.name, target->impl.info.name);
        parent = *new;
        if (result < 0)
            new = &((*new)->rb_left);
        else if (result > 0)
            new = &((*new)->rb_right);
        else
            return QOT_RETURN_TYPE_ERR;
    }
    rb_link_node(&clk->node, parent, new);
    rb_insert_color(&clk->node, &qot_core_clock_root);
    return QOT_RETURN_TYPE_OK;
}

/* INTERNAL FUNCTIONS */

/* Get the next timeline in the set */
qot_return_t qot_core_timeline_next(qot_timeline_t *timeline) {
    timeline_t *timeline_priv;
    struct rb_node *node;
    if (!timeline) {
        node = rb_first(&qot_core_clock_root);
    } else {
        timeline_priv = qot_core_find_timeline(timeline->name);
        if (!timeline_priv)
            return QOT_RETURN_TYPE_ERR;
        node = rb_next(&timeline_priv->node);
    }
    if (!node)
        return QOT_RETURN_TYPE_ERR;
    timeline_priv = rb_entry(node, timeline_t, node);
    memcpy(timeline,&timeline_priv->info,sizeof(qot_timeline_t));
    return QOT_RETURN_TYPE_OK;
}

/* Get information about a timeline */
qot_return_t qot_core_timeline_get_info(qot_timeline_t *timeline) {
    timeline_t *timeline_priv;
    if (!timeline)
        return QOT_RETURN_TYPE_ERR;
    timeline_priv = qot_core_find_timeline(timeline->name);
    if (!timeline_priv)
        return QOT_RETURN_TYPE_ERR;
    memcpy(timeline,&timeline_priv->info,sizeof(qot_timeline_t));
    return QOT_RETURN_TYPE_OK;
}

/* Delete an existing binding from a timeline */
qot_return_t qot_core_timeline_create(qot_timeline_t *timeline) {
    timeline_t *timeline_priv;
    if (!timeline)
        return QOT_RETURN_TYPE_ERR;
    timeline_priv = kzalloc(sizeof(timeline_t), GFP_KERNEL);
    if (!timeline_priv)
        return QOT_RETURN_TYPE_ERR;
    memcpy(&timeline_priv->info,&timeline,sizeof(qot_timeline_t));
    /* TODO: full initialization of the timeline itself (SYSFS) */
    timeline->timeline_id = 0; /* for now */
    if (qot_core_insert_timeline(timeline_priv)) {
        kfree(timeline_priv);
        return QOT_RETURN_TYPE_ERR;
    }
    return QOT_RETURN_TYPE_OK;
}

/* Get the next clock in the list */
qot_return_t qot_core_clock_next(qot_clock_t *clk) {
    clk_t *clk_priv;
    struct rb_node *node;
    if (!clk) {
        node = rb_first(&qot_core_clock_root);
    } else {
        clk_priv = qot_core_find_clock(clk->name);
        if (!clk_priv)
            return QOT_RETURN_TYPE_ERR;
        node = rb_next(&clk_priv->node);
    }
    if (!node)
        return QOT_RETURN_TYPE_ERR;
    clk_priv = rb_entry(node, clk_t, node);
    memcpy(clk,&clk_priv->impl.info,sizeof(qot_clock_t));
    return QOT_RETURN_TYPE_OK;
}

/* Get information about a clock */
qot_return_t qot_core_clock_get_info(qot_clock_t *clk) {
    clk_t *clk_priv;
    if (!clk)
        return QOT_RETURN_TYPE_ERR;
    clk_priv = qot_core_find_clock(clk->name);
    if (!clk_priv)
        return QOT_RETURN_TYPE_ERR;
    memcpy(clk,&clk_priv->impl.info,sizeof(qot_clock_t));
    return QOT_RETURN_TYPE_OK;
}

/* Put the specified clock to sleep */
qot_return_t qot_core_clock_sleep(qot_clock_t *clk) {
    clk_t *clk_priv;
    if (!clk)
        return QOT_RETURN_TYPE_ERR;
    clk_priv = qot_core_find_clock(clk->name);
    if (!clk_priv)
        return QOT_RETURN_TYPE_ERR;
    clk_priv->impl.sleep();
    return QOT_RETURN_TYPE_OK;
}

/* Wake up the specified clock */
qot_return_t qot_core_clock_wake(qot_clock_t *clk) {
    clk_t *clk_priv;
    if (!clk)
        return QOT_RETURN_TYPE_ERR;
    clk_priv = qot_core_find_clock(clk->name);
    if (!clk_priv)
        return QOT_RETURN_TYPE_ERR;
    clk_priv->impl.wake();
    return QOT_RETURN_TYPE_OK;
}

/* Switch core to run from this clock */
qot_return_t qot_core_clock_switch(qot_clock_t *clk) {
    clk_t *clk_priv;
    if (!clk)
        return QOT_RETURN_TYPE_ERR;
    clk_priv = qot_core_find_clock(clk->name);
    if (!clk_priv)
        return QOT_RETURN_TYPE_ERR;
    /* TODO: switch core clock */
    return QOT_RETURN_TYPE_OK;
}

/* EXPORTED SYMBOLS */

/* Register a clock with the QoT stack */
qot_return_t qot_clock_register(qot_clock_impl_t *impl) {
    clk_t *clk_priv;
    if (!impl)
        return QOT_RETURN_TYPE_ERR;
    clk_priv = kzalloc(sizeof(clk_t), GFP_KERNEL);
    if (!clk_priv)
        return QOT_RETURN_TYPE_ERR;
    memcpy(&clk_priv->impl,impl,sizeof(qot_clock_impl_t));
    if (qot_core_insert_clock(clk_priv)) {
        kfree(clk_priv);
        return QOT_RETURN_TYPE_ERR;
    }
    return QOT_RETURN_TYPE_OK;
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
    qot_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(qot_class)) {
        pr_err("qot_chardev_usr: cannot create device class\n");
        goto failed_classreg;
    }
    ret = qot_chardev_adm_init(qot_class);
    if (ret) {
        pr_err("qot_core: problem calling qot_chardev_adm_init\n");
        goto fail_adm;
    }
    ret = qot_chardev_usr_init(qot_class);
    if (ret) {
        pr_err("qot_core: problem calling qot_chardev_usr_init\n");
        goto fail_usr;
    }
    ret = qot_sysfs_init(qot_class);
    if (ret) {
        pr_err("qot_core: problem calling qot_sysfs_init\n");
        goto fail_sys;
    }
    return 0;
fail_sys:
    qot_chardev_usr_cleanup(qot_class);
fail_usr:
    qot_chardev_adm_cleanup(qot_class);
fail_adm:
    class_destroy(qot_class);
failed_classreg:
	return 1;
}

/* Cleanup the QoT core */
static void qot_cleanup(void) {
    if (qot_sysfs_cleanup(qot_class))
        pr_err("qot_core: problem calling qot_sysfs_cleanup\n");
    if (qot_chardev_usr_cleanup(qot_class))
        pr_err("qot_core: problem calling qot_chardev_usr_cleanup\n");
    if (qot_chardev_adm_cleanup(qot_class))
        pr_err("qot_core: problem calling qot_chardev_adm_cleanup\n");
    class_destroy(qot_class);
}

module_init(qot_init);
module_exit(qot_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andrew Symington <asymingt@ucla.edu>");
MODULE_DESCRIPTION("QoT (core)");