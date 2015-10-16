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
#include <linux/hashtable.h>
#include <linux/dcache.h>
#include <linux/module.h>
#include <linux/slab.h>

// This file includes
#include "qot.h"
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
    struct list_head res_list;			// Next resolution 
    struct list_head acc_list;			// Next accuracy
    struct list_head list;				// Next binding
    struct qot_timeline* timeline;		// Parent timeline
};

// Keep track of the next binding ID
static struct qot_binding *bindings[QOT_MAX_BINDINGS];
static int32_t binding_next =  0;
static int32_t binding_last = -1;

// A hash table designed to quickly find timeline based on its UUID
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

// Check to see if bindng is valid
static uint8_t qot_is_binding_invalid(uint32_t bid)
{
	return (bid < 0 || bid >= QOT_MAX_BINDINGS || !bindings[bid]);
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
	struct qot_timeline *obj;
	int32_t bid;
	uint32_t key;

	// Allocate memory for the new binding
	bindings[binding_next] = kzalloc(sizeof(struct qot_binding), GFP_KERNEL);
	if (bindings[binding_next] == NULL)
		return -ENOMEM;

	// Find the next binding in the sequence, and record the last
	for (bid = binding_next; binding_next < QOT_MAX_BINDINGS; binding_next++)
		if (!bindings[binding_next])
			break;
	if (binding_last < binding_next)
		binding_last = binding_next;		

	// Generate a key for the UUID
	key = qot_hash_uuid(uuid);

	// Copy over data accuracy and resolution requirements
	bindings[bid]->request.acc = acc;
	bindings[bid]->request.res = res;
	bindings[bid]->timeline = NULL;

	// Check to see if this hash element exists
	hash_for_each_possible(qot_timelines_hash, obj, collision_hash, key)
		if (!strcmp(obj->uuid, uuid))
			bindings[bid]->timeline = obj;

	// If the hash element does not exist, add it
	if (!bindings[bid]->timeline)
	{
		// If we get to this point, there is no corresponding UUID in the hash
		bindings[bid]->timeline = kzalloc(sizeof(struct qot_timeline), GFP_KERNEL);
		
		// Copy over the UUID
		strncpy(bindings[bid]->timeline->uuid, uuid, QOT_MAX_UUIDLEN);

		// Register the QoT clock
		if (qot_clock_register(bindings[bid]->timeline))
			return -EACCES;

		// Add the timeline
		hash_add(qot_timelines_hash, &bindings[bid]->timeline->collision_hash, key);

		// Initialize list heads
		INIT_LIST_HEAD(&bindings[bid]->timeline->head_acc);
		INIT_LIST_HEAD(&bindings[bid]->timeline->head_res);
	}

	// In both cases, we must insert and sort the two linked lists
	qot_insert_list_acc(bindings[bid], &bindings[bid]->timeline->head_acc);
	qot_insert_list_res(bindings[bid], &bindings[bid]->timeline->head_res);

	return 0;
}

// Get POSIX clock index for a timeline
int32_t qot_timeline_index(int32_t bid)
{
	if (qot_is_binding_invalid(bid))
		return -1;
	return bindings[bid]->timeline->index;
}

// Unbind from a timeline
int32_t qot_timeline_unbind(int32_t bid)
{
	if (qot_is_binding_invalid(bid))
		return -1;

	// Remove the entries from the sort tables (ordering will be updated)
	list_del(&bindings[bid]->res_list);
	list_del(&bindings[bid]->acc_list);

	// If we have removed the last binding from a timeline, then the lists
	// associated with the timeline should both be empty
	if (	list_empty(&bindings[bid]->timeline->head_acc) 
		&&  list_empty(&bindings[bid]->timeline->head_res))
	{
		// Remove element from the hashmap
		hash_del(&bindings[bid]->timeline->collision_hash);

		// Unregister the QoT clock
		qot_clock_unregister(bindings[bid]->timeline);

		// Free the timeline entry memory
		kfree(bindings[bid]->timeline);
	}

	// Free the memory used by this binding
	kfree(bindings[bid]);

	// Update the binding list
	if (bid < binding_next)
		binding_next = bid;		

	// Success
	return 0;
}

// Update the accuracy of a binding
int32_t qot_timeline_set_accuracy(int32_t bid, uint64_t acc)
{
	if (qot_is_binding_invalid(bid))
		return -1;

	// Copy over the accuracy
	bindings[bid]->request.acc = acc;

	// Delete the item from the list
	list_del(&bindings[bid]->acc_list);

	// Add the item back into the list at the correct point
	qot_insert_list_acc(bindings[bid], &bindings[bid]->timeline->head_acc);

	return 0;
}

// Update the resolution of a binding
int32_t qot_timeline_set_resolution(int32_t bid, uint64_t res)
{
	if (qot_is_binding_invalid(bid))
		return -1;

	// Copy over the resolution
	bindings[bid]->request.res = res;

	// Delete the item from the list
	list_del(&bindings[bid]->res_list);

	// Add the item back into the list at the correct point
	qot_insert_list_res(bindings[bid], &bindings[bid]->timeline->head_res);

	return 0;
}

// Initialize the timeline system
int32_t qot_timeline_init(void)
{
	return 0;
} 

// Cleanup the timeline system
void qot_timeline_cleanup(void)
{
	// Remove all bindings and timelines
	int bid;
	for (bid = 0; bid <= binding_last; bid++)
		qot_timeline_unbind(bid);
}