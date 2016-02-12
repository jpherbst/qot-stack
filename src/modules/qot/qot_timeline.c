/*
 * @file qot_timeline.c
 * @brief Interface to timeline management in the QoT stack
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

#include "qot_timeline.h"

/* Private functions */

/* Internal timeline type */
typedef struct timeline {
    qot_timeline_t info;        /* Timeline information                */
    struct rb_node node;        /* Red-black tree indexes by name      */
} timeline_t;

/* Root of the red-black tree used to store timelines */
static struct rb_root qot_timeline_root = RB_ROOT;

/* Search for a timeline given by a name */
static timeline_t *qot_timeline_find(char *name) {
    int result;
    timeline_t *timeline = NULL;
    struct rb_node *node = qot_timeline_root.rb_node;
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

/* Insert a timeline into our data structure */
static qot_return_t qot_timeline_insert(timeline_t *timeline) {
    int result;
    timeline_t *target;
    struct rb_node **new = &(qot_timeline_root.rb_node), *parent = NULL;
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
    rb_insert_color(&timeline->node, &qot_timeline_root);
    return QOT_RETURN_TYPE_OK;
}

/* Public functions */

/* Get the next timeline in the set */
qot_return_t qot_timeline_first(qot_timeline_t *timeline) {
    timeline_t *timeline_priv = NULL;
    struct rb_node *node;
    node = rb_first(&qot_timeline_root);
    timeline_priv = rb_entry(node, timeline_t, node);
    memcpy(timeline,&timeline_priv->info,sizeof(qot_timeline_t));
    return QOT_RETURN_TYPE_OK;
}

/* Get the next timeline in the set */
qot_return_t qot_timeline_next(qot_timeline_t *timeline) {
    timeline_t *timeline_priv = NULL;
    struct rb_node *node;
    if (!timeline)
        return QOT_RETURN_TYPE_ERR;
    timeline_priv = qot_timeline_find(timeline->name);
    if (!timeline_priv)
        return QOT_RETURN_TYPE_ERR;
    node = rb_next(&timeline_priv->node);
    if (!node)
        return QOT_RETURN_TYPE_ERR;
    timeline_priv = rb_entry(node, timeline_t, node);
    memcpy(timeline,&timeline_priv->info,sizeof(qot_timeline_t));
    return QOT_RETURN_TYPE_OK;
}

/* Get information about a timeline */
qot_return_t qot_timeline_get_info(qot_timeline_t *timeline) {
    timeline_t *timeline_priv = NULL;
    if (!timeline)
        return QOT_RETURN_TYPE_ERR;
    timeline_priv = qot_timeline_find(timeline->name);
    if (!timeline_priv)
        return QOT_RETURN_TYPE_ERR;
    memcpy(timeline,&timeline_priv->info,sizeof(qot_timeline_t));
    return QOT_RETURN_TYPE_OK;
}

/* Creata a new timeline */
qot_return_t qot_timeline_create(qot_timeline_t *timeline) {
    timeline_t *timeline_priv = NULL;
    if (!timeline)
        return QOT_RETURN_TYPE_ERR;
    /* Make sure timeline doesn't already exist */
    timeline_priv = qot_timeline_find(timeline->name);
    if (timeline_priv)
    	return QOT_RETURN_TYPE_ERR;
    /* Copy over the timeline information to some new private memory */
    timeline_priv = kzalloc(sizeof(timeline_t), GFP_KERNEL);
    if (!timeline_priv) {
        pr_err("qot_timeline: cannot allocate memory for timeline_priv");
		return QOT_RETURN_TYPE_ERR;
    }
	memcpy(&timeline_priv->info,timeline,sizeof(qot_timeline_t));
    /* Try and initialize the character device for this timeline */
    timeline_priv->info.index =
        qot_timeline_chdev_register(timeline_priv->info.name);
    if (timeline_priv->info.index < 0) {
    	pr_err("qot_timeline: cannot create the character device");
        goto fail_chdev_register;
    }
    /* Try and insert into the red-black tree */
    if (qot_timeline_insert(timeline_priv)) {
    	pr_err("qot_timeline: cannot insert the timeline");
        goto fail_timeline_insert;
    }
    /* Copy the IDR back to the user */
	memcpy(timeline,&timeline_priv->info,sizeof(qot_timeline_t));
    return QOT_RETURN_TYPE_OK;

fail_timeline_insert:
	qot_timeline_chdev_unregister(timeline_priv->info.index);
fail_chdev_register:
	kfree(timeline_priv);
	return QOT_RETURN_TYPE_ERR;
}

/* Remove a timeline */
qot_return_t qot_timeline_remove(qot_timeline_t *timeline) {
    timeline_t *timeline_priv = NULL;
    if (!timeline)
        return QOT_RETURN_TYPE_ERR;
    /* Make certain that timeline->index has been set */
    timeline_priv = qot_timeline_find(timeline->name);
	if (!timeline_priv)
		return QOT_RETURN_TYPE_ERR;
	qot_timeline_chdev_unregister(timeline_priv->info.index);
	rb_erase(&timeline_priv->node,&qot_timeline_root);
    kfree(timeline_priv);
    return QOT_RETURN_TYPE_OK;
}

/* Remove all timelines */
void qot_timeline_remove_all(void) {
    timeline_t *timeline, *timeline_next;
    rbtree_postorder_for_each_entry_safe(timeline, timeline_next,
        &qot_timeline_root, node) {
        qot_timeline_chdev_unregister(timeline->info.index);
        rb_erase(&timeline->node,&qot_timeline_root);
        kfree(timeline);
    }
}

/* Cleanup the timeline subsystem */
void qot_timeline_cleanup(struct class *qot_class) {
    qot_timeline_remove_all();
    qot_timeline_chdev_cleanup(qot_class);
}

/* Initialize the timeline subsystem */
qot_return_t qot_timeline_init(struct class *qot_class) {
    /* Initialize the character device */
    if (qot_timeline_chdev_init(qot_class)) {
        pr_err("qot_timeline: problem calling qot_timeline_chdev_init\n");
        goto fail_chdev_init;
    }
    return QOT_RETURN_TYPE_OK;
fail_chdev_init:
    return QOT_RETURN_TYPE_ERR;
}

MODULE_LICENSE("GPL");
