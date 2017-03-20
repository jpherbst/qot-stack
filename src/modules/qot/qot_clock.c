/*
 * @file qot_clock.c
 * @brief Interface to clock management in the QoT stack
 * @author Andrew Symington and Sandeep D'souza
 *
 * Copyright (c) Regents of the University of California, 2015.
 * Copyright (c) Carnegie Mellon University 2016.
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
#include "qot_admin.h"

/* Private data */

/* Internal clock type */
typedef struct clk {
    qot_clock_impl_t impl;      /* Driver implementation               */
    struct rb_node node;        /* Red-black tree indexes by name      */
} clk_t;

/* Root of the red-black tree used to store clocks */
static struct rb_root qot_clock_root = RB_ROOT;

/* Root of the red-black tree used to store clocks */
static clk_t *core = NULL;

/* Get the presiding core clock */
qot_return_t qot_get_core_clock(qot_clock_t *clk)
{   
    if(core == NULL)
        return QOT_RETURN_TYPE_ERR;
    *clk = core->impl.info;
    return QOT_RETURN_TYPE_OK;
}

/* Search for a clock given by a name */
static clk_t *qot_clock_find(char *name) {
    int result;
    clk_t *clk = NULL;
    struct rb_node *node = qot_clock_root.rb_node;
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

/* Insert a clock into our data structure */
static qot_return_t qot_clock_insert(clk_t *clk)
{
    int result;
    clk_t *target = NULL;
    struct rb_node **new = &(qot_clock_root.rb_node), *parent = NULL;
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
    rb_insert_color(&clk->node, &qot_clock_root);
    return QOT_RETURN_TYPE_OK;
}

/* Public functions */

/* Get the core time (with query uncertainty added) */
qot_return_t qot_clock_get_core_time(utimepoint_t *utp)
{
    if (!utp || !core)
        return QOT_RETURN_TYPE_ERR;
    /* Get a measurement of core time */
    utp->estimate = core->impl.read_time();
    TL_FROM_uSEC(utp->interval.below, 0);
    TL_FROM_uSEC(utp->interval.above, 0);
    /* Add the uncertainty to the measurement */
    // utimepoint_add(utp, &core->impl.info.read_latency);
    qot_admin_add_latency(utp);
    /* Success */
    return QOT_RETURN_TYPE_OK;
}

/* Get the core time without uncertainity estimate */
qot_return_t qot_clock_get_core_time_raw(timepoint_t *tp)
{
    if (!tp || !core)
        return QOT_RETURN_TYPE_ERR;
    /* Get a measurement of core time */
    *tp = core->impl.read_time();
    /* Success */
    return QOT_RETURN_TYPE_OK;
}

/* Program an interrupt on core time */
qot_return_t qot_clock_program_core_interrupt(timepoint_t expiry, int force, long (*callback)(void))
{
    if (!core)
        return QOT_RETURN_TYPE_ERR;
    /* Program an interrupt on core time */
    if(core->impl.program_interrupt(expiry, force, callback))
        return QOT_RETURN_TYPE_ERR;
    /* Success */
    return QOT_RETURN_TYPE_OK;
}

/* Program a PWM */
qot_return_t qot_clock_program_output_compare(timepoint_t *core_start, timelength_t *core_period, qot_perout_t *perout, int on, qot_return_t (*callback)(qot_perout_t *perout_ret, timepoint_t *event_core_timestamp, timepoint_t *next_event))
{
    long retval = 0;
    if (!core)
        return QOT_RETURN_TYPE_ERR;
    /* Program an interrupt on core time */
    retval = core->impl.enable_compare(core_start, core_period, perout, callback, on);
    if(retval)
    {
        return QOT_RETURN_TYPE_ERR;
    }
    /* Success */
    return QOT_RETURN_TYPE_OK;
}

/* Add Interrupt Latency uncertainity to a measurement */
qot_return_t qot_clock_add_core_interrupt_latency(utimepoint_t *utp)
{
    if (!utp || !core)
        return QOT_RETURN_TYPE_ERR;
    /* Add the uncertainty to the measurement */
    utimepoint_add(utp, &core->impl.info.interrupt_latency);
    /* Success */
    return QOT_RETURN_TYPE_OK;
}



qot_return_t qot_clock_register(qot_clock_impl_t *impl)
{
    clk_t *clk_priv = NULL;
    if (!impl)
        return QOT_RETURN_TYPE_ERR;
    clk_priv = kzalloc(sizeof(clk_t), GFP_KERNEL);
    if (!clk_priv)
        return QOT_RETURN_TYPE_ERR;
    memcpy(&clk_priv->impl,impl,sizeof(qot_clock_impl_t));
    if (qot_clock_insert(clk_priv)) {
        kfree(clk_priv);
        return QOT_RETURN_TYPE_ERR;
    }
    /* If no other clock is acting as the core select this clock as the core */
    if(!core)
        core = clk_priv;   
    qot_admin_clock_register_notify(&clk_priv->impl.info);
    return QOT_RETURN_TYPE_OK;
}

qot_return_t qot_clock_unregister(qot_clock_impl_t *impl)
{
    clk_t *clk_priv = NULL;
    if (!impl)
        return QOT_RETURN_TYPE_ERR;
    clk_priv = qot_clock_find(impl->info.name);
    if (!clk_priv)
        return QOT_RETURN_TYPE_ERR;
    rb_erase(&clk_priv->node,&qot_clock_root);
    kfree(clk_priv);
    return QOT_RETURN_TYPE_OK;
}

qot_return_t qot_clock_update(qot_clock_impl_t *impl)
{
    clk_t *clk_priv = NULL;
    if (!clk_priv)
        return QOT_RETURN_TYPE_ERR;
    clk_priv = qot_clock_find(impl->info.name);
    memcpy(&clk_priv->impl,impl,sizeof(qot_clock_impl_t));
    return QOT_RETURN_TYPE_OK;
}

/* Get the next clk in the set */
qot_return_t qot_clock_first(qot_clock_t *clk)
{
    clk_t *clk_priv = NULL;
    struct rb_node *node;
    node = rb_first(&qot_clock_root);
    clk_priv = rb_entry(node, clk_t, node);
    memcpy(clk,&clk_priv->impl.info,sizeof(qot_clock_t));
    return QOT_RETURN_TYPE_OK;
}

/* Get the next timeline in the set */
qot_return_t qot_clock_next(qot_clock_t *clk)
{
    clk_t *clk_priv = NULL;
    struct rb_node *node;
    if (!clk)
        return QOT_RETURN_TYPE_ERR;
    clk_priv = qot_clock_find(clk->name);
    if (!clk_priv)
        return QOT_RETURN_TYPE_ERR;
    node = rb_next(&clk_priv->node);
    if (!node)
        return QOT_RETURN_TYPE_ERR;
    clk_priv = rb_entry(node, clk_t, node);
    memcpy(clk,&clk_priv->impl.info,sizeof(qot_clock_t));
    return QOT_RETURN_TYPE_OK;
}

qot_return_t qot_clock_get_info(qot_clock_t *clk)
{
    clk_t *clk_priv = NULL;
    if (!clk)
        return QOT_RETURN_TYPE_ERR;
    clk_priv = qot_clock_find(clk->name);
    if (!clk_priv)
        return QOT_RETURN_TYPE_ERR;
    memcpy(clk,&clk_priv->impl.info,sizeof(qot_clock_t));
    return QOT_RETURN_TYPE_OK;
}

qot_return_t qot_clock_remove(qot_clock_t *clk)
{
    clk_t *clk_priv = NULL;
    if (!clk)
        return QOT_RETURN_TYPE_ERR;
    /* Make certain that clk->index has been set */
    clk_priv = qot_clock_find(clk->name);
    if (!clk)
        return QOT_RETURN_TYPE_ERR;
    /* TODO: Remove any inner structures */
    rb_erase(&clk_priv->node,&qot_clock_root);
    kfree(clk_priv);
    return QOT_RETURN_TYPE_OK;
}

void qot_clock_remove_all(void)
{
    clk_t *clk, *clk_next;
    rbtree_postorder_for_each_entry_safe(clk, clk_next, &qot_clock_root, node) {
        /* TODO: Remove any inner structures */
        rb_erase(&clk->node,&qot_clock_root);
        kfree(clk);
    }
}

qot_return_t qot_clock_sleep(qot_clock_t *clk)
{
    clk_t *clk_priv = NULL;
    if (!clk)
        return QOT_RETURN_TYPE_ERR;
    clk_priv = qot_clock_find(clk->name);
    if (!clk_priv)
        return QOT_RETURN_TYPE_ERR;
    clk_priv->impl.sleep();
    return QOT_RETURN_TYPE_OK;
}

qot_return_t qot_clock_wake(qot_clock_t *clk)
{
    clk_t *clk_priv = NULL;
    if (!clk)
        return QOT_RETURN_TYPE_ERR;
    clk_priv = qot_clock_find(clk->name);
    if (!clk_priv)
        return QOT_RETURN_TYPE_ERR;
    clk_priv->impl.wake();
    return QOT_RETURN_TYPE_OK;
}

qot_return_t qot_clock_switch(qot_clock_t *clk)
{
    clk_t *clk_priv = NULL;
    if (!clk)
        return QOT_RETURN_TYPE_ERR;
    clk_priv = qot_clock_find(clk->name);
    if (!clk_priv)
        return QOT_RETURN_TYPE_ERR;
    /* TODO: switch core clock */
    return QOT_RETURN_TYPE_OK;
}

void qot_clock_cleanup(struct class *qot_class)
{
    clk_t *clk, *clk_next;
    /* Remove all clocks */
    rbtree_postorder_for_each_entry_safe(clk, clk_next, &qot_clock_root, node) {
        rb_erase(&clk->node, &qot_clock_root);
        kfree(clk);
    }
}

qot_return_t qot_clock_init(struct class *qot_class)
{ 
    return QOT_RETURN_TYPE_OK;
}
