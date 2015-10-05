#ifndef QOT_IOCTL_H
#define QOT_IOCTL_H

#include <linux/ioctl.h>
#include <linux/idr.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/posix-clock.h>
#include <linux/errno.h>
#include <linux/list.h>
#include <linux/hashtable.h>
#include <linux/types.h>
#include <linux/hash.h>

struct qot_binding
{
	uint32_t res;						// Desired resolution
	uint32_t acc;						// Desired accuracy
    struct list_head res_sort_list;		// Next resolution
    struct list_head acc_sort_list;		// Next accuracy
};

struct qot_timeline
{
    struct posix_clock clock;
    char   uuid[32];
    struct hlist_node collision_hash;	// pointer to next sorted clock in the list w.r.t accuracy
    struct qot_binding *head_acc;
    struct qot_binding *head_res;
};


struct qot_binding *binding_map[];
int next_binding = 0;


/** function calls to create / destory QoT clock device **/

extern struct qot_timeline_data *qot_clock_register(struct qot_clock *info);
extern int qot_clock_unregister(struct qot_timeline_data *qotclk);
extern int qot_clock_index(struct qot_timeline_data *qotclk);

/** unique code for ioctl **/
#define MAGIC_CODE 0xAB

/** read / write clock and schedule parameters **/
#define QOT_INIT_CLOCK  		_IOW(MAGIC_CODE, 0, qot_clock*)
#define QOT_RETURN_BINDING  	_IOR(MAGIC_CODE, 1, uint16_t*)
#define QOT_RELEASE_CLOCK       _IOW(MAGIC_CODE, 2, uint16_t*)
#define QOT_SET_ACCURACY 	    _IOW(MAGIC_CODE, 3, qot_clock*)
#define QOT_SET_RESOLUTION 	    _IOW(MAGIC_CODE, 4, qot_clock*)
#define QOT_SET_SCHED 	        _IOW(MAGIC_CODE, 5, qot_sched*)
#define QOT_GET_SCHED       	_IOR(MAGIC_CODE, 6, qot_sched*)
#define QOT_WAIT				_IOW(MAGIC_CODE, 7, qot_sched*)
#define QOT_WAIT_CYCLES			_IOW(MAGIC_CODE, 8, qot_sched*)

/** read clock metadata from virtual qot clocks **/
#define QOT_GET_TIMELINE_ID		_IOR(MAGIC_CODE, 9, uint16_t*)
#define QOT_GET_ACCURACY		_IOR(MAGIC_CODE, A, timespec*)
#define QOT_GET_RESOLUTION		_IOR(MAGIC_CODE, B, timespec*)
	
#endif
