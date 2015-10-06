#ifndef QOT_IOCTL_H
#define QOT_IOCTL_H

#include <linux/ioctl.h>

#define MAX_UUIDLEN 	16
#define MAX_BINDINGS 	(1 << (MAX_UUIDLEN))

/** clock source structure **/
typedef struct qot_message_t {
	char   	 uuid[MAX_UUIDLEN];			// UUID of reference timeline shared among all collaborating entities
	uint64_t acc;	       				// Range of acceptable deviation from the reference timeline in nanosecond
	uint64_t res;	    				// Required clock resolution
	int      bid;				   		// id returned once the clock is iniatialized
} qot_message;

/** unique code for ioctl **/
#define MAGIC_CODE 0xAB

/** read / write clock and schedule parameters **/
#define QOT_BIND  			    _IOWR(MAGIC_CODE, 1, qot_message*)
#define QOT_GET_ACCURACY		_IOWR(MAGIC_CODE, 5, qot_message*)
#define QOT_GET_RESOLUTION		_IOWR(MAGIC_CODE, 6, qot_message*)
#define QOT_SET_ACCURACY 		 _IOW(MAGIC_CODE, 3, qot_message*)
#define QOT_SET_RESOLUTION 		 _IOW(MAGIC_CODE, 4, qot_message*)
#define QOT_UNBIND 				 _IOW(MAGIC_CODE, 2, int32_t*)
	
#endif
