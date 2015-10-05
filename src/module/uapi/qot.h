#ifndef QOTCLOCK_H
#define QOTCLOCK_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#include "qot_ioctl.h"

/**-------- function calls for clock  --------**/

/** Phase 1: initialize or release a clock **/

uint16_t qot_init_clock(uint16_t timeline, struct timespec accuracy, struct timespec resolution);
void qot_release_clock(uint16_t bindingId);

/** Phase 2: dynamically change and access the quality metrics of a registered clock **/

void qot_set_accuracy(uint16_t bindingId, struct timespec accuracy);
void qot_set_resolution(uint16_t bindingId, struct timespec resolution);
//void qot_set_timeline(qot_clock clk, uint16_t timeline);

//struct timespec qot_get_time(qot_clock clk);

/** Phase 3: Event scheduling, ordering and tracking **/

// wait for a specified amount of time w.r.t the given clock timeline
void qot_wait_until(qot_clock clk, struct timespec eventtime);

// wait for a specified amount of clock cycles w.r.t the given clock timeline
void qot_wait_until_cycles(qot_clock clk, uint64_t cycles);

// schedule an event at eventtime according to the given clock
void qot_schedule_event(qot_clock clk, struct timespec eventtime, struct timespec high, struct timespec low, uint16_t repetitions);

// returns the time for the upcoming event according to the clock
struct timespec qot_get_next_event(qot_clock clk);

// prints the scheduled event list for this clock
void qot_get_scheduled_events(qot_clock clk);

// cancels the specified scheduled event
void qot_cancel_event(qot_clock clk, struct timespec eventtime);

// track an event locally which was timestamped according to the given clock
struct timespec qot_track_event(qot_clock clk, struct timespec eventtime);

#endif
