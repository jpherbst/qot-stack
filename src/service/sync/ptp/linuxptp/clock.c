/**
 * @file clock.c
 * @note Copyright (C) 2011 Richard Cochran <richardcochran@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Modified Code for Quality of Time (QoT) Stack
 *
 */
#include <errno.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/queue.h>
#include <math.h>
#include <fcntl.h>

#include "address.h"
#include "bmc.h"
#include "clock.h"
#include "clockadj.h"
#include "clockcheck.h"
#include "foreign.h"
#include "filter.h"
#include "missing.h"
#include "msg.h"
#include "phc.h"
#include "port.h"
#include "servo.h"
#include "stats.h"
#include "print.h"
#include "tlv.h"
#include "uds.h"
#include "util.h"

#define N_CLOCK_PFD (N_POLLFD + 1) /* one extra per port, for the fault timer */
#define POW2_41 ((double)(1ULL << 41))

#define MAX_SAMPLES 4 /* QOT, number of points needed for uncertainty calculation */
#define SQUARE(x) (x*x)

struct port {
	LIST_ENTRY(port) list;
};

struct freq_estimator {
	tmv_t origin1;
	tmv_t ingress1;
	unsigned int max_count;
	unsigned int count;
};

struct clock_stats {
	struct stats *offset;
	struct stats *freq;
	struct stats *delay;
	unsigned int max_count;
};

/* QoT_start, uncertainty calculation data structures */
struct uncertainty_stats{
	uint64_t disp;		// error in measurement used to calculate peer-dispersion
	int64_t offset;		// offsets stored to calculate jitter
	uint64_t delay;		// one-way propagation delay, lesser the delay more accurate the offset value is
};

struct clock_uncertainty_stats {
	struct uncertainty_stats u_stats[MAX_SAMPLES];	// uncertainty stats
	unsigned int num_points;						// number of stored points
	unsigned int last_point;						// index of newest point
};

struct Interval{ // Interval for Marzullo's algorithm
  long long offset, low, up, dist; 
};

struct Point{
  int type;
  long long offset;
};

struct Bound{
  long long up, low;
};

struct servo {
        LIST_ENTRY(servo) list; 				/* QOT */
        int tml_clkid; 							/* timeline clock id */       
        struct clock_uncertainty_stats cu_stats; /* QOT, stats to calculate uncertainty in time */
		uint64_t local_ts; 						/* QOT, store the last ingress timestamp */
		double ppb;								/* QOT, store the last freq correction */

        double max_frequency;
        double step_threshold;
        double first_step_threshold;
        int first_update;

        void (*destroy)(struct servo *servo);

        double (*sample)(struct servo *servo,
                         int64_t offset, uint64_t local_ts,
                         enum servo_state *state);

        void (*sync_interval)(struct servo *servo, double interval);

        void (*reset)(struct servo *servo);

        double (*rate_ratio)(struct servo *servo);

        void (*leap)(struct servo *servo, int leap);
};

/* QoT_end */

struct clock_subscriber {
	LIST_ENTRY(clock_subscriber) list;
	uint8_t events[EVENT_BITMASK_CNT];
	struct PortIdentity targetPortIdentity;
	struct address addr;
	UInteger16 sequenceId;
	time_t expiration;
};

struct clock {
	clockid_t clkid;
	clockid_t core_clkid; /* QOT */
	struct servo *servo;
	enum servo_type servo_type;
	struct defaultDS dds;
	struct dataset default_dataset;
	struct currentDS cur;
	struct parent_ds dad;
	struct timePropertiesDS tds;
	struct ClockIdentity ptl[PATH_TRACE_MAX];
	struct foreign_clock *best;
	struct ClockIdentity best_id;
	LIST_HEAD(ports_head, port) ports;
	struct port *uds_port;
	struct pollfd *pollfd;
	int pollfd_valid;
	int nports; /* does not include the UDS port */
	int last_port_number;
	int free_running;
	int freq_est_interval;
	int grand_master_capable; /* for 802.1AS only */
	int utc_timescale;
	int utc_offset_set;
	int leap_set;
	int kernel_leap;
	int utc_offset;  /* grand master role */
	int time_flags;  /* grand master role */
	int time_source; /* grand master role */
	int	sw_ts; /* software timestamping, QOT */
	enum servo_state servo_state;
	tmv_t master_offset;
	tmv_t path_delay;
	struct filter *delay_filter;
	struct freq_estimator fest;
	struct time_status_np status;
	double nrr;
	tmv_t c1;
	tmv_t c2;
	tmv_t t1;
	tmv_t t2;
	struct clock_description desc;
	struct clock_stats stats;
	int stats_interval;
	struct clockcheck *sanity_check;
	struct interface uds_interface;
	LIST_HEAD(clock_subscribers_head, clock_subscriber) subscribers;
	LIST_HEAD(tmls_servo_head, servo) tmls_servos; /* QOT */
	int timelineid; /* QOT */
	int *timelinesfd; /* QOT */
	int timelines_size; /* QOT */
	double off_stddev; /* QOT */
	double freq_stddev; /* QOT */
};

//struct clock the_clock;
struct clock global_clocks[4];

static void handle_state_decision_event(struct clock *c);
static int clock_resize_pollfd(struct clock *c, int new_nports);
static void clock_remove_port(struct clock *c, struct port *p);

static int cid_eq(struct ClockIdentity *a, struct ClockIdentity *b)
{
	return 0 == memcmp(a, b, sizeof(*a));
}

#ifndef LIST_FOREACH_SAFE
#define	LIST_FOREACH_SAFE(var, head, field, tvar)			\
	for ((var) = LIST_FIRST((head));				\
	    (var) && ((tvar) = LIST_NEXT((var), field), 1);		\
	    (var) = (tvar))
#endif

/* QoT_start */

static void add_clock_uncertainty_sample(struct clock_uncertainty_stats *u, uint64_t disp, int64_t offset, uint64_t delay)
{
	u->last_point = (u->last_point + 1) % MAX_SAMPLES;
	
	u->u_stats[u->last_point].disp = disp;
	u->u_stats[u->last_point].offset = offset;
	u->u_stats[u->last_point].delay = delay;

	if(u->num_points < MAX_SAMPLES)
		u->num_points++;
}

static void destroy_timelines_servos(struct clock *c)
{
	struct servo *s, *tmp;
	LIST_FOREACH_SAFE(s, &c->tmls_servos, list, tmp) {
		LIST_REMOVE(s, list);
		servo_destroy(s);
	}
}

static int create_timelines_servos(struct clock *c, int sw_ts, enum servo_type servo)
{
	int i, tml_id, fadj = 0 , max_adj = 0;

	for (i = 0; i < c->timelines_size; i++) {
		tml_id = FD_TO_CLOCKID(c->timelinesfd[i]);

		/*if (tml_id != CLOCK_INVALID) {
			fadj = (int) clockadj_get_freq(tml_id);
			/* Due to a bug in older kernels, the reading may silently fail
			and return 0. Set the frequency back to make sure fadj is
			the actual frequency of the clock. */
			/*clockadj_set_freq(tml_id, fadj);
		}*/
		//TODO: Get max_adj value through core
		//max_adj = phc_max_adj(tml_id);
		//if (!max_adj) {
			//pr_err("clock is not adjustable");
			//return -1;
		max_adj = 1000000;
		//}
		//pr_info("clock, max_adj: %d", max_adj);

		struct servo *s, *piter, *lasts = NULL;
		s = servo_create(servo, -fadj, max_adj, sw_ts);

		if (!s) {
			pr_err("Failed to create timeline%d servo", i);
			return -1;
		}
		s->tml_clkid = tml_id;
		s->cu_stats.num_points = 0;
		//s->cu_stats.u_stats = calloc(1, sizeof *uncertainty_stats);

		LIST_FOREACH(piter, &c->tmls_servos, list)
		lasts = piter;
		if (lasts)
			LIST_INSERT_AFTER(lasts, s, list);
		else
			LIST_INSERT_HEAD(&c->tmls_servos, s, list);
	}
	return 0;
}

static void timelines_servos_sync_interval(struct clock *c, double interval)
{
	struct servo *s;
	LIST_FOREACH(s, &c->tmls_servos, list){
		servo_sync_interval(s, interval);
	}       
}

static void filter(struct clock_uncertainty_stats *u, struct Interval *interv_list, int total) 
{
  // ========== sort response according to delay from min to max =========
  int i, j, i_min;
  long long min_d;
  struct uncertainty_stats u_tem;
  for(i = 0; i < total; i++){
    for(j = i; j < total; j++){ // find min delay
      if(j == i) { // init
	       min_d = u->u_stats[j].delay;
	       i_min = j;
      } else if(min_d > u->u_stats[j].delay) {
	       min_d = u->u_stats[j].delay;
	       i_min = j;
      }
    }
    // === exchange i_min and i ===
    u_tem = u->u_stats[i];
    u->u_stats[i] = u->u_stats[i_min];
    u->u_stats[i_min] = u_tem;
  }
  
  // ======== filtering ========
  long long peer_disp = 0;
  long long sum = 0;
  long long diff;
  for(i = 0; i < total; i++) {
    // calculate peer dispersion
    peer_disp += (u->u_stats[i].disp >> (i+1));
    // calculate offset jitter
    diff = u->u_stats[i].offset - u->u_stats[0].offset;
    sum += SQUARE(diff);
  }
  long long jitter = (long long) sqrt(sum/total);
  //todo: fix it
  //long long jitter = (long long) (sum/total);
  
#ifdef DEBUG
  printf("peer_disp: %lld\n", peer_disp);
  printf("jitter: %lld\n", jitter);
#endif

  // calculate root distance
  long long root_dist;
  struct Interval interval;
  for(i = 0; i < total; i++) {
    root_dist = u->u_stats[i].delay + peer_disp + jitter;
    interval.offset = u->u_stats[i].offset;
    interval.dist = root_dist;
    interval.up = u->u_stats[i].offset + root_dist;
    interval.low = u->u_stats[i].offset - root_dist;
    *(interv_list + i) = interval;
  }
}

static int select_clock(struct Interval *interv_list, int total, struct Bound *bound)
{
  const int total_point = total*3;
  struct Point point_list[total_point];
  int i, j;
  for(i = 0; i < total; i++){
    j = i*3;
    point_list[j].type = 0; // low
    point_list[j].offset = (*(interv_list + i)).low;
    point_list[j+1].type = 1; // mean
    point_list[j+1].offset = (*(interv_list + i)).offset;
    point_list[j+2].type = 2; // high
    point_list[j+2].offset = (*(interv_list + i)).up;
  }
  // ==== sort point according to offset ====
  struct Point point_tem;
  long long min_d;
  int i_min;
  for(i = 0; i < total_point; i++){
    for(j = i; j < total_point; j++){ // find min
      if(j == i) { // init
	min_d = point_list[j].offset;
	i_min = j;
      } else if(min_d > point_list[j].offset) {
	min_d = point_list[j].offset;
	i_min = j;
      }
    }
    point_tem = point_list[i];
    point_list[i] = point_list[i_min];
    point_list[i_min] = point_tem;
  }
  
#ifdef DEBUG
  printf("\n");
  for(i = 0; i < 3*total; i++){
    printf("point: %d, %lld\n", point_list[i].type, point_list[i].offset);
  }
#endif
  
  int m = total, f = 0, d = 0, c = 0; 
  long long l = 0, u = 0;
  struct Point *p;
  while(1){
    d = 0; c = 0;
    for(i = 0; i < total_point; i++){
      p = &point_list[i];
      switch(p->type){
        case 0: c++; break;
        case 1: d++; break;
        case 2: c--; break;
        default: printf("error: invalid point\n"); exit(1);
      }

      if(c >= m - f){
	       l = p->offset; break;
      }
    }
    
    c = 0;
    for(i = total_point - 1; i >= 0; i--){
      p = &point_list[i];
      switch(p->type){
        case 2: c++; break;
        case 1: d++; break;
        case 0: c--; break;
        default: printf("error: invalid point\n"); exit(1);
      }
      
      if(c >= m - f){
	       u = p->offset; break;
      }
    }
    
    if(d <= f && l < u) break;
    else {
      f++;
      if(f >= m/2) return -1; // selection failure
    }
  }

  bound->low = l; 
  bound->up = u;
    
  return 0;
}

static int update_clock_uncertainty(struct clock_uncertainty_stats *u, struct Bound *bound){
	if(u->num_points < MAX_SAMPLES)
		return -1;

	struct Interval interv_list[u->num_points];

	// convert stats to intervals
	filter(u, &interv_list[u->num_points], u->num_points);

	// apply Marzullo's algorithm to select the tightest interval
	if(select_clock(interv_list, u->num_points, bound)){
    	printf("no valid bound\n");
    	return -1;
  	}
  	return 0;
}
/* QOT_end */

static void remove_subscriber(struct clock_subscriber *s)
{
	LIST_REMOVE(s, list);
	free(s);
}

static void clock_update_subscription(struct clock *c, struct ptp_message *req,
				      uint8_t *bitmask, uint16_t duration)
{
	struct clock_subscriber *s;
	int i, remove = 1;
	struct timespec now;

	for (i = 0; i < EVENT_BITMASK_CNT; i++) {
		if (bitmask[i]) {
			remove = 0;
			break;
		}
	}

	LIST_FOREACH(s, &c->subscribers, list) {
		if (!memcmp(&s->targetPortIdentity, &req->header.sourcePortIdentity,
		            sizeof(struct PortIdentity))) {
			/* Found, update the transport address and event
			 * mask. */
			if (!remove) {
				s->addr = req->address;
				memcpy(s->events, bitmask, EVENT_BITMASK_CNT);
				clock_gettime(CLOCK_MONOTONIC, &now);
				s->expiration = now.tv_sec + duration;
			} else {
				remove_subscriber(s);
			}
			return;
		}
	}
	if (remove)
		return;
	/* Not present yet, add the subscriber. */
	s = malloc(sizeof(*s));
	if (!s) {
		pr_err("failed to allocate memory for a subscriber");
		return;
	}
	s->targetPortIdentity = req->header.sourcePortIdentity;
	s->addr = req->address;
	memcpy(s->events, bitmask, EVENT_BITMASK_CNT);
	clock_gettime(CLOCK_MONOTONIC, &now);
	s->expiration = now.tv_sec + duration;
	s->sequenceId = 0;
	LIST_INSERT_HEAD(&c->subscribers, s, list);
}

static void clock_get_subscription(struct clock *c, struct ptp_message *req,
				   uint8_t *bitmask, uint16_t *duration)
{
	struct clock_subscriber *s;
	struct timespec now;

	LIST_FOREACH(s, &c->subscribers, list) {
		if (!memcmp(&s->targetPortIdentity, &req->header.sourcePortIdentity,
			    sizeof(struct PortIdentity))) {
			memcpy(bitmask, s->events, EVENT_BITMASK_CNT);
			clock_gettime(CLOCK_MONOTONIC, &now);
			if (s->expiration < now.tv_sec)
				*duration = 0;
			else
				*duration = s->expiration - now.tv_sec;
			return;
		}
	}
	/* A client without entry means the client has no subscriptions. */
	memset(bitmask, 0, EVENT_BITMASK_CNT);
	*duration = 0;
}

static void clock_flush_subscriptions(struct clock *c)
{
	struct clock_subscriber *s, *tmp;

	LIST_FOREACH_SAFE(s, &c->subscribers, list, tmp) {
		remove_subscriber(s);
	}
}

static void clock_prune_subscriptions(struct clock *c)
{
	struct clock_subscriber *s, *tmp;
	struct timespec now;

	clock_gettime(CLOCK_MONOTONIC, &now);
	LIST_FOREACH_SAFE(s, &c->subscribers, list, tmp) {
		if (s->expiration <= now.tv_sec) {
			pr_info("subscriber %s timed out",
				pid2str(&s->targetPortIdentity));
			remove_subscriber(s);
		}
	}
}

void clock_send_notification(struct clock *c, struct ptp_message *msg,
			     int msglen, enum notification event)
{
	unsigned int event_pos = event / 8;
	uint8_t mask = 1 << (event % 8);
	struct port *uds = c->uds_port;
	struct clock_subscriber *s;

	LIST_FOREACH(s, &c->subscribers, list) {
		if (!(s->events[event_pos] & mask))
			continue;
		/* send event */
		msg->header.sequenceId = htons(s->sequenceId);
		s->sequenceId++;
		msg->management.targetPortIdentity.clockIdentity =
			s->targetPortIdentity.clockIdentity;
		msg->management.targetPortIdentity.portNumber =
			htons(s->targetPortIdentity.portNumber);
		msg->address = s->addr;
		port_forward_to(uds, msg);
	}
}

void clock_destroy(struct clock *c)
{
	struct port *p, *tmp;

	clock_flush_subscriptions(c);
	LIST_FOREACH_SAFE(p, &c->ports, list, tmp) {
		clock_remove_port(c, p);
	}
	port_close(c->uds_port);
	free(c->pollfd);
	if (c->clkid != CLOCK_REALTIME) {
		phc_close(c->clkid);
	}
	//servo_destroy(c->servo);
	/* QOT, destroying servos for all timelines */
	destroy_timelines_servos(c);
	filter_destroy(c->delay_filter);
	stats_destroy(c->stats.offset);
	stats_destroy(c->stats.freq);
	stats_destroy(c->stats.delay);
	if (c->sanity_check)
		clockcheck_destroy(c->sanity_check);
	memset(c, 0, sizeof(*c));
	msg_cleanup();
}

static int clock_fault_timeout(struct port *port, int set)
{
	struct fault_interval i;

	if (!set) {
		pr_debug("clearing fault on port %d", port_number(port));
		return port_set_fault_timer_lin(port, 0);
	}

	fault_interval(port, last_fault_type(port), &i);

	if (i.type == FTMO_LINEAR_SECONDS) {
		pr_debug("waiting %d seconds to clear fault on port %d",
			 i.val, port_number(port));
		return port_set_fault_timer_lin(port, i.val);
	} else if (i.type == FTMO_LOG2_SECONDS) {
		pr_debug("waiting 2^{%d} seconds to clear fault on port %d",
			 i.val, port_number(port));
		return port_set_fault_timer_log(port, 1, i.val);
	}

	pr_err("Unsupported fault interval type %d", i.type);
	return -1;
}

static void clock_freq_est_reset(struct clock *c)
{
	c->fest.origin1 = tmv_zero();
	c->fest.ingress1 = tmv_zero();
	c->fest.count = 0;
};

static void clock_management_send_error(struct port *p,
					struct ptp_message *msg, int error_id)
{
	if (port_management_error(port_identity(p), p, msg, error_id))
		pr_err("failed to send management error status");
}

/* The 'p' and 'req' paremeters are needed for the GET actions that operate
 * on per-client datasets. If such actions do not apply to the caller, it is
 * allowed to pass both of them as NULL.
 */
static int clock_management_fill_response(struct clock *c, struct port *p,
					  struct ptp_message *req,
					  struct ptp_message *rsp, int id)
{
	int datalen = 0, respond = 0;
	struct management_tlv *tlv;
	struct management_tlv_datum *mtd;
	struct time_status_np *tsn;
	struct grandmaster_settings_np *gsn;
	struct subscribe_events_np *sen;
	struct PTPText *text;

	tlv = (struct management_tlv *) rsp->management.suffix;
	tlv->type = TLV_MANAGEMENT;
	tlv->id = id;

	switch (id) {
	case TLV_USER_DESCRIPTION:
		text = (struct PTPText *) tlv->data;
		text->length = c->desc.userDescription.length;
		memcpy(text->text, c->desc.userDescription.text, text->length);
		datalen = 1 + text->length;
		respond = 1;
		break;
	case TLV_DEFAULT_DATA_SET:
		memcpy(tlv->data, &c->dds, sizeof(c->dds));
		datalen = sizeof(c->dds);
		respond = 1;
		break;
	case TLV_CURRENT_DATA_SET:
		memcpy(tlv->data, &c->cur, sizeof(c->cur));
		datalen = sizeof(c->cur);
		respond = 1;
		break;
	case TLV_PARENT_DATA_SET:
		memcpy(tlv->data, &c->dad.pds, sizeof(c->dad.pds));
		datalen = sizeof(c->dad.pds);
		respond = 1;
		break;
	case TLV_TIME_PROPERTIES_DATA_SET:
		memcpy(tlv->data, &c->tds, sizeof(c->tds));
		datalen = sizeof(c->tds);
		respond = 1;
		break;
	case TLV_PRIORITY1:
		mtd = (struct management_tlv_datum *) tlv->data;
		mtd->val = c->dds.priority1;
		datalen = sizeof(*mtd);
		respond = 1;
		break;
	case TLV_PRIORITY2:
		mtd = (struct management_tlv_datum *) tlv->data;
		mtd->val = c->dds.priority2;
		datalen = sizeof(*mtd);
		respond = 1;
		break;
	case TLV_DOMAIN:
		mtd = (struct management_tlv_datum *) tlv->data;
		mtd->val = c->dds.domainNumber;
		datalen = sizeof(*mtd);
		respond = 1;
		break;
	case TLV_SLAVE_ONLY:
		mtd = (struct management_tlv_datum *) tlv->data;
		mtd->val = c->dds.flags & DDS_SLAVE_ONLY;
		datalen = sizeof(*mtd);
		respond = 1;
		break;
	case TLV_CLOCK_ACCURACY:
		mtd = (struct management_tlv_datum *) tlv->data;
		mtd->val = c->dds.clockQuality.clockAccuracy;
		datalen = sizeof(*mtd);
		respond = 1;
		break;
	case TLV_TRACEABILITY_PROPERTIES:
		mtd = (struct management_tlv_datum *) tlv->data;
		mtd->val = c->tds.flags & (TIME_TRACEABLE|FREQ_TRACEABLE);
		datalen = sizeof(*mtd);
		respond = 1;
		break;
	case TLV_TIMESCALE_PROPERTIES:
		mtd = (struct management_tlv_datum *) tlv->data;
		mtd->val = c->tds.flags & PTP_TIMESCALE;
		datalen = sizeof(*mtd);
		respond = 1;
		break;
	case TLV_TIME_STATUS_NP:
		tsn = (struct time_status_np *) tlv->data;
		tsn->master_offset = c->master_offset;
		tsn->ingress_time = tmv_to_nanoseconds(c->t2);
		tsn->cumulativeScaledRateOffset =
			(Integer32) (c->status.cumulativeScaledRateOffset +
				      c->nrr * POW2_41 - POW2_41);
		tsn->scaledLastGmPhaseChange = c->status.scaledLastGmPhaseChange;
		tsn->gmTimeBaseIndicator = c->status.gmTimeBaseIndicator;
		tsn->lastGmPhaseChange = c->status.lastGmPhaseChange;
		if (cid_eq(&c->dad.pds.grandmasterIdentity, &c->dds.clockIdentity))
			tsn->gmPresent = 0;
		else
			tsn->gmPresent = 1;
		tsn->gmIdentity = c->dad.pds.grandmasterIdentity;
		datalen = sizeof(*tsn);
		respond = 1;
		break;
	case TLV_GRANDMASTER_SETTINGS_NP:
		gsn = (struct grandmaster_settings_np *) tlv->data;
		gsn->clockQuality = c->dds.clockQuality;
		gsn->utc_offset = c->utc_offset;
		gsn->time_flags = c->time_flags;
		gsn->time_source = c->time_source;
		datalen = sizeof(*gsn);
		respond = 1;
		break;
	case TLV_SUBSCRIBE_EVENTS_NP:
		if (p != c->uds_port) {
			/* Only the UDS port allowed. */
			break;
		}
		sen = (struct subscribe_events_np *)tlv->data;
		clock_get_subscription(c, req, sen->bitmask, &sen->duration);
		respond = 1;
		break;
	}
	if (respond) {
		if (datalen % 2) {
			tlv->data[datalen] = 0;
			datalen++;
		}
		tlv->length = sizeof(tlv->id) + datalen;
		rsp->header.messageLength += sizeof(*tlv) + datalen;
		rsp->tlv_count = 1;
	}
	return respond;
}

static int clock_management_get_response(struct clock *c, struct port *p,
					 int id, struct ptp_message *req)
{
	struct PortIdentity pid = port_identity(p);
	struct ptp_message *rsp;
	int respond;

	rsp = port_management_reply(pid, p, req);
	if (!rsp) {
		return 0;
	}
	respond = clock_management_fill_response(c, p, req, rsp, id);
	if (respond)
		port_prepare_and_send(p, rsp, 0);
	msg_put(rsp);
	return respond;
}

static int clock_management_set(struct clock *c, struct port *p,
				int id, struct ptp_message *req, int *changed)
{
	int respond = 0;
	struct management_tlv *tlv;
	struct grandmaster_settings_np *gsn;
	struct subscribe_events_np *sen;

	tlv = (struct management_tlv *) req->management.suffix;

	switch (id) {
	case TLV_GRANDMASTER_SETTINGS_NP:
		gsn = (struct grandmaster_settings_np *) tlv->data;
		c->dds.clockQuality = gsn->clockQuality;
		c->utc_offset = gsn->utc_offset;
		c->time_flags = gsn->time_flags;
		c->time_source = gsn->time_source;
		*changed = 1;
		respond = 1;
		break;
	case TLV_SUBSCRIBE_EVENTS_NP:
		sen = (struct subscribe_events_np *)tlv->data;
		clock_update_subscription(c, req, sen->bitmask,
					  sen->duration);
		respond = 1;
		break;
	}
	if (respond && !clock_management_get_response(c, p, id, req))
		pr_err("failed to send management set response");
	return respond ? 1 : 0;
}

static void clock_stats_update(struct clock_stats *s,
			       int64_t offset, double freq)
{
	struct stats_result offset_stats, freq_stats, delay_stats;

	stats_add_value(s->offset, offset);
	stats_add_value(s->freq, freq);

	if (stats_get_num_values(s->offset) < s->max_count)
		return;

	stats_get_result(s->offset, &offset_stats);
	stats_get_result(s->freq, &freq_stats);

	/* Path delay stats are updated separately, they may be empty. */
	if (!stats_get_result(s->delay, &delay_stats)) {
/*
		pr_info("rms %4.0f max %4.0f "
			"freq %+6.0f +/- %3.0f "
			"delay %5.0f +/- %3.0f",
			offset_stats.rms, offset_stats.max_abs,
			freq_stats.mean, freq_stats.stddev,
			delay_stats.mean, delay_stats.stddev);
*/
	} else {
/*
		pr_info("rms %4.0f max %4.0f "
			"freq %+6.0f +/- %3.0f",
			offset_stats.rms, offset_stats.max_abs,
			freq_stats.mean, freq_stats.stddev);
*/
	}

	//stats_reset(s->offset);
	//stats_reset(s->freq);
	//stats_reset(s->delay);
}

static enum servo_state clock_no_adjust(struct clock *c)
{
	double fui;
	double ratio, freq;
	tmv_t origin2;
	struct freq_estimator *f = &c->fest;
	enum servo_state state = SERVO_UNLOCKED;
	/*
	 * We have clock.t1 as the origin time stamp, and clock.t2 as
	 * the ingress. According to the master's clock, the time at
	 * which the sync arrived is:
	 *
	 *    origin = origin_ts + path_delay + correction
	 *
	 * The ratio of the local clock freqency to the master clock
	 * is estimated by:
	 *
	 *    (ingress_2 - ingress_1) / (origin_2 - origin_1)
	 *
	 * Both of the origin time estimates include the path delay,
	 * but we assume that the path delay is in fact constant.
	 * By leaving out the path delay altogther, we can avoid the
	 * error caused by our imperfect path delay measurement.
	 */
	if (!f->ingress1) {
		f->ingress1 = c->t2;
		f->origin1 = tmv_add(c->t1, tmv_add(c->c1, c->c2));
		return state;
	}
	f->count++;
	if (f->count < f->max_count) {
		return state;
	}
	if (tmv_eq(c->t2, f->ingress1)) {
		pr_warning("bad timestamps in rate ratio calculation");
		return state;
	}
	/*
	 * origin2 = c->t1 (+c->path_delay) + c->c1 + c->c2;
	 */
	origin2 = tmv_add(c->t1, tmv_add(c->c1, c->c2));

	ratio = tmv_dbl(tmv_sub(origin2, f->origin1)) /
		tmv_dbl(tmv_sub(c->t2, f->ingress1));
	freq = (1.0 - ratio) * 1e9;

	if (c->stats.max_count > 1) {
		clock_stats_update(&c->stats,
				   tmv_to_nanoseconds(c->master_offset), freq);
	} else {
		pr_info("[T%i]: master offset %10" PRId64 " s%d freq %+7.0f "
			"path delay %9" PRId64,
			c->timelineid,
			tmv_to_nanoseconds(c->master_offset), state, freq,
			tmv_to_nanoseconds(c->path_delay));
	}

	fui = 1.0 + (c->status.cumulativeScaledRateOffset + 0.0) / POW2_41;

	pr_debug("peer/local    %.9f", c->nrr);
	pr_debug("fup_info      %.9f", fui);
	pr_debug("product       %.9f", fui * c->nrr);
	pr_debug("sum-1         %.9f", fui + c->nrr - 1.0);
	pr_debug("master/local  %.9f", ratio);
	pr_debug("diff         %+.9f", ratio - (fui + c->nrr - 1.0));

	f->ingress1 = c->t2;
	f->origin1 = origin2;
	f->count = 0;

	return state;
}

static void clock_update_grandmaster(struct clock *c)
{
	struct parentDS *pds = &c->dad.pds;
	memset(&c->cur, 0, sizeof(c->cur));
	memset(c->ptl, 0, sizeof(c->ptl));
	pds->parentPortIdentity.clockIdentity   = c->dds.clockIdentity;
	pds->parentPortIdentity.portNumber      = 0;
	pds->grandmasterIdentity                = c->dds.clockIdentity;
	pds->grandmasterClockQuality            = c->dds.clockQuality;
	pds->grandmasterPriority1               = c->dds.priority1;
	pds->grandmasterPriority2               = c->dds.priority2;
	c->dad.path_length                      = 0;
	c->tds.currentUtcOffset                 = c->utc_offset;
	c->tds.flags                            = c->time_flags;
	c->tds.timeSource                       = c->time_source;
}

static void clock_update_slave(struct clock *c)
{
	struct parentDS *pds = &c->dad.pds;
	struct ptp_message *msg        = TAILQ_FIRST(&c->best->messages);
	c->cur.stepsRemoved            = 1 + c->best->dataset.stepsRemoved;
	pds->parentPortIdentity        = c->best->dataset.sender;
	pds->grandmasterIdentity       = msg->announce.grandmasterIdentity;
	pds->grandmasterClockQuality   = msg->announce.grandmasterClockQuality;
	pds->grandmasterPriority1      = msg->announce.grandmasterPriority1;
	pds->grandmasterPriority2      = msg->announce.grandmasterPriority2;
	c->tds.currentUtcOffset        = msg->announce.currentUtcOffset;
	c->tds.flags                   = msg->header.flagField[1];
	c->tds.timeSource              = msg->announce.timeSource;
	if (!(c->tds.flags & PTP_TIMESCALE)) {
		pr_warning("foreign master not using PTP timescale");
	}
	if (c->tds.currentUtcOffset < CURRENT_UTC_OFFSET) {
		pr_warning("running in a temporal vortex");
	}
}

static int clock_utc_correct(struct clock *c, tmv_t ingress)
{
	struct timespec offset;
	int utc_offset, leap, clock_leap;
	uint64_t ts;

	if (!c->utc_timescale)
		return 0;

	if (c->tds.flags & UTC_OFF_VALID && c->tds.flags & TIME_TRACEABLE) {
		utc_offset = c->tds.currentUtcOffset;
	} else if (c->tds.currentUtcOffset > CURRENT_UTC_OFFSET) {
		utc_offset = c->tds.currentUtcOffset;
	} else {
		utc_offset = CURRENT_UTC_OFFSET;
	}

	if (c->tds.flags & LEAP_61) {
		leap = 1;
	} else if (c->tds.flags & LEAP_59) {
		leap = -1;
	} else {
		leap = 0;
	}

	/* Handle leap seconds. */
	if ((leap || c->leap_set) && c->clkid == CLOCK_REALTIME) {
		/* If the clock will be stepped, the time stamp has to be the
		   target time. Ignore possible 1 second error in utc_offset. */
		if (c->servo_state == SERVO_UNLOCKED) {
			ts = tmv_to_nanoseconds(tmv_sub(ingress,
							c->master_offset));
			if (c->tds.flags & PTP_TIMESCALE)
				ts -= utc_offset * NS_PER_SEC;
		} else {
			ts = tmv_to_nanoseconds(ingress);
		}

		/* Suspend clock updates in the last second before midnight. */
		if (is_utc_ambiguous(ts)) {
			pr_info("clock update suspended due to leap second");
			return -1;
		}

		clock_leap = leap_second_status(ts, c->leap_set,
						&leap, &utc_offset);
		if (c->leap_set != clock_leap) {
			if (c->kernel_leap)
				sysclk_set_leap(clock_leap);
			else
				servo_leap(c->servo, clock_leap);
			c->leap_set = clock_leap;
		}
	}

	/* Update TAI-UTC offset of the system clock if valid and traceable. */
	if (c->tds.flags & UTC_OFF_VALID && c->tds.flags & TIME_TRACEABLE &&
	    c->utc_offset_set != utc_offset && c->clkid == CLOCK_REALTIME) {
		sysclk_set_tai_offset(utc_offset);
		c->utc_offset_set = utc_offset;
	}

	if (!(c->tds.flags & PTP_TIMESCALE))
		return 0;

	offset.tv_sec = utc_offset;
	offset.tv_nsec = 0;
	/* Local clock is UTC, but master is TAI. */
	c->master_offset = tmv_add(c->master_offset, timespec_to_tmv(offset));
	return 0;
}

static int forwarding(struct clock *c, struct port *p)
{
	enum port_state ps = port_state(p);
	switch (ps) {
	case PS_MASTER:
	case PS_GRAND_MASTER:
	case PS_SLAVE:
	case PS_UNCALIBRATED:
	case PS_PRE_MASTER:
		return 1;
	default:
		break;
	}
	if (p == c->uds_port && ps != PS_FAULTY) {
		return 1;
	}
	return 0;
}

/* public methods */

UInteger8 clock_class(struct clock *c)
{
	return c->dds.clockQuality.clockClass;
}

static int clock_add_port(struct clock *c, int phc_index,
                          enum timestamp_type timestamping,
                          struct interface *iface)
{
	struct port *p, *piter, *lastp = NULL;

	if (clock_resize_pollfd(c, c->nports + 1))
		return -1;
	p = port_open(phc_index, timestamping, ++c->last_port_number + 100*c->timelineid,
	              iface, c);
	if (!p) {
		/* No need to shrink pollfd */
		return -1;
	}
	LIST_FOREACH(piter, &c->ports, list)
		lastp = piter;
	if (lastp)
		LIST_INSERT_AFTER(lastp, p, list);
	else
		LIST_INSERT_HEAD(&c->ports, p, list);
	c->nports++;
	clock_fda_changed(c);
	return 0;
}

static void clock_remove_port(struct clock *c, struct port *p)
{
	/* Do not call clock_resize_pollfd, it's pointless to shrink
	 * the allocated memory at this point, clock_destroy will free
	 * it all anyway. This function is usable from other parts of
	 * the code, but even then we don't mind if pollfd is larger
	 * than necessary. */
	LIST_REMOVE(p, list);
	c->nports--;
	clock_fda_changed(c);
	port_close(p);
}

struct clock *clock_create(int phc_index, struct interfaces_head *ifaces,
	enum timestamp_type timestamping, struct default_ds *dds,
	enum servo_type servo, int timelineid, int *timelinesfd, int timelines_size) /* QOT */
{
	int fadj = 0, max_adj = 0, sw_ts = timestamping == TS_SOFTWARE ? 1 : 0;
	//struct clock *c = &the_clock;
	struct clock *c = &global_clocks[timelineid]; // for multiple 'global' clocks
	struct port *p;
	char phc[32];
	char phccore[32]; /* QOT */
	struct interface *udsif = &c->uds_interface;
	struct interface *iface;
	struct timespec ts;

	/* Add core clock index and clockid , Fatima_START */ //todo: added for test
	int phccore_index = 1;
	snprintf(phccore, 31, "/dev/ptp%d", phccore_index);
	c->core_clkid = phc_open(phccore);
	if (c->core_clkid == CLOCK_INVALID) {
		pr_err("Failed to open %s: %m", phccore);
		return NULL;
	}
	/* Add core clock index and clockid , Fatima_END */

	clock_gettime(CLOCK_REALTIME, &ts);
	srandom(ts.tv_sec ^ ts.tv_nsec);

	if (c->nports)
		clock_destroy(c);

	snprintf(udsif->name, sizeof(udsif->name), "%s", uds_path);
	udsif->transport = TRANS_UDS;
	udsif->delay_filter_length = 1;

	c->sw_ts = sw_ts; /* QOT */
	c->free_running = dds->free_running;
	c->freq_est_interval = dds->freq_est_interval;
	c->grand_master_capable = dds->grand_master_capable;
	c->kernel_leap = dds->kernel_leap;
	c->utc_offset = CURRENT_UTC_OFFSET;
	c->time_source = dds->time_source;
	c->desc = dds->clock_desc;

	if (c->free_running) {
		c->clkid = CLOCK_INVALID;
		if (timestamping == TS_SOFTWARE || timestamping == TS_LEGACY_HW) {
			c->utc_timescale = 1;
		}
	} else if (phc_index >= 0) {
		snprintf(phc, 31, "/dev/ptp%d", phc_index);
		c->clkid = phc_open(phc);
		if (c->clkid == CLOCK_INVALID) {
			pr_err("Failed to open %s: %m", phc);
			return NULL;
		}
		max_adj = phc_max_adj(c->clkid);
		if (!max_adj) {
			pr_err("clock is not adjustable");
			return NULL;
		}
		clockadj_init(c->clkid);
	} else {
		c->clkid = CLOCK_REALTIME;
		c->utc_timescale = 1;
		clockadj_init(c->clkid);
		max_adj = sysclk_max_freq();
		sysclk_set_leap(0);
	}
	c->utc_offset_set = 0;
	c->leap_set = 0;
	c->time_flags = c->utc_timescale ? 0 : PTP_TIMESCALE;

	if (c->clkid != CLOCK_INVALID) {
		fadj = (int) clockadj_get_freq(c->clkid);
		/* Due to a bug in older kernels, the reading may silently fail
		   and return 0. Set the frequency back to make sure fadj is
		   the actual frequency of the clock. */
		clockadj_set_freq(c->clkid, fadj);
	}

	/* QOT, Creating individual servos for all the timelines */
	c->timelineid     = timelineid;
	c->timelinesfd    = timelinesfd; 
	c->timelines_size = timelines_size;

	LIST_INIT(&c->tmls_servos);
	if(create_timelines_servos(c, sw_ts, servo)){
		return NULL;
	}
	/* QOT */

	c->servo_state = SERVO_UNLOCKED;
	c->servo_type = servo;
	c->delay_filter = filter_create(dds->delay_filter,
	                                dds->delay_filter_length);
	if (!c->delay_filter) {
		pr_err("Failed to create delay filter");
		return NULL;
	}
	c->nrr = 1.0;
	c->stats_interval = dds->stats_interval;
	c->stats.offset = stats_create();
	c->stats.freq = stats_create();
	c->stats.delay = stats_create();
	if (!c->stats.offset || !c->stats.freq || !c->stats.delay) {
		pr_err("failed to create stats");
		return NULL;
	}
	if (dds->sanity_freq_limit) {
		c->sanity_check = clockcheck_create(dds->sanity_freq_limit);
		if (!c->sanity_check) {
			pr_err("Failed to create clock sanity check");
			return NULL;
		}
	}

	c->dds = dds->dds;

	/* Initialize the parentDS. */
	clock_update_grandmaster(c);
	c->dad.pds.parentStats                           = 0;
	c->dad.pds.observedParentOffsetScaledLogVariance = 0xffff;
	c->dad.pds.observedParentClockPhaseChangeRate    = 0x7fffffff;
	c->dad.ptl = c->ptl;

	clock_sync_interval(c, 0);

	LIST_INIT(&c->subscribers);
	LIST_INIT(&c->ports);
	c->last_port_number = 0;

	/*
	 * Create the UDS interface.
	 */
	if (clock_resize_pollfd(c, 0)) {
		pr_err("failed to allocate pollfd");
		return NULL;
	}
	c->uds_port = port_open(phc_index, timestamping, 0, udsif, c);
	if (!c->uds_port) {
		pr_err("failed to open the UDS port");
		return NULL;
	}
	clock_fda_changed(c);

	/* Create the ports. */
	STAILQ_FOREACH(iface, ifaces, list) {
		if (clock_add_port(c, phc_index, timestamping, iface)) {
			pr_err("failed to open port %s", iface->name);
			return NULL;
		}
	}

	c->dds.numberPorts = c->nports;

	LIST_FOREACH(p, &c->ports, list) {
		port_dispatch(p, EV_INITIALIZE, 0);
	}
	port_dispatch(c->uds_port, EV_INITIALIZE, 0);

	return c;
}

struct dataset *clock_best_foreign(struct clock *c)
{
	return c->best ? &c->best->dataset : NULL;
}

struct port *clock_best_port(struct clock *c)
{
	return c->best ? c->best->port : NULL;
}

struct dataset *clock_default_ds(struct clock *c)
{
	struct dataset *out = &c->default_dataset;
	struct defaultDS *in = &c->dds;

	out->priority1              = in->priority1;
	out->identity               = in->clockIdentity;
	out->quality                = in->clockQuality;
	out->priority2              = in->priority2;
	out->stepsRemoved           = 0;
	out->sender.clockIdentity   = in->clockIdentity;
	out->sender.portNumber      = 0;
	out->receiver.clockIdentity = in->clockIdentity;
	out->receiver.portNumber    = 0;

	return out;
}

UInteger8 clock_domain_number(struct clock *c)
{
	return c->dds.domainNumber;
}

void clock_follow_up_info(struct clock *c, struct follow_up_info_tlv *f)
{
	c->status.cumulativeScaledRateOffset = f->cumulativeScaledRateOffset;
	c->status.scaledLastGmPhaseChange = f->scaledLastGmPhaseChange;
	c->status.gmTimeBaseIndicator = f->gmTimeBaseIndicator;
	memcpy(&c->status.lastGmPhaseChange, &f->lastGmPhaseChange,
	       sizeof(c->status.lastGmPhaseChange));
}

int clock_gm_capable(struct clock *c)
{
	return c->grand_master_capable;
}

struct ClockIdentity clock_identity(struct clock *c)
{
	return c->dds.clockIdentity;
}

static int clock_resize_pollfd(struct clock *c, int new_nports)
{
	struct pollfd *new_pollfd;

	/* Need to allocate one extra block of fds for uds */
	new_pollfd = realloc(c->pollfd,
			     (new_nports + 1) * N_CLOCK_PFD *
			     sizeof(struct pollfd));
	if (!new_pollfd)
		return -1;
	c->pollfd = new_pollfd;
	return 0;
}

static void clock_fill_pollfd(struct pollfd *dest, struct port *p)
{
	struct fdarray *fda;
	int i;

	fda = port_fda(p);
	for (i = 0; i < N_POLLFD; i++) {
		dest[i].fd = fda->fd[i];
		dest[i].events = POLLIN|POLLPRI;
	}
	dest[i].fd = port_fault_fd(p);
	dest[i].events = POLLIN|POLLPRI;
}

static void clock_check_pollfd(struct clock *c)
{
	struct port *p;
	struct pollfd *dest = c->pollfd;

	if (c->pollfd_valid)
		return;
	LIST_FOREACH(p, &c->ports, list) {
		clock_fill_pollfd(dest, p);
		dest += N_CLOCK_PFD;
	}
	clock_fill_pollfd(dest, c->uds_port);
	c->pollfd_valid = 1;
}

void clock_fda_changed(struct clock *c)
{
	c->pollfd_valid = 0;
}

static int clock_do_forward_mgmt(struct clock *c,
				 struct port *in, struct port *out,
				 struct ptp_message *msg, int *pre_sent)
{
	if (in == out || !forwarding(c, out))
		return 0;
	if (!*pre_sent) {
		/* delay calling msg_pre_send until
		 * actually forwarding */
		msg_pre_send(msg);
		*pre_sent = 1;
	}
	return port_forward(out, msg);
}

static void clock_forward_mgmt_msg(struct clock *c, struct port *p, struct ptp_message *msg)
{
	struct port *piter;
	int pdulen = 0, msg_ready = 0;

	if (forwarding(c, p) && msg->management.boundaryHops) {
		pdulen = msg->header.messageLength;
		msg->management.boundaryHops--;
		LIST_FOREACH(piter, &c->ports, list) {
			if (clock_do_forward_mgmt(c, p, piter, msg, &msg_ready))
				pr_err("port %d: management forward failed",
				       port_number(piter));
		}
		if (clock_do_forward_mgmt(c, p, c->uds_port, msg, &msg_ready))
			pr_err("uds port: management forward failed");
		if (msg_ready) {
			msg_post_recv(msg, pdulen);
			msg->management.boundaryHops++;
		}
	}
}

int clock_manage(struct clock *c, struct port *p, struct ptp_message *msg)
{
	int changed = 0, res, answers;
	struct port *piter;
	struct management_tlv *mgt;
	struct ClockIdentity *tcid, wildcard = {
		{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}
	};

	/* Forward this message out all eligible ports. */
	clock_forward_mgmt_msg(c, p, msg);

	/* Apply this message to the local clock and ports. */
	tcid = &msg->management.targetPortIdentity.clockIdentity;
	if (!cid_eq(tcid, &wildcard) && !cid_eq(tcid, &c->dds.clockIdentity)) {
		return changed;
	}
	if (msg->tlv_count != 1) {
		return changed;
	}
	mgt = (struct management_tlv *) msg->management.suffix;

	/*
	  The correct length according to the management ID is checked
	  in tlv.c, but management TLVs with empty bodies are also
	  received successfully to support GETs and CMDs. At this
	  point the TLV either has the correct length or length 2.
	*/
	switch (management_action(msg)) {
	case GET:
		if (clock_management_get_response(c, p, mgt->id, msg))
			return changed;
		break;
	case SET:
		if (mgt->length == 2 && mgt->id != TLV_NULL_MANAGEMENT) {
			clock_management_send_error(p, msg, TLV_WRONG_LENGTH);
			return changed;
		}
		if (p != c->uds_port) {
			/* Sorry, only allowed on the UDS port. */
			clock_management_send_error(p, msg, TLV_NOT_SUPPORTED);
			return changed;
		}
		if (clock_management_set(c, p, mgt->id, msg, &changed))
			return changed;
		break;
	case COMMAND:
		break;
	default:
		return changed;
	}

	switch (mgt->id) {
	case TLV_PORT_PROPERTIES_NP:
		if (p != c->uds_port) {
			/* Only the UDS port allowed. */
			clock_management_send_error(p, msg, TLV_NOT_SUPPORTED);
			return 0;
		}
	}

	switch (mgt->id) {
	case TLV_USER_DESCRIPTION:
	case TLV_SAVE_IN_NON_VOLATILE_STORAGE:
	case TLV_RESET_NON_VOLATILE_STORAGE:
	case TLV_INITIALIZE:
	case TLV_FAULT_LOG:
	case TLV_FAULT_LOG_RESET:
	case TLV_DEFAULT_DATA_SET:
	case TLV_CURRENT_DATA_SET:
	case TLV_PARENT_DATA_SET:
	case TLV_TIME_PROPERTIES_DATA_SET:
	case TLV_PRIORITY1:
	case TLV_PRIORITY2:
	case TLV_DOMAIN:
	case TLV_SLAVE_ONLY:
	case TLV_TIME:
	case TLV_CLOCK_ACCURACY:
	case TLV_UTC_PROPERTIES:
	case TLV_TRACEABILITY_PROPERTIES:
	case TLV_TIMESCALE_PROPERTIES:
	case TLV_PATH_TRACE_LIST:
	case TLV_PATH_TRACE_ENABLE:
	case TLV_GRANDMASTER_CLUSTER_TABLE:
	case TLV_ACCEPTABLE_MASTER_TABLE:
	case TLV_ACCEPTABLE_MASTER_MAX_TABLE_SIZE:
	case TLV_ALTERNATE_TIME_OFFSET_ENABLE:
	case TLV_ALTERNATE_TIME_OFFSET_NAME:
	case TLV_ALTERNATE_TIME_OFFSET_MAX_KEY:
	case TLV_ALTERNATE_TIME_OFFSET_PROPERTIES:
	case TLV_TRANSPARENT_CLOCK_DEFAULT_DATA_SET:
	case TLV_PRIMARY_DOMAIN:
	case TLV_TIME_STATUS_NP:
	case TLV_GRANDMASTER_SETTINGS_NP:
	case TLV_SUBSCRIBE_EVENTS_NP:
		clock_management_send_error(p, msg, TLV_NOT_SUPPORTED);
		break;
	default:
		answers = 0;
		LIST_FOREACH(piter, &c->ports, list) {
			res = port_manage(piter, p, msg);
			if (res < 0)
				return changed;
			if (res > 0)
				answers++;
		}
		if (!answers) {
			/* IEEE 1588 Interpretation #21 suggests to use
			 * TLV_WRONG_VALUE for ports that do not exist */
			clock_management_send_error(p, msg, TLV_WRONG_VALUE);
		}
		break;
	}
	return changed;
}

void clock_notify_event(struct clock *c, enum notification event)
{
	struct port *uds = c->uds_port;
	struct PortIdentity pid = port_identity(uds);
	struct ptp_message *msg;
	UInteger16 msg_len;
	int id;

	switch (event) {
	/* set id */
	default:
		return;
	}
	/* targetPortIdentity and sequenceId will be filled by
	 * clock_send_notification */
	msg = port_management_notify(pid, uds);
	if (!msg)
		return;
	if (!clock_management_fill_response(c, NULL, NULL, msg, id))
		goto err;
	msg_len = msg->header.messageLength;
	if (msg_pre_send(msg))
		goto err;
	clock_send_notification(c, msg, msg_len, event);
err:
	msg_put(msg);
}

struct parent_ds *clock_parent_ds(struct clock *c)
{
	return &c->dad;
}

struct PortIdentity clock_parent_identity(struct clock *c)
{
	return c->dad.pds.parentPortIdentity;
}

int clock_poll(struct clock *c)
{
	int cnt, err, i, sde = 0;
	enum fsm_event event;
	struct pollfd *cur;
	struct port *p;

	clock_check_pollfd(c);
	cnt = poll(c->pollfd, (c->nports + 1) * N_CLOCK_PFD, -1);
	if (cnt < 0) {
		if (EINTR == errno) {
			return 0;
		} else {
			pr_emerg("poll failed");
			return -1;
		}
	} else if (!cnt) {
		return 0;
	}

	cur = c->pollfd;
	LIST_FOREACH(p, &c->ports, list) {
		/* Let the ports handle their events. */
		for (i = err = 0; i < N_POLLFD && !err; i++) {
			if (cur[i].revents & (POLLIN|POLLPRI)) {
				event = port_event(p, i);
				if (EV_STATE_DECISION_EVENT == event)
					sde = 1;
				if (EV_ANNOUNCE_RECEIPT_TIMEOUT_EXPIRES == event)
					sde = 1;
				err = port_dispatch(p, event, 0);
				/* Clear any fault after a little while. */
				if (PS_FAULTY == port_state(p)) {
					clock_fault_timeout(p, 1);
					break;
				}
			}
		}

		/* Check the fault timer. */
		if (cur[N_POLLFD].revents & (POLLIN|POLLPRI)) {
			clock_fault_timeout(p, 0);
			port_dispatch(p, EV_FAULT_CLEARED, 0);
		}

		cur += N_CLOCK_PFD;
	}

	/* Check the UDS port. */
	for (i = 0; i < N_POLLFD; i++) {
		if (cur[i].revents & (POLLIN|POLLPRI)) {
			event = port_event(c->uds_port, i);
			if (EV_STATE_DECISION_EVENT == event)
				sde = 1;
		}
	}

	if (sde)
		handle_state_decision_event(c);

	clock_prune_subscriptions(c);
	return 0;
}

void clock_path_delay(struct clock *c, struct timespec req, struct timestamp rx,
		      Integer64 correction)
{
	tmv_t c1, c2, c3, pd, t1, t2, t3, t4;
	double rr;

	if (tmv_is_zero(c->t1))
		return;

	c1 = c->c1;
	c2 = c->c2;
	c3 = correction_to_tmv(correction);
	t1 = c->t1;
	t2 = c->t2;
	t3 = timespec_to_tmv(req);
	t4 = timestamp_to_tmv(rx);
	// rr = clock_rate_ratio(c); /* QOT, clock drift between path delay message exchange is negligible */
	rr = 1.0; /* QOT */

	/*
	 * c->path_delay = (t2 - t3) * rr + (t4 - t1);
	 * c->path_delay -= c_sync + c_fup + c_delay_resp;
	 * c->path_delay /= 2.0;
	 */

	pd = tmv_sub(t2, t3);
	if (rr != 1.0)
		pd = dbl_tmv(tmv_dbl(pd) * rr);
	pd = tmv_add(pd, tmv_sub(t4, t1));
	pd = tmv_sub(pd, tmv_add(c1, tmv_add(c2, c3)));
	pd = tmv_div(pd, 2);

	if (pd < 0) {
		pr_debug("negative path delay %10" PRId64, pd);
		pr_debug("path_delay = (t2 - t3) * rr + (t4 - t1) - (c1 + c2 + c3)");
		pr_debug("t2 - t3 = %+10" PRId64, t2 - t3);
		pr_debug("t4 - t1 = %+10" PRId64, t4 - t1);
		pr_debug("rr = %.9f", rr);
		pr_debug("c1 %10" PRId64, c1);
		pr_debug("c2 %10" PRId64, c2);
		pr_debug("c3 %10" PRId64, c3);
	}

	c->path_delay = filter_sample(c->delay_filter, pd);

	c->cur.meanPathDelay = tmv_to_TimeInterval(c->path_delay);

	pr_debug("path delay    %10" PRId64 " %10" PRId64, c->path_delay, pd);

	if (c->stats.delay)
		stats_add_value(c->stats.delay, tmv_to_nanoseconds(pd));
}

void clock_peer_delay(struct clock *c, tmv_t ppd, double nrr)
{
	c->path_delay = ppd;
	c->nrr = nrr;

	if (c->stats.delay)
		stats_add_value(c->stats.delay, tmv_to_nanoseconds(ppd));
}

int clock_slave_only(struct clock *c)
{
	return c->dds.flags & DDS_SLAVE_ONLY;
}

UInteger16 clock_steps_removed(struct clock *c)
{
	return c->cur.stepsRemoved;
}

int clock_switch_phc(struct clock *c, int phc_index)
{
	struct servo *servo;
	int fadj, max_adj;
	clockid_t clkid;
	char phc[32];

	snprintf(phc, 31, "/dev/ptp%d", phc_index);
	clkid = phc_open(phc);
	if (clkid == CLOCK_INVALID) {
		pr_err("Switching PHC, failed to open %s: %m", phc);
		return -1;
	}
	max_adj = phc_max_adj(clkid);
	if (!max_adj) {
		pr_err("Switching PHC, clock is not adjustable");
		phc_close(clkid);
		return -1;
	}
	fadj = (int) clockadj_get_freq(clkid);
	clockadj_set_freq(clkid, fadj);

	/* QOT, destroying servos for all timelines */
	destroy_timelines_servos(c);

	if(create_timelines_servos(c, 0, c->servo_type)){
		pr_err("Switching PHC, failed to create timeline servo");
		phc_close(clkid);
		return -1;
	}

	/*servo = servo_create(c->servo_type, -fadj, max_adj, 0);
	if (!servo) {
		pr_err("Switching PHC, failed to create clock servo");
		phc_close(clkid);
		return -1;
	}*/
	/* QOT */

	phc_close(c->clkid);
	//servo_destroy(c->servo); /* QOT */
	c->clkid = clkid;
	//c->servo = servo; /* QOT */
	c->servo_state = SERVO_UNLOCKED;
	return 0;
}

// Global Variable for Sharing Computed Clock Statistic from Sync to Uncertainty Calculation
qot_stat_t ptp_clocksync_data_point;

enum servo_state clock_synchronize(struct clock *c,
				   struct timespec ingress_ts,
				   struct timestamp origin_ts,
				   Integer64 correction1,
				   Integer64 correction2)
{
	double adj;
	tmv_t ingress, origin;
	enum servo_state state;
	struct servo *s;
	struct timespec ingress_tml, ingress_tml_inv; //QOT
	tmv_t tml_offset, tml_local; //QOT
	double dmax, dmin;
	//qot_bounds_t bounds;// Commented out by Sandeep
	int fd;
	struct stats_result offset_stats, freq_stats;

	/* QOT, synchronize all the timelines */
	LIST_FOREACH(s, &c->tmls_servos, list) {
		state = SERVO_UNLOCKED;

	/* QOT, Project received timestamp to timeline reference */
	if(clock_project_timeline(s->tml_clkid, ingress_ts, &ingress_tml)){
		pr_warning("[T%i]: timeline projection failed", c->timelineid);
		return SERVO_UNLOCKED;
	}
	
	// Populate timestamps for offset calculation
	ingress = timespec_to_tmv(ingress_ts); 		/* QOT */
	origin  = timestamp_to_tmv(origin_ts);

	c->t1 = origin;
	c->t2 = timespec_to_tmv(ingress_ts);

	c->c1 = correction_to_tmv(correction1);
	c->c2 = correction_to_tmv(correction2);

	// Added by Sandeep -> Calculate ingress timeline timestamp (shifting from feed-forward to feedback)
	ingress = timespec_to_tmv(ingress_tml); 		/* QOT */

	/*
	 * c->master_offset = ingress - origin - c->path_delay - c->c1 - c->c2;
	 */
	c->master_offset = tmv_sub(ingress,
		tmv_add(origin, tmv_add(c->path_delay, tmv_add(c->c1, c->c2))));

	tml_local = timespec_to_tmv(ingress_tml); /* QOT */
	tml_offset = tmv_sub(tml_local, tmv_add(origin, tmv_add(c->path_delay, tmv_add(c->c1, c->c2)))); /* QOT */

	if (!c->path_delay)
		return state;

	if (clock_utc_correct(c, ingress))
		return c->servo_state;

	c->cur.offsetFromMaster = tmv_to_TimeInterval(c->master_offset);

	//if (c->free_running) -
		//return clock_no_adjust(c);

	/*adj = servo_sample(c->servo, tmv_to_nanoseconds(c->master_offset),
			   tmv_to_nanoseconds(ingress), &state);*/
	// Commented out by Sandeep (Old code written by Fatima)
	// adj = servo_sample(CLOCK_SERVO_LINREGNEW, s, tmv_to_nanoseconds(c->master_offset),
	// 		   tmv_to_nanoseconds(ingress), &state, &dmax, &dmin); /* QOT */

	// Added by Sandeep
    adj = servo_sample(CLOCK_SERVO_PI, s, tmv_to_nanoseconds(c->master_offset),
			   tmv_to_nanoseconds(ingress), &state, &dmax, &dmin); /* QOT */

	c->servo_state = state;

	/* QOT, Stats maintained in QOT core */
	/*if (c->stats.max_count > 1) {
		clock_stats_update(&c->stats,
				   tmv_to_nanoseconds(tml_offset), adj);
	}*/
	// else {
	pr_info("[T%i]: tid: %i master offset %10" PRId64 " s%d freq %+7.0f dmax %+7.0f dmin %+7.0f "
		"path delay %9" PRId64,
		c->timelineid,
		s->tml_clkid,
		tmv_to_nanoseconds(tml_offset),
		state, adj, dmax, dmin,
		tmv_to_nanoseconds(c->path_delay));
			
	//}


	switch (state) {
	case SERVO_UNLOCKED:
		break;
	case SERVO_JUMP:
		//clockadj_set_freq(c->clkid, -adj); /* QOT */
		//clockadj_step(c->clkid, -tmv_to_nanoseconds(c->master_offset)); 
		// Adjust the timeline
		clockadj_set_freq(s->tml_clkid, -adj); /* QOT */
		clockadj_step(s->tml_clkid, -tmv_to_nanoseconds(tml_offset)); /* QOT */ 
		c->t1 = tmv_zero();
		c->t2 = tmv_zero();
		if (c->sanity_check) {
			clockcheck_set_freq(c->sanity_check, -adj);
			clockcheck_step(c->sanity_check,
					-tmv_to_nanoseconds(c->master_offset));
		}
		// Add Clock-Skew Statistic for the QoT Uncertainty Service to process
		ptp_clocksync_data_point.offset = tmv_to_nanoseconds(tml_offset);
		ptp_clocksync_data_point.drift = (int64_t)ceil(adj);
		ptp_clocksync_data_point.data_id++;
		break;
	case SERVO_LOCKED:
		/* QOT */
		/*clockadj_set_freq(c->clkid, -adj);
		if (c->clkid == CLOCK_REALTIME)
			sysclk_set_sync();
		*/
		// Adjust the timeline
		if(tmv_to_nanoseconds(tml_offset) < 0)
			clockadj_set_freq(s->tml_clkid, -adj); /* QOT */
		else
			clockadj_set_freq(s->tml_clkid, -adj); /* QOT */
		//clockadj_step(s->tml_clkid, -tmv_to_nanoseconds(tml_offset)); /* QOT */

		/* SET UNCERTAINTY START */
		// Find uncertainty in Offset (Variance)
		clock_stats_update(&c->stats,
				tmv_to_nanoseconds(tml_offset), adj);
		stats_get_result(c->stats.offset, &offset_stats); //QOT
		stats_get_result(c->stats.freq, &freq_stats); //QOT
		if(offset_stats.stddev == 0)
			offset_stats.stddev = c->off_stddev;
		if(freq_stats.stddev == 0)
			freq_stats.stddev = c->freq_stddev;

		// Add Statistic for the QoT Uncertainty Service to process
		ptp_clocksync_data_point.offset = tmv_to_nanoseconds(tml_offset);
		ptp_clocksync_data_point.drift = (int64_t)ceil(adj);
		ptp_clocksync_data_point.data_id++;
		
		//Invert max and min since we run the clock in opposite direction to compensate for drift
		//bounds.u_drift = (s32) (-dmin);
		//bounds.l_drift = (s32) (-dmax);
		// bounds.u_drift = (s32) -(freq_stats.mean - (0.5*freq_stats.stddev)); // Commented out by Sandeep
		// bounds.l_drift = (s32) -(freq_stats.mean + (0.5*freq_stats.stddev)); // Commented out by Sandeep
		//bounds.u_nsec = -(tmv_to_nanoseconds(tml_offset) - 0.9*(tmv_to_nanoseconds(tml_offset)) - offset_stats.stddev);
		//bounds.l_nsec = -(tmv_to_nanoseconds(tml_offset) - 0.9*(tmv_to_nanoseconds(tml_offset)) + offset_stats.stddev);
		
		// bounds.u_nsec = -(offset_stats.rms - offset_stats.stddev); // Commented out by Sandeep
		// bounds.l_nsec = -(offset_stats.rms + offset_stats.stddev); // Commented out by Sandeep

		//c->off_stddev = offset_stats.stddev;
		//c->freq_stddev = freq_stats.stddev;

		//pr_info("[T%i]: Offset upp %lld Offset low %lld Freq upp %lld Freq low %lld ",
		//	c->timelineid, bounds.u_nsec, bounds.l_nsec, bounds.u_drift, bounds.l_drift);

		// fd = CLOCKID_TO_FD(s->tml_clkid); // Commented out by Sandeep

		// if(ioctl(fd, TIMELINE_SET_SYNC_UNCERTAINTY, &bounds)){ // Commented out by Sandeep
		// 	pr_warning("Setting sync uncertainty failed for timeline "); // Commented out by Sandeep
		// } // Commented out by Sandeep
		/* SET UNCERTAINTY END */

		if (c->sanity_check)
			clockcheck_set_freq(c->sanity_check, -adj);

		/*if(s->local_ts && s->ppb){ // QOT, calculate uncertainty values, provided it is not the first sample
			uint64_t disp = ((tmv_to_nanoseconds(ingress) - s->local_ts)/NS_PER_SEC) * s->ppb;
			add_clock_uncertainty_sample(&s->cu_stats, disp, 
				tmv_to_nanoseconds(c->master_offset), tmv_to_nanoseconds(c->path_delay));	
			struct Bound bound; 
			update_clock_uncertainty(&s->cu_stats, &bound); // Todo: send uncertainty to core
		}*/
		break;
	}
		//s->local_ts = tmv_to_nanoseconds(ingress);	/* QOT */
		//s->ppb = adj; /* QOT */
	}
	return state;
}

void clock_sync_interval(struct clock *c, int n)
{
	int shift;

	shift = c->freq_est_interval - n;
	if (shift < 0)
		shift = 0;
	else if (shift >= sizeof(int) * 8) {
		shift = sizeof(int) * 8 - 1;
		pr_warning("freq_est_interval is too long");
	}
	c->fest.max_count = (1 << shift);

	shift = c->stats_interval - n;
	if (shift < 0)
		shift = 0;
	else if (shift >= sizeof(int) * 8) {
		shift = sizeof(int) * 8 - 1;
		pr_warning("summary_interval is too long");
	}
	c->stats.max_count = (1 << shift);

	/* QOT, setting sync interval for all timelines servos */
	//servo_sync_interval(c->servo, n < 0 ? 1.0 / (1 << -n) : 1 << n);
	timelines_servos_sync_interval(c, n < 0 ? 1.0 / (1 << -n) : 1 << n);
}

struct timePropertiesDS *clock_time_properties(struct clock *c)
{
	return &c->tds;
}

void clock_update_time_properties(struct clock *c, struct timePropertiesDS tds)
{
	c->tds = tds;
}

static void handle_state_decision_event(struct clock *c)
{
	struct foreign_clock *best = NULL, *fc;
	struct ClockIdentity best_id;
	struct port *piter;
	int fresh_best = 0;

	LIST_FOREACH(piter, &c->ports, list) {
		fc = port_compute_best(piter);
		if (!fc)
			continue;
		if (!best || dscmp(&fc->dataset, &best->dataset) > 0)
			best = fc;
	}

	if (best) {
		best_id = best->dataset.identity;
	} else {
		best_id = c->dds.clockIdentity;
	}

	pr_notice("[T%i]: selected best master clock %s",
		c->timelineid, cid2str(&best_id));

	if (!cid_eq(&best_id, &c->best_id)) {
		clock_freq_est_reset(c);
		filter_reset(c->delay_filter);
		c->t1 = tmv_zero();
		c->t2 = tmv_zero();
		c->path_delay = 0;
		c->nrr = 1.0;
		fresh_best = 1;
	}

	c->best = best;
	c->best_id = best_id;

	LIST_FOREACH(piter, &c->ports, list) {
		enum port_state ps;
		enum fsm_event event;
		ps = bmc_state_decision(c, piter);
		switch (ps) {
		case PS_LISTENING:
			event = EV_NONE;
			break;
		case PS_GRAND_MASTER:
			pr_notice("[T%i]: assuming the grand master role", c->timelineid);
			clock_update_grandmaster(c);
			event = EV_RS_GRAND_MASTER;
			break;
		case PS_MASTER:
			event = EV_RS_MASTER;
			break;
		case PS_PASSIVE:
			event = EV_RS_PASSIVE;
			break;
		case PS_SLAVE:
			clock_update_slave(c);
			event = EV_RS_SLAVE;
			break;
		default:
			event = EV_FAULT_DETECTED;
			break;
		}
		port_dispatch(piter, event, fresh_best);
	}
}

struct clock_description *clock_description(struct clock *c)
{
	return &c->desc;
}

int clock_num_ports(struct clock *c)
{
	return c->nports;
}

void clock_check_ts(struct clock *c, struct timespec ts)
{
	/* QOT, removed sanity check for now */
	/*if (c->sanity_check &&
	    clockcheck_sample(c->sanity_check,
			      ts.tv_sec * NS_PER_SEC + ts.tv_nsec)) {
		servo_reset(c->servo);
	}
	*/
}

double clock_rate_ratio(struct clock *c)
{
	return servo_rate_ratio(c->servo);
}

/* QoT_start */
int clock_project_timeline(clockid_t clkid, struct timespec ts, struct timespec *tml_ts)
{
	stimepoint_t utp;
	timepoint_from_timespec(&utp.estimate, &ts); 

	int fd = CLOCKID_TO_FD(clkid);

	if(ioctl(fd, TIMELINE_CORE_TO_REMOTE, &utp)){
		return -1;
	}
	tml_ts->tv_sec = utp.estimate.sec;
    tml_ts->tv_nsec= utp.estimate.asec / NS_PER_SEC;
	return 0;
}

int clock_project_inverse_timeline(clockid_t clkid, struct timespec ts, struct timespec *tml_ts)
{
	timepoint_t tp;
	timepoint_from_timespec(&tp, &ts); 

	int fd = CLOCKID_TO_FD(clkid);

	if(ioctl(fd, TIMELINE_REMOTE_TO_CORE, &tp)){
		return -1;
	}
	tml_ts->tv_sec = tp.sec;
    tml_ts->tv_nsec= tp.asec / NS_PER_SEC;
	return 0;
}

void get_core_time(struct clock *c, struct timespec *ts){
	if(c->sw_ts){
		if(clock_gettime(c->core_clkid, ts))
			pr_err("failed to read core clock: %m");
		//pr_info("core time, sec: %u core time nsec: %lu\n",
				//(int)ts->tv_sec, ts->tv_nsec);
	}
}
/* QoT_end */ 
