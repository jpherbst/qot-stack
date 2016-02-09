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

#ifndef QOT_STACK_SRC_QOT_H
#define QOT_STACK_SRC_QOT_H

/* So that we might expose a meaningful name through PTP interface */
#define QOT_MAX_NAMELEN 16

/* Cater for different C compilation pipelines (ie. no floats / doubles) */
#ifdef __KERNEL__
	#include <linux/ioctl.h>
#else
	#include <time.h>
	#include <stdint.h>
	#include <sys/ioctl.h>
#endif

/* A single point in time with respect to some reference */
typedef struct timepoint {
	 int64_t sec;	/* Seconds since reference */
	uint64_t asec;  /* Fractional seconds expressed in attoseconds */
} timepoint_t;

/* An absolute length time */
typedef struct timelength {
	uint64_t sec;	/* Seconds */
	uint64_t asec;  /* Fractional seconds expressed in attoseconds */
} timelength_t;

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

/* A duration of time with an uncertain end point */
typedef struct timedemand {
	timelength_t resolution;	/* Time resolution */
	timeinterval_t accuracy;	/* Time accuracy  */
} timedemand_t;

/* Popular ratios that will be used throughout this file */
#define MSEC_PER_SEC 1000ULL
#define USEC_PER_SEC 1000000ULL
#define PSEC_PER_SEC 1000000000ULL
#define NSEC_PER_SEC 1000000000000ULL
#define FSEC_PER_SEC 1000000000000000ULL
#define ASEC_PER_SEC 1000000000000000000ULL

/* Some initializers for efficiency */
#define WEEKS(t) { .sec=(t)*604800ULL, 	.asec=0, 						}
#define DAYS(t)  { .sec=(t)*86400ULL,  	.asec=0, 						}
#define HOURS(t) { .sec=(t)*3600ULL, 	.asec=0, 						}
#define MINS(t)  { .sec=(t)*60ULL, 	 	.asec=0, 						}
#define  SEC(t)  { .sec=(t), 			.asec=0, 						}
#define MSEC(t)  { .sec=0, 				.asec=(t)*1000000000000000ULL, 	}
#define USEC(t)  { .sec=0, 				.asec=(t)*1000000000000ULL, 	}
#define PSEC(t)  { .sec=0, 				.asec=(t)*1000000000ULL, 		}
#define NSEC(t)  { .sec=0, 				.asec=(t)*1000000ULL, 			}
#define FSEC(t)  { .sec=0, 				.asec=(t)*1000ULL, 				}
#define ASEC(t)  { .sec=0, 				.asec=(t), 						}

/* Add a length of time to a point in time */
void timepoint_add(timepoint_t *t, timelength_t *v) {
	t->asec += v->asec;
	t->sec  += v->sec + t->asec / ASEC_PER_SEC;
	t->asec %= ASEC_PER_SEC;
}

/* Subtract a length of time from a point in time */
void timepoint_sub(timepoint_t *t, timelength_t *v) {
	t->sec -= v->sec;
	if (t->asec < v->asec) { /* Special case */
		t->sec--;
		t->asec = ASEC_PER_SEC + t->asec - v->asec;
	}
	else
		t->asec = t->asec - v->asec;
}

/* Add two lengths of time together */
void timelength_add(timelength_t *l1, timelength_t *l2) {
	l1->asec += l2->asec;
	l1->sec  += l2->sec + l1->asec / ASEC_PER_SEC;
	l1->asec %= ASEC_PER_SEC;
}

/* Get the difference between two timepoints as a timelength */
void timepoint_diff(timelength_t *v,
	timepoint_t *t1, timepoint_t *t2) {
	v->sec = abs(t1->sec - t2->sec);
	if (t2->asec > t1->asec)
		v->asec = t2->asec - t1->asec;
	else
		v->asec = t1->asec - t2->asec;
}

/* Add a length of uncertain time to an uncertain point in time */
void utimepoint_add(utimepoint_t *t, utimelength_t *v) {
	timepoint_add(&t->estimate, &v->estimate);
	timelength_add(&t->interval.below, &v->interval.below);
	timelength_add(&t->interval.above, &v->interval.above);
}

/* Subtract a length of uncertain time from an uncertain point in time */
void utimepoint_sub(utimepoint_t *t, utimelength_t *v) {
	timepoint_sub(&t->estimate, &v->estimate);
	timelength_add(&t->interval.below, &v->interval.below);
	timelength_add(&t->interval.above, &v->interval.above);
}

/**
 * @brief Edge trigger codes from the QoT stack
 */
typedef enum {
	QOT_TRIGGER_NONE	= (0),		/* No edges								*/
	QOT_TRIGGER_RISING	= (1),		/* Rising edge only						*/
	QOT_TRIGGER_FALLING	= (2),		/* Falling edge only					*/
	QOT_TRIGGER_BOTH	= (3),		/* Both edges							*/
} qot_trigger_t;

/**
 * @brief Response codes from the QoT stack
 */
typedef enum {
	QOT_RESPONSE_OK   = (0),	/* Everything OK 						*/
	QOT_RESPONSE_ERR  = (1),	/* There's been an error				*/
} qot_return_t;

/**
 * @brief Timeline events
 */
typedef enum {
	QOT_TYPE_CREATE   = (0),	/* Timeline created 					*/
	QOT_TYPE_DESTROY  = (1),	/* Timeline destroyed 					*/
	QOT_TYPE_JOIN     = (2),	/* Peer joined timeline 				*/
	QOT_TYPE_LEAVE    = (3),	/* Peer left timeline 					*/
	QOT_TYPE_SYNC     = (4),	/* Timeline synchronization update 		*/
	QOT_TYPE_CAPTURE  = (5),	/* Capture event on this timeline  		*/
	QOT_TYPE_UDPATE   = (6),	/* Local timeline parameters updated 	*/
} qot_type_t;

/* QoT external input timestamping */
typedef struct qot_extts {
	char pin[QOT_MAX_NAMELEN];			/* Pin (according to testptp -l) */
	qot_trigger_t edge;					/* Edge to capture */
	qot_return_t response;			/* Response */
} qot_extts_t;

/* QoT programmable periodic output */
typedef struct qot_perout {
	char pin[QOT_MAX_NAMELEN];			/* Pin (according to testptp -l) */
	qot_trigger_t edge;					/* Off, rising, falling, toggle */
	timepoint_t start;					/* Start time */
	timelength_t period;				/* Period */
	qot_return_t response;			/* Response */
} qot_perout_t;

/* Binding request */
typedef struct qot_bind {
	char uuid[QOT_MAX_NAMELEN];			/* Timeline UUID */
	timedemand_t demand;				/* Quality of Time (QoT) demand */
	qot_return_t response;			/* Response */
} qot_bind_t;

/* QoT event */
typedef struct qot_event {
	qot_type_t type;					/* Event type */
	utimepoint_t timestamp;				/* Uncertain time of event */
	char data[QOT_MAX_NAMELEN];			/* Event data */
} qot_event_t;

/**
 * @brief Ioctl messages supported by /dev/qotusr
 */
#define QOTUSR_MAGIC_CODE  0xEE
#define QOTUSR_BIND       _IOWR(QOTUSR_MAGIC_CODE,  0, qot_bind_t*)
#define QOTUSR_UNBIND      _IOR(QOTUSR_MAGIC_CODE,  1, qot_return_t*)
#define QOTUSR_SET_DEMAND  _IOW(QOTUSR_MAGIC_CODE,  2, timedemand_t*)
#define QOTUSR_GET_DEMAND  _IOR(QOTUSR_MAGIC_CODE,  3, timedemand_t*)
#define QOTUSR_REQ_PEROUT  _IOW(QOTUSR_MAGIC_CODE,  4, qot_perout_t*)
#define QOTUSR_REQ_EXTTS   _IOW(QOTUSR_MAGIC_CODE,  5, qot_extts_t*)
#define QOTUSR_GET_EVENT   _IOR(QOTUSR_MAGIC_CODE,  6, qot_event_t*)

/**
 * @brief Key messages supported by /dev/qotadm
 */
#define QOTADM_MAGIC_CODE  0xEF
#define QOTADM_GET_DEMAND _IOWR(QOTADM_MAGIC_CODE,  0, qot_bind_t*)
#define QOTADM_SET_DEMAND  _IOW(QOTADM_MAGIC_CODE,  1, qot_bind_t*)
#define QOTADM_GET_EVENT   _IOR(QOTADM_MAGIC_CODE,  2, qot_event_t*)

#endif
