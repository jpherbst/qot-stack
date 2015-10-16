/*
 * @file qot_timelines.c
 * @brief Linux 4.1.6 kernel module for creation anmd destruction of QoT timelines
 * @author Fatima Anwar 
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
#include <linux/hashtable.h>
#include <linux/dcache.h>
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
    struct hlist_node collision_hash;	// Pointer to next timeline with common hash key
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
DEFINE_HASHTABLE(qot_timelines_hash, QOT_HASHTABLE_BITS);

// DATA STRUCTURE MANAGEMENT ///////////////////////////////////////////////////

// Generate an unsigned int hash from a null terminated string
static  uint32_t qot_hash_uuid(const char *uuid)
{
	uint32_t hash = init_name_hash();
	while (*uuid)
		hash = partial_name_hash(*uuid++, hash);
	return end_name_hash(hash);
}

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

// GENERAL FUNCTIONS ////////////////////////////////////////////////////////

// Bind to a clock called UUID with a given accuracy and resolution
int32_t qot_timeline_bind(const char *uuid, uint64_t acc, uint64_t res)
{	
	struct qot_timeline *timeline;
	struct qot_binding *binding;
	int bid;
	uint32_t key;

	// Allocate memory for the new binding
	binding = kzalloc(sizeof(struct qot_binding), GFP_KERNEL);
	if (binding == NULL)
		return -ENOMEM;

	// Obtain a new binding ID
	bid = idr_alloc(&idr_bindings, binding, 0, MINORMASK + 1, GFP_KERNEL);
	if (bid < 0)
		goto free_binding;

	// Generate a key for the UUID
	key = qot_hash_uuid(uuid);

	// Copy over data accuracy and resolution requirements
	binding->request.acc = acc;
	binding->request.res = res;
	binding->timeline = NULL;

	// Check to see if this hash element exists
	hash_for_each_possible(qot_timelines_hash, timeline, collision_hash, key)
		if (!strcmp(timeline->uuid, uuid)) binding->timeline = timeline;

	// If the hash element does not exist, add it
	if (!binding->timeline)
	{
		// If we get to this point, there is no corresponding UUID in the hash
		binding->timeline = kzalloc(sizeof(struct qot_timeline), GFP_KERNEL);
		
		// Copy over the UUID
		strncpy(binding->timeline->uuid, uuid, QOT_MAX_UUIDLEN);

		// Register a new clock for this timeline
		binding->timeline->index = qot_clock_register(uuid);
		if (binding->timeline->index < 0)	
			goto free_timeline;

		// Add the timeline
		hash_add(qot_timelines_hash, &binding->timeline->collision_hash, key);

		// Initialize list heads
		INIT_LIST_HEAD(&binding->timeline->head_acc);
		INIT_LIST_HEAD(&binding->timeline->head_res);
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

	// Remove the entries from the sort tables (ordering will be updated)
	list_del(&binding->res_list);
	list_del(&binding->acc_list);

	// If we have removed the last binding from a timeline, then the lists
	// associated with the timeline should both be empty
	if (	list_empty(&binding->timeline->head_acc) 
		&&  list_empty(&binding->timeline->head_res))
	{
		// Remove element from the hashmap
		hash_del(&binding->timeline->collision_hash);

		// Unregister the QoT clock
		qot_clock_unregister(binding->timeline->index);

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
	struct qot_binding *binding = idr_find(&idr_bindings, bid);
	if (!binding)
		return -1;

	// Remove the binding completely
	qot_destroy_binding(bid, (void *)binding, NULL);
	
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