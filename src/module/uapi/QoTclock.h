#ifndef QOTCLOCK_H
#define QOTCLOCK_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>

#include "../qot_ioctl.h"

/**-------- function calls for clock  --------**/

/** Phase 1: initialize or release a clock **/

int qot_init_clock(char timeline[], uint64_t accuracy, uint64_t resolution);
void qot_release_clock(int bindingId);

/** Phase 2: dynamically change and access the quality metrics of a registered clock **/

void qot_set_accuracy(int bindingId, uint64_t accuracy);
void qot_set_resolution(int bindingId, uint64_t resolution);
//void qot_set_timeline(qot_clock clk, uint16_t timeline);

//struct timespec qot_get_time(qot_clock clk);

/** Phase 3: Event scheduling, ordering and tracking **/

// wait for a specified amount of time w.r.t the given clock timeline
void qot_wait_until(int bindingId, uint64_t eventtime);

// wait for a specified amount of clock cycles w.r.t the given clock timeline
void qot_wait_until_cycles(int bindingId, uint64_t cycles);

// schedule an event at eventtime according to the given clock
void qot_schedule_event(int bindingId, uint64_t eventtime, uint64_t high, uint64_t low, uint16_t repetitions);

// returns the time for the upcoming event according to the clock
uint64_t qot_get_next_event(int bindingId);

// prints the scheduled event list for this clock
void qot_get_scheduled_events(int bindingId);

// cancels the specified scheduled event
void qot_cancel_event(int bindingId, uint64_t eventtime);

// track an event locally which was timestamped according to the given clock
//struct timespec qot_track_event(qot_clock clk, struct timespec eventtime);

#endif
