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
#define QOT_MAX_NAMELEN 64
#define QOT_MAX_NUMCLKS 8

/* For ease of conversion */
#define ASEC_PER_NSEC 1000000000ULL

/* Cater for different C compilation pipelines (ie. no floats / doubles) */
#ifdef __KERNEL__
	#include <linux/ioctl.h>
 	#include <linux/math64.h>
#else
	#include <time.h>
	#include <stdint.h>
	#include <sys/ioctl.h>
 	#define div_u64(X,Y) (uint64_t)X/(uint64_t)Y
 	#define u64 uint64_t
 	#define s64 int64_t
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

/* A time quality comprised of a (min, max) accuracy and tick resolution  */
typedef struct timequality {
	timelength_t resolution;	/* Time resolution */
	timeinterval_t accuracy;	/* Time accuracy  */
} timequality_t;

/* Popular ratios that will be used throughout this file */
#define aSEC_PER_SEC 1000000000000000000ULL

/* Some initializers for efficiency */
#define  SEC(t)  { .sec=(t), 			.asec=0, 						}
#define mSEC(t)  { .sec=0, 				.asec=(t)*1000000000000000ULL, 	}
#define uSEC(t)  { .sec=0, 				.asec=(t)*1000000000000ULL, 	}
#define pSEC(t)  { .sec=0, 				.asec=(t)*1000000000ULL, 		}
#define nSEC(t)  { .sec=0, 				.asec=(t)*1000000ULL, 			}
#define fSEC(t)  { .sec=0, 				.asec=(t)*1000ULL, 				}
#define aSEC(t)  { .sec=0, 				.asec=(t), 						}


/* Add a length of time to a point in time */
static inline void timepoint_add(timepoint_t *t, timelength_t *v) {
	t->asec += v->asec;
	t->sec  += v->sec + div_u64(t->asec,aSEC_PER_SEC);
	t->asec -= t->sec*aSEC_PER_SEC;
}

/* Subtract a length of time from a point in time */
static inline void timepoint_sub(timepoint_t *t, timelength_t *v) {
	t->sec -= v->sec;
	if (t->asec < v->asec) { /* Special case */
		t->sec--;
		t->asec = aSEC_PER_SEC + t->asec - v->asec;
	}
	else
		t->asec = t->asec - v->asec;
}

/* Add two lengths of time together */
static inline void timelength_add(timelength_t *l1, timelength_t *l2) {
	l1->asec += l2->asec;
	l1->sec  += l2->sec + div_u64(l1->asec, aSEC_PER_SEC);
	l1->asec -= l1->sec*aSEC_PER_SEC;
}

/* Compare two timelengths: l1 < l2 => -1, l1 > l2 => 1, else 0 */
static inline int timelength_cmp(timelength_t *l1, timelength_t *l2) {
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
static inline void timelength_max(timelength_t *sol,
    timelength_t *l1, timelength_t *l2) {
    if (timelength_cmp(l1,l2) < 0)
        memcpy(sol,l1,sizeof(timelength_t));
    else
        memcpy(sol,l2,sizeof(timelength_t));
}

/* Copy the minimum value of two timelengths into a third timelength */
static inline void timelength_min(timelength_t *sol,
    timelength_t *l1, timelength_t *l2) {
    if (timelength_cmp(l1,l2) > 0)
        memcpy(sol,l1,sizeof(timelength_t));
    else
        memcpy(sol,l2,sizeof(timelength_t));
}

/* Get the difference between two timepoints as a timelength */
static inline void timepoint_diff(timelength_t *v,
	timepoint_t *t1, timepoint_t *t2) {
	v->sec = abs(t1->sec - t2->sec);
	if (t2->asec > t1->asec)
		v->asec = t2->asec - t1->asec;
	else
		v->asec = t1->asec - t2->asec;
}

/* Add a length of uncertain time to an uncertain point in time */
static inline void utimepoint_add(utimepoint_t *t, utimelength_t *v) {
	timepoint_add(&t->estimate, &v->estimate);
	timelength_add(&t->interval.below, &v->interval.below);
	timelength_add(&t->interval.above, &v->interval.above);
}

/* Subtract a length of uncertain time from an uncertain point in time */
static inline void utimepoint_sub(utimepoint_t *t, utimelength_t *v) {
	timepoint_sub(&t->estimate, &v->estimate);
	timelength_add(&t->interval.below, &v->interval.below);
	timelength_add(&t->interval.above, &v->interval.above);
}

/* Popular ratios that will be used throughout this file */
#define aHZ_PER_Hz 1000000000000000000ULL

/* Some initializers for efficiency */
#define EHz(t) { .hz=(t)*1000000000000000000ULL,  .ahz=0, }
#define PHz(t) { .hz=(t)*1000000000000000ULL,  .ahz=0,    }
#define THz(t) { .hz=(t)*1000000000000ULL,  .ahz=0,       }
#define GHz(t) { .hz=(t)*1000000000ULL,   .ahz=0,         }
#define MHz(t) { .hz=(t)*1000000ULL,    .ahz=0,           }
#define KHz(t) { .hz=(t)*1000ULL,     .ahz=0,             }
#define  Hz(t) { .hz=(t),           .ahz=0,               }
#define mHz(t) { .hz=0, .ahz=(t)*1000000000000000ULL,     }
#define uHz(t) { .hz=0, .ahz=(t)*1000000000000ULL,        }
#define pHz(t) { .hz=0, .ahz=(t)*1000000000ULL,           }
#define nHz(t) { .hz=0, .ahz=(t)*1000000ULL,              }
#define fHz(t) { .hz=0, .ahz=(t)*1000ULL,                 }
#define aHz(t) { .hz=0, .ahz=(t),                         }


// SUPPORT FUNCTIONS ///////////////
/*Convert ns to timepoint_t and vice versa*/
static inline u64 timepoint_to_ns(timepoint_t expiry) {
	s64 ns = 0;//= expiry.sec*NSEC_PER_SEC + div_u64(expiry.asec, ASEC_PER_NSEC);
	return (u64)ns;
}

static inline timepoint_t ns_to_timepoint(u64 ns) {
     timepoint_t tp;
     tp.sec	= 0;//(int64_t) div_u64(ns, 1000000000ULL);
     tp.asec = 0;//(ns - tp.sec*1000000000ULL)*ASEC_PER_NSEC;
     return tp;
}
/* An absolute frequency */
typedef struct frequency {
    uint64_t hz;   /* Hertz */
    uint64_t ahz;  /* Fractional Hertz expressed in attohertz */
} frequency_t;


/* Add two frequencies together */
static inline void frequency_add(frequency_t *l1, frequency_t *l2) {
    l1->ahz += l2->ahz;
    l1->hz  += l2->hz + div_u64(l1->ahz,  aHZ_PER_Hz);
    l1->ahz -= l1->hz*aHZ_PER_Hz;
}

/* Popular ratios that will be used throughout this file */
#define aWATT_PER_WATT 1000000000000000000ULL

/* Some initializers for efficiency */
#define EWATT(t) { .watt=(t)*1000000000000000000ULL,  .awatt=0, }
#define PWATT(t) { .watt=(t)*1000000000000000ULL,  .awatt=0,    }
#define TWATT(t) { .watt=(t)*1000000000000ULL,  .awatt=0,       }
#define GWATT(t) { .watt=(t)*1000000000ULL,   .awatt=0,         }
#define MWATT(t) { .watt=(t)*1000000ULL,    .awatt=0,           }
#define KWATT(t) { .watt=(t)*1000ULL,     .awatt=0,             }
#define  WATT(t) { .watt=(t),           .awatt=0,               }
#define mWATT(t) { .watt=0, .awatt=(t)*1000000000000000ULL,     }
#define uWATT(t) { .watt=0, .awatt=(t)*1000000000000ULL,        }
#define pWATT(t) { .watt=0, .awatt=(t)*1000000000ULL,           }
#define nWATT(t) { .watt=0, .awatt=(t)*1000000ULL,              }
#define fWATT(t) { .watt=0, .awatt=(t)*1000ULL,                 }
#define aWATT(t) { .watt=0, .awatt=(t),                         }

/* An absolute frequency */
typedef struct power {
    uint64_t watt;   /* Watts */
    uint64_t awatt;  /* Fractional Watts expressed in attowatt */
} power_t;

/* Add two frequencies together */
static inline void power_add(power_t *l1, power_t *l2) {
    l1->awatt += l2->awatt;
    l1->watt  += l2->watt + div_u64(l1->awatt, aWATT_PER_WATT);
    l1->awatt -= l1->watt*aWATT_PER_WATT;
}

/**
 * @brief Scalar type used by the system
 **/
typedef int64_t scalar_t;

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
	QOT_RETURN_TYPE_OK   = (0),	/* Everything OK 						*/
	QOT_RETURN_TYPE_ERR  = (1),	/* There's been an error				*/
} qot_return_t;

/**
 * @brief Timeline events
 */
typedef enum {
	QOT_EVENT_TIMELINE_CREATE   = (0),	/* Timeline created 			 */
	QOT_EVENT_CLOCK_CREATE      = (1),	/* Clock created 				 */
} qot_event_type_t;

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
	qot_return_t response;			    /* Response */
} qot_perout_t;

/* QoT event */
typedef struct qot_event {
	qot_event_type_t type;				/* Event type */
	utimepoint_t timestamp;				/* Uncertain time of event */
	char data[QOT_MAX_NAMELEN];			/* Event data */
} qot_event_t;

/* QoT timeline type */
typedef struct qot_timeline {
    char name[QOT_MAX_NAMELEN];          /* Timeline name          */
    int index;                           /* The integer Y in /dev/timelineY */
} qot_timeline_t;

/* QoT timeline type */
typedef struct qot_binding {
    timequality_t demand;                /* Requested QoT          */
} qot_binding_t;

/**
 * @brief Ioctl messages supported by /dev/qotusr
 */
#define QOTUSR_MAGIC_CODE  0xEE
#define QOTUSR_GET_NEXT_EVENT     _IOR(QOTUSR_MAGIC_CODE, 1, qot_event_t*)
#define QOTUSR_GET_TIMELINE_INFO _IOWR(QOTUSR_MAGIC_CODE, 2, qot_timeline_t*)
#define QOTUSR_CREATE_TIMELINE   _IOWR(QOTUSR_MAGIC_CODE, 3, qot_timeline_t*)

/* QoT clock type (admin only) */
typedef struct qot_clock {
    char name[QOT_MAX_NAMELEN];         /* Clock name                   */
    qot_clk_state_t state;              /* Clock state                  */
    frequency_t nominal_freq;           /* Frequency in Hz              */
    power_t nominal_power;              /* Power draw in Watts          */
    timeinterval_t read_latency;        /* Latency in seconds           */
    timeinterval_t interrupt_latency;   /* Interrupt latency            */
    scalar_t errors[QOT_CLK_ERR_NUM];   /* Error characteristics        */
    int phc_id;                         /* The integer X in /dev/ptpX   */
} qot_clock_t;

/**
 * @brief Key messages supported by /dev/qotadm
 */
#define QOTADM_MAGIC_CODE 0xEF
#define QOTADM_GET_NEXT_EVENT     _IOR(QOTADM_MAGIC_CODE, 1, qot_event_t*)
#define QOTADM_GET_CLOCK_INFO    _IOWR(QOTADM_MAGIC_CODE, 2, qot_clock_t*)
#define QOTADM_SET_CLOCK_SLEEP    _IOW(QOTADM_MAGIC_CODE, 3, qot_clock_t*)
#define QOTADM_SET_CLOCK_WAKE     _IOW(QOTADM_MAGIC_CODE, 4, qot_clock_t*)
#define QOTADM_SET_CLOCK_ACTIVE   _IOW(QOTADM_MAGIC_CODE, 5, qot_clock_t*)

#endif
