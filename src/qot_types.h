/*
 * @file qot.h
 * @brief Data types, temporal math and ioctl interfaces for the qot-stack
 * @author Andrew Symington
 *
 * Copyright (c) Regents of the University of California, 2015.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 	1. Redistributions of source code must retain the above copyright notice,
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

#ifndef QOT_STACK_SRC_QOT_TYPES_H
#define QOT_STACK_SRC_QOT_TYPES_H

/* So that we might expose a meaningful name through PTP interface */
#define QOT_MAX_NAMELEN 	64
#define QOT_MAX_NUMCLKS 	8

/* For ease of conversion */
#define ASEC_PER_NSEC (u64)1000000000ULL
#define ASEC_PER_USEC (u64)1000000000000ULL
#define ASEC_PER_MSEC (u64)1000000000000000ULL

#define MAX_TIMEPOINT_SEC  9223372036LL
#define MAX_LL  9223372036854775807LL
#define MAX_ULL 18446744073709551615ULL

/* Cater for different C compilation pipelines (ie. no floats / doubles) */
#ifdef __KERNEL__
	#include <linux/ioctl.h>
 	#include <linux/math64.h>
    #include <linux/kernel.h>
    #include <linux/fs.h>
 	#define llabs abs
#else
	#include <time.h>
	#include <stdint.h>
 	#include <string.h>
	#include <sys/ioctl.h>
    #include <stdlib.h>
    #define div64_u64(X,Y) (uint64_t)X/(uint64_t)Y;
 	#define u64 uint64_t
 	#define s64  int64_t
 	#define u32 uint32_t
 	#define s32  int32_t
#endif

/* Operations on time */

/* Popular ratios that will be used throughout this file */
#define  SEC_PER_SEC  1ULL
#define mSEC_PER_SEC  1000ULL
#define uSEC_PER_SEC  1000000ULL
#define nSEC_PER_SEC  1000000000ULL
#define pSEC_PER_SEC  1000000000000ULL
#define fSEC_PER_SEC  1000000000000000ULL
#define aSEC_PER_SEC  1000000000000000000ULL

/* Operations on lengths of time */

/* An absolute length time */
typedef struct timelength {
	u64 sec;	/* Seconds */
	u64 asec;  	/* Fractional seconds expressed in attoseconds */
} timelength_t;

/* Convert a scalar value to a timelength */
static inline void scalar_to_timelength(timelength_t *tl, u64 t, u64 dv, u64 ml)
{
    /* Special case: time or divisor of zero */
    if (!t || !dv || !ml) {
    	if (tl) {
        	tl->sec  = 0;
        	tl->asec = 0;
    	}
        return;
    }

    /* Perform the calculation */
	tl->sec = div64_u64(t, dv);
	tl->asec = t - dv * (u64) tl->sec;
	tl->asec *= ml;
}

/* Add two lengths of time together */
static inline void timelength_add(timelength_t *l1, timelength_t *l2)
{
	u64 tm;
	l1->asec += l2->asec;
	l1->sec  += l2->sec;
	tm = div64_u64(l1->asec, aSEC_PER_SEC);
	l1->sec  += tm;
	l1->asec -= (tm * aSEC_PER_SEC);
}

/* Compare two timelengths: l1 < l2 => -1, l1 > l2 => 1, else 0 */
static inline int timelength_cmp(timelength_t *l1, timelength_t *l2)
{
	if (!l1 || !l2)
		return 0;
    if (l1->sec > l2->sec)
        return -1;
    if (l1->sec < l2->sec)
        return  1;
    if (l1->asec > l2->asec)
        return -1;
    if (l1->asec < l2->asec)
        return  1;
    return 0;
}

/* Copy the maximum value of two timelengths into a third timelength */
static inline void timelength_max(timelength_t *sol, timelength_t *l1,
    timelength_t *l2)
{
	if (!sol || !l1 || !l2)
		return;
    if (timelength_cmp(l1,l2) < 0)
        memcpy(sol,l1,sizeof(timelength_t));
    else
        memcpy(sol,l2,sizeof(timelength_t));
}

/* Copy the minimum value of two timelengths into a third timelength */
static inline void timelength_min(timelength_t *sol, timelength_t *l1,
    timelength_t *l2)
{
	if (!sol || !l1 || !l2)
		return;
    if (timelength_cmp(l1,l2) > 0)
        memcpy(sol,l1,sizeof(timelength_t));
    else
        memcpy(sol,l2,sizeof(timelength_t));
}

/* Initializers for time lengths */
#define  TL_FROM_SEC(d,t) scalar_to_timelength(&d,t, SEC_PER_SEC,aSEC_PER_SEC)
#define TL_FROM_mSEC(d,t) scalar_to_timelength(&d,t,mSEC_PER_SEC,fSEC_PER_SEC)
#define TL_FROM_uSEC(d,t) scalar_to_timelength(&d,t,uSEC_PER_SEC,pSEC_PER_SEC)
#define TL_FROM_nSEC(d,t) scalar_to_timelength(&d,t,nSEC_PER_SEC,nSEC_PER_SEC)
#define TL_FROM_pSEC(d,t) scalar_to_timelength(&d,t,pSEC_PER_SEC,uSEC_PER_SEC)
#define TL_FROM_fSEC(d,t) scalar_to_timelength(&d,t,fSEC_PER_SEC,mSEC_PER_SEC)
#define TL_FROM_aSEC(d,t) scalar_to_timelength(&d,t,aSEC_PER_SEC, SEC_PER_SEC)

/* Casts to scalars */
#define  TL_TO_GEN(d,f1,f2) (d.sec*(u64)f1)+div64_u64(d.asec,f2)
#define  TL_TO_SEC(d) TL_TO_GEN(d, SEC_PER_SEC,aSEC_PER_SEC)
#define TL_TO_mSEC(d) TL_TO_GEN(d,mSEC_PER_SEC,fSEC_PER_SEC)
#define TL_TO_uSEC(d) TL_TO_GEN(d,uSEC_PER_SEC,pSEC_PER_SEC)
#define TL_TO_nSEC(d) TL_TO_GEN(d,nSEC_PER_SEC,nSEC_PER_SEC)
#define TL_TO_pSEC(d) TL_TO_GEN(d,pSEC_PER_SEC,uSEC_PER_SEC)
#define TL_TO_fSEC(d) TL_TO_GEN(d,fSEC_PER_SEC,mSEC_PER_SEC)
#define TL_TO_aSEC(d) TL_TO_GEN(d,aSEC_PER_SEC, SEC_PER_SEC)

/* Operations on points of time */

/* A single point in time with respect to some reference */
typedef struct timepoint {
	s64 sec;	/* Seconds since reference */
	u64 asec;  	/* Fractional seconds expressed in attoseconds */
} timepoint_t;


/* Convert a scalar value to a timepoint */
static inline void scalar_to_timepoint(timepoint_t *tp, s64 t, u64 dv, u64 ml)
{
    u64 tm;
    int neg;

    /* Special case: time or divisor of zero */
    if (!t || !dv || !ml) {
    	if (tp) {
        	tp->sec  = 0;
        	tp->asec = 0;
    	}
        return;
    }

    /* Check whether the value is negative */
    neg = (t < 0);

    /* Absolute value of t */ //-> Check this out 
    #ifdef __KERNEL__
    if(t > 0LL)
    	tm = (u64) t;
    else
    	tm = (u64) (-1LL * t);
    //tm = (u64) abs(t);
    #else
    tm = (u64) llabs(t);
    #endif   

    /* Get number of seconds */
	tp->sec = div64_u64(tm, dv);
	tp->asec = tm - dv * (u64) tp->sec;
	#ifdef __KERNEL__

	//pr_info("Time t = %lld, tm = %llu, dv = %llu, sec = %lld, asec = %llu\n", t, tm, dv, tp->sec, tp->asec);
	#endif
	/* Are we dealing with a negative number */
	if (neg) {
		tp->sec = -tp->sec;
		tp->asec *= ml;
		/* Keep the definition of time as T = +/-sec + asec */
		if (tp->asec) {
			tp->sec--;
			tp->asec = aSEC_PER_SEC - tp->asec;
		}
	} else {
		tp->asec *= ml;
	}
}

/* Add a length of time to a point in time */
static inline void timepoint_add(timepoint_t *t, timelength_t *v)
{
	u64 tm;
	t->asec += v->asec;
	t->sec  += v->sec;
	tm = div64_u64(t->asec, aSEC_PER_SEC);
	t->sec  += tm;
	t->asec -= (tm * aSEC_PER_SEC);
}

/* Subtract a length of time from a point in time */
static inline void timepoint_sub(timepoint_t *t, timelength_t *v)
{
	t->sec -= v->sec;
	if (t->asec < v->asec) { /* Special case */
		t->sec--;
		t->asec = aSEC_PER_SEC - (v->asec - t->asec);
	} else {
		t->asec = t->asec - v->asec;
	}
}

/* Compare two timepoints: t1 > t2 => -1, t1 < t2 => 1, else 0 */
static inline int timepoint_cmp(timepoint_t *t1, timepoint_t *t2)
{
	if (!t1 || !t2)
		return 0;
    if (t1->sec > t2->sec)
        return -1;
    if (t1->sec < t2->sec)
        return  1;
    if (t1->asec > t2->asec)
        return -1;
    if (t1->asec < t2->asec)
        return  1;
    return 0;
}

/* Get the difference between two timepoints as a timelength */
static inline void timepoint_diff(timelength_t *v, timepoint_t *t1,
    timepoint_t *t2)
{
	if (!v || !t1 || !t2)
		return;
	v->sec = abs(t1->sec - t2->sec);
	if (t2->asec > t1->asec)
		v->asec = t2->asec - t1->asec;
	else
		v->asec = t1->asec - t2->asec;
}

/* Get the difference between two timepoints as a timelength */
static inline void timepoint_from_timespec(timepoint_t *t, struct timespec *ts)
{
	t->sec  = ts->tv_sec;
	t->asec = ts->tv_nsec*nSEC_PER_SEC;
}

/* Initializers for timepoints */
#define  TP_FROM_SEC(d,t) scalar_to_timepoint(&d,t, SEC_PER_SEC,aSEC_PER_SEC)
#define TP_FROM_mSEC(d,t) scalar_to_timepoint(&d,t,mSEC_PER_SEC,fSEC_PER_SEC)
#define TP_FROM_uSEC(d,t) scalar_to_timepoint(&d,t,uSEC_PER_SEC,pSEC_PER_SEC)
#define TP_FROM_nSEC(d,t) scalar_to_timepoint(&d,t,nSEC_PER_SEC,nSEC_PER_SEC)
#define TP_FROM_pSEC(d,t) scalar_to_timepoint(&d,t,pSEC_PER_SEC,uSEC_PER_SEC)
#define TP_FROM_fSEC(d,t) scalar_to_timepoint(&d,t,fSEC_PER_SEC,mSEC_PER_SEC)
#define TP_FROM_aSEC(d,t) scalar_to_timepoint(&d,t,aSEC_PER_SEC, SEC_PER_SEC)

/* Some casts for efficiency */
#define  TP_TO_GEN(d,f1,f2) (d.sec*f1)+div64_u64(d.asec,f2)
#define  TP_TO_SEC(d) TP_TO_GEN(d, SEC_PER_SEC,aSEC_PER_SEC)
#define TP_TO_mSEC(d) TP_TO_GEN(d,mSEC_PER_SEC,fSEC_PER_SEC)
#define TP_TO_uSEC(d) TP_TO_GEN(d,uSEC_PER_SEC,pSEC_PER_SEC)
#define TP_TO_nSEC(d) TP_TO_GEN(d,nSEC_PER_SEC,nSEC_PER_SEC)
#define TP_TO_pSEC(d) TP_TO_GEN(d,pSEC_PER_SEC,uSEC_PER_SEC)
#define TP_TO_fSEC(d) TP_TO_GEN(d,fSEC_PER_SEC,mSEC_PER_SEC)
#define TP_TO_aSEC(d) TP_TO_GEN(d,aSEC_PER_SEC, SEC_PER_SEC)

/* Operations on intervals of time and uncertainties */

/* An interval of time */
typedef struct timeinterval {
	timelength_t below;			/* Seconds below (-ve) */
	timelength_t above; 		/* Seconds above (+ve) */
} timeinterval_t;

/* An point of time with an interval of uncertainty */
typedef struct utimepoint {
	timepoint_t estimate;		/* Estimate of time */
	timeinterval_t interval;	/* Uncertainty interval around timepoint */
} utimepoint_t;

/* A duration of time with an uncertain end point */
typedef struct utimelength {
	timelength_t estimate;		/* Estimate of time */
	timeinterval_t interval;	/* Uncertainty interval around endpoint */
} utimelength_t;

/* A time quality comprised of a (min, max) accuracy and tick resolution  */
typedef struct timequality {
	timelength_t resolution;	/* Time resolution */
	timeinterval_t accuracy;	/* Time accuracy  */
} timequality_t;


/* Add a length of uncertain time to an uncertain point in time */
static inline void utimepoint_add(utimepoint_t *t, utimelength_t *v)
{
	timepoint_add(&t->estimate, &v->estimate);
	timelength_add(&t->interval.below, &v->interval.below);
	timelength_add(&t->interval.above, &v->interval.above);
}

/* Subtract a length of uncertain time from an uncertain point in time */
static inline void utimepoint_sub(utimepoint_t *t, utimelength_t *v)
{
	timepoint_sub(&t->estimate, &v->estimate);
	timelength_add(&t->interval.below, &v->interval.below);
	timelength_add(&t->interval.above, &v->interval.above);
}

/* Operations on frequencies */

/**
 * @brief Edge trigger codes from the QoT stack
 */
typedef enum {
	QOT_TRIGGER_NONE	= (0),		/* No edges	- Not for PWM				*/
	QOT_TRIGGER_RISING	= (1),		/* Rising edge only						*/
	QOT_TRIGGER_FALLING	= (2),		/* Falling edge only					*/
	QOT_TRIGGER_BOTH	= (3),		/* Both edges - Not for PWM				*/
} qot_trigger_t;

/**
 * @brief Response codes from the QoT stack
 */
typedef enum {
	QOT_RETURN_TYPE_OK   = (0),	/* Everything OK 						*/
	QOT_RETURN_TYPE_ERR  = (1),	/* There's been an error				*/
} qot_return_t;

/**
 * @brief Timeline events
 */
typedef enum {
	QOT_EVENT_TIMELINE_CREATE    = (0),	   /* Timeline created 		*/
	QOT_EVENT_CLOCK_CREATE       = (1),	   /* Clock created 		*/
	QOT_EVENT_EXTERNAL_TIMESTAMP = (2),	   /* External Timestamp    */
	QOT_EVENT_PWM_START          = (3),    /* PWM Started           */
	QOT_EVENT_TIMER_CALLBACK     = (4),    /* Timer Callback        */
} qot_event_type_t;

/**
 * @brief Timeline message types
 */
typedef enum {
	QOT_MSG_COORD_READY        = (0),    /* Node ready            */
	QOT_MSG_COORD_START        = (1),    /* Coordination Start    */
	QOT_MSG_COORD_STOP         = (2),    /* Coordination Stop     */
	QOT_MSG_SENSOR_VAL         = (3),    /* Sensor Value          */
	QOT_MSG_INVALID            = (4),    /* Invalid Message (used as a bound for the enum List) */
} qot_msg_type_t;

/**
 * @brief Clock error types
 */
typedef enum {
    QOT_CLK_ERR_BIAS  = (0),
    QOT_CLK_ERR_WALK,
    QOT_CLK_ERR_NUM
} qot_clk_err_t;

/**
 * @brief Clock error types
 */
typedef enum {
    QOT_CLK_STATE_ON  = (0),
    QOT_CLK_STATE_OFF,
} qot_clk_state_t;

/* QoT timeline type */
typedef struct qot_timeline {
    char name[QOT_MAX_NAMELEN];          /* Timeline name                     */
    int index;                           /* The integer Y in /dev/timelineY   */
    // #ifdef __KERNEL__
    // //Changes added by Sandeep -> Scheduler Specific Stuff
    // struct rb_root event_head;            RB tree head for events on this timeline 
    // raw_spinlock_t rb_lock;              /* RB tree spinlock */
    // #endif
} qot_timeline_t;

/* QoT wait until */
typedef struct  qot_sleeper {
	qot_timeline_t timeline;	        /* Timeline Information    */
	utimepoint_t wait_until_time;	    /* Uncertain time of event */
} qot_sleeper_t;

/* QoT Periodic Timer */
typedef struct qot_timer {
	qot_timeline_t timeline;	        /* Timeline Information    */
	timepoint_t start_offset;			/* Timer Start Offset      */
	timelength_t period;				/* Timer Period            */
	int count;                          /* Timer Iterations        */
} qot_timer_t;

/* QoT external input timestamping */
typedef struct qot_extts {
	int pin_index;          			/* Pin (according to testptp -l) */
	qot_trigger_t edge;					/* Edge to capture */
	qot_return_t response;			    /* Response */
	#ifdef __KERNEL__
	struct file *owner_file;            /* Owning File Descriptor  */
	#endif
} qot_extts_t;

/* QoT programmable periodic output */
typedef struct qot_perout {
	char pin[QOT_MAX_NAMELEN];			/* Pin (according to testptp -l) */
	qot_trigger_t edge;					/* Rising and falling only*/
	timepoint_t start;					/* Start time */
	timelength_t period;				/* Period */
	int duty_cycle;						/* Duty Cycle 1-100 */
	qot_return_t response;			    /* Response */
	qot_timeline_t timeline;			/* Timeline Data Structure */
	#ifdef __KERNEL__
	struct file *owner_file;            /* Owning File Descriptor  */
	#endif
} qot_perout_t;

/* QoT event */
typedef struct qot_event {
	qot_event_type_t type;				/* Event type */
	utimepoint_t timestamp;				/* Uncertain time of event */
	char data[QOT_MAX_NAMELEN];			/* Event data */
} qot_event_t;

/**
 * @brief Ioctl messages supported by /dev/qotusr
 */
#define QOTUSR_MAGIC_CODE  0xED
#define QOTUSR_GET_NEXT_EVENT     _IOR(QOTUSR_MAGIC_CODE, 1, qot_event_t*)
#define QOTUSR_GET_TIMELINE_INFO _IOWR(QOTUSR_MAGIC_CODE, 2, qot_timeline_t*)
#define QOTUSR_CREATE_TIMELINE   _IOWR(QOTUSR_MAGIC_CODE, 3, qot_timeline_t*)
#define QOTUSR_DESTROY_TIMELINE  _IOWR(QOTUSR_MAGIC_CODE, 4, qot_timeline_t*)
#define QOTUSR_WAIT_UNTIL       _IOWR(QOTUSR_MAGIC_CODE, 9, qot_sleeper_t*)
#define QOTUSR_OUTPUT_COMPARE_ENABLE       _IOWR(QOTUSR_MAGIC_CODE, 10, qot_perout_t*)
#define QOTUSR_OUTPUT_COMPARE_DISABLE       _IOWR(QOTUSR_MAGIC_CODE, 11, qot_perout_t*)
#define QOTUSR_GET_CORE_CLOCK_INFO     _IOR(QOTUSR_MAGIC_CODE, 12, qot_clock_t*)

/* QoT clock type (admin only) */
typedef struct qot_clock {
    char name[QOT_MAX_NAMELEN];         /* Clock name                   */
    qot_clk_state_t state;              /* Clock state                  */
    u64 nom_freq_nhz;           		/* Frequency in nHz             */
    u64 nom_freq_nwatt;   		        /* Power draw in nWatt          */
    utimelength_t read_latency;         /* Read latency                 */
    utimelength_t interrupt_latency;    /* Interrupt latency            */
    u64 errors[QOT_CLK_ERR_NUM];   		/* Error characteristics        */
    int phc_id;                         /* The integer X in /dev/ptpX   */
} qot_clock_t;

/**
 * @brief Key messages supported by /dev/qotadm
 */
#define QOTADM_MAGIC_CODE 0xEE
#define QOTADM_GET_NEXT_EVENT     _IOR(QOTADM_MAGIC_CODE, 1, qot_event_t*)
#define QOTADM_GET_CLOCK_INFO     _IOWR(QOTADM_MAGIC_CODE, 2, qot_clock_t*)
#define QOTADM_SET_CLOCK_SLEEP    _IOW(QOTADM_MAGIC_CODE, 3, qot_clock_t*)
#define QOTADM_SET_CLOCK_WAKE     _IOW(QOTADM_MAGIC_CODE, 4, qot_clock_t*)
#define QOTADM_SET_CLOCK_ACTIVE   _IOW(QOTADM_MAGIC_CODE, 5, qot_clock_t*)
#define QOTADM_SET_OS_LATENCY     _IOW(QOTADM_MAGIC_CODE, 6, utimelength_t*)
#define QOTADM_GET_OS_LATENCY     _IOR(QOTADM_MAGIC_CODE, 7, utimelength_t*)
#define QOTADM_GET_CORE_TIME_RAW  _IOR(QOTADM_MAGIC_CODE, 8, timepoint_t*)


/* QoT Binding type */
typedef struct qot_binding {
    char name[QOT_MAX_NAMELEN];          /* Application name */
    timequality_t demand;                /* Requested QoT */
    int id;                              /* Binding ID */
    /* Scheduling Parameters */
	timepoint_t start_offset;			 /* Start offset for periodic scheduling */
	timelength_t period;                 /* Scheduling Period */
} qot_binding_t;

/* QoT Message type */
typedef struct qot_message {
	char name[QOT_MAX_NAMELEN];          /* Application node name */
	qot_msg_type_t type;				 /* Message type */
	utimepoint_t timestamp;				 /* Uncertain timestamp associated with message*/
	char data[QOT_MAX_NAMELEN];			 /* Message data */
} qot_message_t;

/* Upper and lower bound on current time */
typedef struct qot_bounds {
	s64 u_drift; // Upper bound on drift
	s64 l_drift; // Lower bound on drift
	s64 u_nsec; // Upper bound on offset
	s64 l_nsec; // Lower bound on offset
} qot_bounds_t;

/* An point of time with an upper and lower bound time */
typedef struct stimepoint {
	timepoint_t estimate;		/* Estimate of time */
	timepoint_t u_estimate;		/* Upper bound on estimate of time */
	timepoint_t l_estimate;		/* Lower bound on estimate of time */
} stimepoint_t;

/* @brief Cluster Management Flags*/
typedef enum {
    QOT_NODE_JOINED  = (0),
    QOT_NODE_LEFT
} qot_node_status_t;

typedef struct qot_node {
	char name[QOT_MAX_NAMELEN];          /* Application node name     */
	long userID; 						 /* Unique userID of the node */
	qot_node_status_t status;            /* Status of the node        */
} qot_node_t;

// Callback Function Prototypes
typedef void (*qot_msg_callback_t)(const qot_message_t *msg);
typedef void (*qot_node_callback_t)(qot_node_t *node);

/**
 * @brief Key messages supported by /dev/timelineX
 */
#define TIMELINE_MAGIC_CODE 0xEF

#define TIMELINE_GET_INFO           	_IOR(TIMELINE_MAGIC_CODE, 1, qot_timeline_t*)
#define TIMELINE_GET_BINDING_INFO		_IOR(TIMELINE_MAGIC_CODE, 2, qot_binding_t*)
#define TIMELINE_BIND_JOIN         		_IOWR(TIMELINE_MAGIC_CODE, 3, qot_binding_t*)
#define TIMELINE_BIND_LEAVE         	_IOW(TIMELINE_MAGIC_CODE, 4, qot_binding_t*)
#define TIMELINE_BIND_UPDATE        	_IOW(TIMELINE_MAGIC_CODE, 5, qot_binding_t*)
#define TIMELINE_CORE_TO_REMOTE    		_IOWR(TIMELINE_MAGIC_CODE, 6, stimepoint_t*)
#define TIMELINE_REMOTE_TO_CORE    		_IOWR(TIMELINE_MAGIC_CODE, 7, timepoint_t*)
#define TIMELINE_GET_CORE_TIME_NOW      _IOR(TIMELINE_MAGIC_CODE, 8, utimepoint_t*)
#define TIMELINE_GET_TIME_NOW       	_IOR(TIMELINE_MAGIC_CODE, 9, utimepoint_t*)
#define TIMELINE_SET_SYNC_UNCERTAINTY   _IOR(TIMELINE_MAGIC_CODE, 10, qot_bounds_t*)
#define TIMELINE_CREATE_TIMER    		_IOWR(TIMELINE_MAGIC_CODE, 11, qot_timer_t*)
#define TIMELINE_DESTROY_TIMER    		_IOWR(TIMELINE_MAGIC_CODE, 12, qot_timer_t*)

#endif
