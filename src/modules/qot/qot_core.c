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
    struct rb_node node_name;   /* Red-black tree indexes by name       */
    struct list_head head_acc;  /* Head pointing to maximum accuracy    */
    struct list_head head_res;  /* Head pointing to maximum resolution  */
} timeline_t;

/* Internal clock type */
typedef struct clk {
    qot_clk_info_t clk_info;    /* Clock information                    */
    struct rb_node node_name;   /* Red-black tree indexes by name       */
} clk_t;

/* Internal binding type */
typedef struct binding {
    timeline_t *timeline;       /* The demand associated with binding   */
    timequality_t demand;       /* Quality demand                       */
    struct rb_node node;        /* Red-black tree node                  */
    struct list_head res_list;  /* Next resolution (ordered)            */
    struct list_head acc_list;  /* Next accuracy (ordered)              */
} binding_t;

/* A red-black tree designed to find a timeline based on its name       */
static struct rb_root timeline_root = RB_ROOT;

/* A red-black tree designed to find a binding based on fileobject      */
static struct rb_root binding_root = RB_ROOT;

/* A red-black tree designed to find a clock based on its name          */
static struct rb_root clk_root = RB_ROOT;

/* The current clock set to core (initially this will be NULL)          */
static clk_t *core = NULL;

/* PRIVATE FUNCTIONS */

/* Insert a new binding into the accuracy list */
static inline void qot_insert_list_acc(binding_t *binding) {
    timelength_t nd, ed;  /* New demand (nd), existing demand (ed)  */
    binding_t *bindcur;   /* Existing binding in the data structure */
    list_for_each_entry(bindcur, head, acc_list) {
        timelength_min(&nd, binding->demand.accuracy.below,
            binding->demand.accuracy.above);
        timelength_min(&ed, bindcur->demand.accuracy.below,
            bindcur->demand.accuracy.above);
        if (timelength_cmp(nd,ed) > 0) {    /* NEW < EXISTING */
            list_add_tail(&binding->acc_list, &bindcur->acc_list);
            return;
        }
    }
    list_add_tail(&binding->acc_list, head);
}

/* Insert a new binding into the resolution list */
static inline void qot_insert_list_res(struct qot_binding *binding) {
    timelength_t *nd, *ed;  /* New demand (nd), existing demand (ed)  */
    binding_t *bindcur;     /* Existing binding in the data structure */
    list_for_each_entry(bindcur, head, res_list) {
        nd = &binding->demand.resolution;
        ed = &bindcur->demand.resolution;
        if (timelength_cmp(nd,ed) > 0) {    /* NEW < EXISTING */
            list_add_tail(&binding->res_list, &bindcur->res_list);
            return;
        }
    }
    list_add_tail(&binding->res_list, head);
}

/* Search for a timeline based on its name */
static struct qot_timeline *qot_timeline_search(const char *name) {
    int result;
    struct qot_timeline *timeline;
    struct rb_node *node = root->rb_node;
    while (node) {
        timeline = container_of(node, struct qot_timeline, node_uuid);
        result = strcmp(uuid, timeline->uuid);
        if (result < 0)
            node = node->rb_left;
        else if (result > 0)
            node = node->rb_right;
        else
            return timeline;
    }
    return NULL;
}

/* Insert a timeline into the red-black tree */
static int qot_timeline_insert(struct qot_timeline *data) {
    int result;
    struct qot_timeline *timeline;
    struct rb_node **new = &(root->rb_node), *parent = NULL;
    while (*new) {
        timeline = container_of(*new, struct qot_timeline, node_uuid);
        result = strcmp(data->uuid, timeline->uuid);
        parent = *new;
        if (result < 0)
            new = &((*new)->rb_left);
        else if (result > 0)
            new = &((*new)->rb_right);
        else
            return 0;
    }
    rb_link_node(&data->node_uuid, parent, new);
    rb_insert_color(&data->node_uuid, root);
    return 1;
}

/* INTERNAL FUNCTIONS */

/* Get information about a timeline */
qot_return_t qot_core_timeline_get_info(qot_timeline_t *timeline) {
    int result;
    struct qot_timeline *tmp;
    struct rb_node *node = timeline_root.rb_node;
    if (!timeline)
        return QOT_RETURN_TYPE_ERR;
    while (node) {
        tmp = container_of(node, timeline_t, node_name);
        result = strcmp(timeline->name, tmp->name);
        if (result < 0)
            node = node->rb_left;
        else if (result > 0)
            node = node->rb_right;
        else {
            memcpy(timeline, tmp, sizeof(timeline_t));
            return QOT_RETURN_TYPE_OK;
        }
    }
    return QOT_RETURN_TYPE_ERR;
}

/* Delete an existing binding from a timeline */
qot_return_t qot_core_timeline_del_binding(qot_binding_t *binding) {
    list_del(&binding->res_list);
    list_del(&binding->acc_list);
    if (list_empty(&binding->timeline->head_acc)) {
        rb_erase(&binding->timeline->node_name, &timeline_root);
        kfree(binding->timeline);
    }
    return QOT_RETURN_TYPE_OK;
}

/* Add a new binding to a timeline */
qot_return_t qot_core_timeline_add_binding(qot_timeline_t *timeline,
    qot_binding_t *binding) {
    int result;
    struct qot_binding *binding;
    struct rb_node **new = &(root->rb_node), *parent = NULL;
    while (*new)
    {
        binding = container_of(*new, struct qot_binding, node);
        result = data->fileobject - binding->fileobject;
        parent = *new;
        if (result < 0)
            new = &((*new)->rb_left);
        else if (result > 0)
            new = &((*new)->rb_right);
        else
            return QOT_RETURN_TYPE_OK;
    }
    rb_link_node(&data->node, parent, new);
    rb_insert_color(&data->node, root);
    return QOT_RETURN_TYPE_OK;
}

/* Get information about a clock */
qot_return_t qot_core_clock_get_info(qot_clock_t *clk) {
    clk_t *clk = qot_core_clock_find(clk->name);
    if (!clk)
        return QOT_RETURN_TYPE_ERR;
    clk->sleep();
    return QOT_RETURN_TYPE_OK;
}

/* Put the specified clock to sleep */
qot_return_t qot_core_clock_sleep(qot_clock_t *clk) {
    clk_t *clk = qot_core_clock_find(clk->name);
    if (!clk)
        return QOT_RETURN_TYPE_ERR;
    clk->sleep();
    return QOT_RETURN_TYPE_OK;
}

/* Wake up the specified clock */
qot_return_t qot_core_clock_wake(qot_clock_t *clk) {
    clk_t *clk = qot_core_clock_find(clk->name);
    if (!clk)
        return QOT_RETURN_TYPE_ERR;
    clk->wake();
    return QOT_RETURN_TYPE_OK;
}

/* Switch core to run from this clock */
qot_return_t qot_core_clock_switch(qot_clock_t *clk) {
    clk_t *clk = qot_core_clock_find(clk->name);
    if (!clk)
        return QOT_RETURN_TYPE_ERR;

    /* TODO: Switch core to use this clock */

    return QOT_RETURN_TYPE_OK;
}

/* EXPORTED SYMBOLS */

/* Register a clock with the QoT stack */
qot_return_t qot_clock_register(struct qot_platform_clock_info *info) {
    return QOT_RETURN_TYPE_ERR;
}
EXPORT_SYMBOL(qot_clock_register);

/* Unregister a clock with the QoT stack */
qot_return_t qot_clock_unregister(struct qot_platform_clock_info *info) {
    return QOT_RETURN_TYPE_ERR;
}
EXPORT_SYMBOL(qot_clock_unregister);

/* Notify the QoT stack that the clock properties have changed */
qot_return_t qot_clock_property_update(struct qot_platform_clock_info *info) {
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