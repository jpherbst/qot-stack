/*
 * @file qot_clock.c
 * @brief Interface to clock management in the QoT stack
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

#include "qot_clock.h"

/* Private data */

/* Internal clock type */
typedef struct clk {
    qot_clock_impl_t impl;      /* Driver implementation               */
    struct rb_node node;        /* Red-black tree indexes by name      */
} clk_t;

/* The current clock set to core (initially this will be NULL)         */
static clk_t *core = NULL;

/* Root of the red-black tree used to store clocks */
static struct rb_root qot_core_clock_root = RB_ROOT;

/* Search for a timeline given by a name */
static clk_t *qot_core_find_clock(char *name) {
    int result;
    clk_t *clk = NULL;
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
    clk_t *target = NULL;
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

/* Public functions */

/* Get the next clock in the list */
qot_return_t qot_core_clock_next(qot_clock_t *clk) {
    clk_t *clk_priv = NULL;
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
    clk_t *clk_priv = NULL;
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
    clk_t *clk_priv = NULL;
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
    clk_t *clk_priv = NULL;
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
    clk_t *clk_priv = NULL;
    if (!clk)
        return QOT_RETURN_TYPE_ERR;
    clk_priv = qot_core_find_clock(clk->name);
    if (!clk_priv)
        return QOT_RETURN_TYPE_ERR;
    /* TODO: switch core clock */
    return QOT_RETURN_TYPE_OK;
}
