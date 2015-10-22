/*
 * @file qot_timelines.c
 * @brief Linux 4.1.6 kernel module for creation anmd destruction of QoT timelines
 * @author Andrew Symington and Fatima Anwar 
 * 
 * Copyright (c) Regents of the University of California, 2015. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 * 	1. Redistributions of source code must retain the above copyright notice, 
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, 
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// Kernel APIs
#include <linux/idr.h>
#include <linux/rbtree.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/fs.h>

// This file includes
#include "qot.h"
#include "qot_clock.h"
#include "qot_timeline.h"

// Stores information about a timeline
struct qot_timeline {
    char uuid[QOT_MAX_UUIDLEN];			// Unique id for this timeline
    struct rb_node node;				// Red-black tree is used to store timelines on UUID
    struct list_head head_acc;			// Head pointing to maximum accuracy structure
    struct list_head head_res;			// Head pointing to maximum resolution structure
    int index;							// POSIX clock index
};

// Stores information about an application binding to a timeline
struct qot_binding {
	struct qot_metric request;			// Requested accuracy and resolution 
    struct list_head res_list;			// Next resolution (ordered)
    struct list_head acc_list;			// Next accuracy (ordered)
    struct list_head list;				// Next binding (gives ability to iterate over all)
    struct qot_timeline* timeline;		// Parent timeline
};

// Use the IDR mechanism to dynamically allocate binding ids and find them quickly
static struct idr idr_bindings;

// A hash table designed to quickly find a timeline based on its UUID
static struct rb_root timeline_root = RB_ROOT;

// DATA STRUCTURE MANAGEMENT ///////////////////////////////////////////////////

// Insert a new binding into the accuracy list
static inline void qot_insert_list_acc(struct qot_binding *binding_list, struct list_head *head)
{
	struct qot_binding *bind_obj;
	list_for_each_entry(bind_obj, head, acc_list)
	{
		if (bind_obj->request.acc > binding_list->request.acc)
		{
			list_add_tail(&binding_list->acc_list, &bind_obj->acc_list);
			return;
		}
	}
	list_add_tail(&binding_list->acc_list, head);
}

// Insert a new binding into the resolution list
static inline void qot_insert_list_res(struct qot_binding *binding_list, struct list_head *head)
{
	struct qot_binding *bind_obj;
	list_for_each_entry(bind_obj, head, res_list)
	{
		if (bind_obj->request.res > binding_list->request.res)
		{
			list_add_tail(&binding_list->res_list, &bind_obj->res_list);
			return;
		}
	}
	list_add_tail(&binding_list->res_list, head);
}

// Search our data structure for a given timeline
static struct qot_timeline *qot_timeline_search(struct rb_root *root, const char *uuid)
{
	int result;
	struct qot_timeline *timeline;
	struct rb_node *node = root->rb_node;
	while (node)
	{
		timeline = container_of(node, struct qot_timeline, node);
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

// Insert a timeline into our data structure
static int qot_timeline_insert(struct rb_root *root, struct qot_timeline *data)
{
	int result;
	struct qot_timeline *timeline;
	struct rb_node **new = &(root->rb_node), *parent = NULL;
	while (*new)
	{
		timeline = container_of(*new, struct qot_timeline, node);
		result = strcmp(data->uuid, timeline->uuid);
		parent = *new;
		if (result < 0)
			new = &((*new)->rb_left);
		else if (result > 0)
			new = &((*new)->rb_right);
		else
			return 0;
	}
	rb_link_node(&data->node, parent, new);
	rb_insert_color(&data->node, root);
	return 1;
}

// GENERAL FUNCTIONS ////////////////////////////////////////////////////////

// Bind to a clock called UUID with a given accuracy and resolution
int32_t qot_timeline_bind(const char *uuid, uint64_t acc, uint64_t res)
{	
	struct qot_binding *binding;
	int bid;

	// Allocate memory for the new binding
	binding = kzalloc(sizeof(struct qot_binding), GFP_KERNEL);
	if (binding == NULL)
		return -ENOMEM;

	// Obtain a new binding ID
	bid = idr_alloc(&idr_bindings, binding, 0, MINORMASK + 1, GFP_KERNEL);
	if (bid < 0)
		goto free_binding;

	// Copy over data accuracy and resolution requirements
	binding->request.acc = acc;
	binding->request.res = res;

	// Check to see if this hash element exists
	binding->timeline = qot_timeline_search(&timeline_root, uuid);

	// If the timeline does not exist, we need to create it
	if (!binding->timeline)
	{
		// If we get to this point, there is no corresponding UUID
		binding->timeline = kzalloc(sizeof(struct qot_timeline), GFP_KERNEL);
		
		// Copy over the UUID and register a clock
		strncpy(binding->timeline->uuid, uuid, QOT_MAX_UUIDLEN);
		binding->timeline->index = qot_clock_register(uuid);
		if (binding->timeline->index < 0)	
			goto free_timeline;
		
		// Initialize list heads
		INIT_LIST_HEAD(&binding->timeline->head_acc);
		INIT_LIST_HEAD(&binding->timeline->head_res);

		// Add the timeline
		qot_timeline_insert(&timeline_root, binding->timeline);
	}

	// In both cases, we must insert and sort the two linked lists
	qot_insert_list_acc(binding, &binding->timeline->head_acc);
	qot_insert_list_res(binding, &binding->timeline->head_res);

	// Success
	return 0;

// If there is a problem at any point
free_timeline:
	kfree(binding->timeline);

free_binding:
	kfree(binding);
	return -1;
}

// Get the POSIX clock index for a given binding ID
int qot_timeline_index(int bid)
{
	// Grab the binding from the ID
	struct qot_binding *binding = idr_find(&idr_bindings, bid);
	if (!binding)
		return -1;

	// Return the timeline clock index
	return binding->timeline->index;	
}

// Completely destroy a binding
int qot_destroy_binding(int bid, void *p, void *data)
{
	// Cast to find the binding information
	struct qot_binding *binding = (struct qot_binding *) p;
	if (!binding)
		return -1;

	// Remove the entries from the sort tables (ordering will be updated)
	list_del(&binding->res_list);
	list_del(&binding->acc_list);

	// If we have removed the last binding from a timeline, then the lists
	// associated with the timeline should both be empty
	if (	list_empty(&binding->timeline->head_acc) 
		&&  list_empty(&binding->timeline->head_res))
	{
		// Unregister the QoT clock
		qot_clock_unregister(binding->timeline->index);

		// Remove the timeline node from the red-black tree
		rb_erase(&binding->timeline->node, &timeline_root);

		// Free the timeline entry memory
		kfree(binding->timeline);
	}

	// Free the memory used by this binding
	kfree(binding);

	// Remove the binding ID form the list
	idr_remove(&idr_bindings, bid);

	// Success
	return 0;
}

// Unbind from a timeline
int qot_timeline_unbind(int bid)
{
	// Grab the binding from the ID
	struct qot_binding *binding;
	struct qot_timeline *timeline;

	// Find the binding associated with the given index
	binding = idr_find(&idr_bindings, bid);
	if (!binding)
		return -1;

	// Find the timeline to which this binding points
	timeline = binding->timeline;

	// Remove the binding completely
	qot_destroy_binding(bid, (void *)binding, NULL);
	
	// This unbinding may lead to a change in target
	qot_clock_set_target(timeline->index, 
		list_entry(timeline->head_res.next, struct qot_binding, res_list)->request.acc,
		list_entry(timeline->head_res.next, struct qot_binding, res_list)->request.res
	);

	// Success
	return 0;
}

// Update the accuracy of a binding
int qot_timeline_set_accuracy(int bid, uint64_t acc)
{
	// Grab the binding from the ID
	struct qot_binding *binding = idr_find(&idr_bindings, bid);
	if (!binding)
		return -1;

	// Copy over the accuracy
	binding->request.acc = acc;

	// Delete the item from the list
	list_del(&binding->acc_list);

	// Add the item back into the list at the correct point
	qot_insert_list_acc(binding, &binding->timeline->head_acc);

	return 0;
}

// Update the resolution of a binding
int qot_timeline_set_resolution(int bid, uint64_t res)
{
	// Grab the binding from the ID
	struct qot_binding *binding = idr_find(&idr_bindings, bid);
	if (!binding)
		return -1;

	// Copy over the resolution
	binding->request.res = res;

	// Delete the item from the list
	list_del(&binding->res_list);

	// Add the item back into the list at the correct point
	qot_insert_list_res(binding, &binding->timeline->head_res);

	return 0;
}

// Initialize the timeline system
int qot_timeline_init(void)
{
	// Allocates clock ids
	idr_init(&idr_bindings);

	// Success
	return 0;
} 

// Cleanup the timeline system
void qot_timeline_cleanup(void)
{
	// Desttroy every binding
	idr_for_each(&idr_bindings, qot_destroy_binding, NULL);

	// Allocates clock ids
	idr_destroy(&idr_bindings);
}