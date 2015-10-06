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

#define MAX_UUIDLEN 	16
#define MAX_BINDINGS 	(1 << (MAX_UUIDLEN))

/** clock source structure **/
typedef struct qot_clock_t
{
	char   timeline[MAX_UUIDLEN];		// UUID of reference timeline shared among all collaborating entities
	uint64_t accuracy;	       			// range of acceptable deviation from the reference timeline in nanosecond
	uint64_t resolution;	    		// required clock resolution
	int binding_id;		        		// id returned once the clock is iniatialized
} qot_clock;

/** scheduling parameters for an event **/
typedef struct qot_sched_t
{
	char   timeline[MAX_UUIDLEN];   	// schedule against this clock
	uint64_t event_time;       			// time for which event is scheduled
	uint64_t event_cycles;     			// wait cycles
	uint64_t high;		       			// time for high pulse
	uint64_t low;		       			// time for low pulse
	uint16_t limit;            			// number of event repetitions
} qot_sched;

struct qot_timeline
{
    struct posix_clock clock;
    struct device *dev;
	int references;						// number of references to the timeline
    dev_t devid;
    int index;                      	// index into clocks.map
    int defunct;                    	// tells readers to go away when clock is being removed
    char   uuid[MAX_UUIDLEN];			// unique id for a timeline
    struct hlist_node collision_hash;	// pointer to next timeline for the same hash key
    struct list_head head_acc;			// head pointing to maximum accuracy structure
    struct list_head head_res;			// head pointing to maximum resolution structure
};

struct qot_binding
{
	char   uuid[MAX_UUIDLEN]; 
	uint64_t resolution;				// Desired resolution
	uint64_t accuracy;					// Desired accuracy
    struct list_head res_sort_list;		// Next resolution
    struct list_head acc_sort_list;		// Next accuracy
};

// pointers in an array to a clock binding data
struct qot_binding *binding_map[MAX_BINDINGS];

// binding id for the applications
int next_binding = 0;

/** function calls to create / destory QoT clock device **/

extern struct qot_timeline *qot_timeline_register(char uuid[]);
extern int qot_timeline_unregister(struct qot_timeline *qotclk);
extern int qot_timeline_index(struct qot_timeline *qotclk);

/** unique code for ioctl **/
#define MAGIC_CODE 0xAB

/** read / write clock and schedule parameters **/
#define QOT_INIT_CLOCK  		_IOW(MAGIC_CODE, 0, qot_clock*)
#define QOT_RETURN_BINDING  	_IOR(MAGIC_CODE, 1, int*)
#define QOT_RELEASE_CLOCK       _IOW(MAGIC_CODE, 2, int*)
#define QOT_SET_ACCURACY 	    _IOW(MAGIC_CODE, 3, qot_clock*)
#define QOT_SET_RESOLUTION 	    _IOW(MAGIC_CODE, 4, qot_clock*)
#define QOT_SET_SCHED 	        _IOW(MAGIC_CODE, 5, qot_sched*)
#define QOT_GET_SCHED       	_IOR(MAGIC_CODE, 6, qot_sched*)
#define QOT_WAIT				_IOW(MAGIC_CODE, 7, qot_sched*)
#define QOT_WAIT_CYCLES			_IOW(MAGIC_CODE, 8, qot_sched*)

/** read clock metadata from virtual qot clocks **/
#define QOT_GET_TIMELINE_ID		_IOR(MAGIC_CODE, 9, char*)
#define QOT_GET_ACCURACY		_IOR(MAGIC_CODE, 0xA, uint64_t*)
#define QOT_GET_RESOLUTION		_IOR(MAGIC_CODE, 0xB, uint64_t*)
	
#endif
