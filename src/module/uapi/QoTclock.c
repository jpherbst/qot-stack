#include "QoTclock.h"

#include <sys/ioctl.h>

int fd;
const char *filename = "/dev/qot";	// name tbd with scheduler

// ----------------- Functions ---------------------
int qot_init_clock(char timeline[], uint64_t accuracy, uint64_t resolution)
{
	int binding_id;
	//open ioctl handle with the scheduler
	fd = open(filename, O_RDWR);
	
	qot_clock clk;
	strncpy(clk.timeline, timeline, MAX_UUIDLEN);
	clk.accuracy = accuracy;
	clk.resolution = resolution;
	
	// add this clock to the qot clock list through scheduler
	if (ioctl(fd, QOT_INIT_CLOCK, &clk) == 0)
	{
		if(ioctl(fd, QOT_RETURN_BINDING, &binding_id) == 0)
			printf("Clock created with binding id %i\n", binding_id);
		else
			return -1;
	}
	
	return binding_id;
}

void qot_release_clock(int bindingId)
{
	// delete this clock from the qot clock list
	if (ioctl(fd, QOT_RELEASE_CLOCK, &bindingId) == 0)
	{
		printf("Clock released with binding id %i\n", bindingId);
	}
	
	close(fd);
}

void qot_set_accuracy(int bindingId, uint64_t accuracy)
{
	qot_clock clk;
	clk.accuracy = accuracy;
	clk.binding_id = bindingId;
	
	// update this clock
	if (ioctl(fd, QOT_SET_ACCURACY, &clk) == 0)
	{
		printf("Clock accuracy updated %i\n", bindingId);
	}
}

void qot_set_resolution(int bindingId, uint64_t resolution)
{
	qot_clock clk;
	clk.resolution = resolution;
	clk.binding_id = bindingId;
	
	if (ioctl(fd, QOT_SET_RESOLUTION, &clk) == 0)
	{
		printf("Clock resolution updated %i\n", bindingId);
	}
}

/* Not needed because it's better to declare a new binding instead of switching the timeline
void qot_set_timeline(qot_clock clk, uint16_t timeline)
{
	
}
*/

void qot_wait_until(int bindingId, uint64_t eventtime)
{
	qot_sched sched;
	sched.binding_id = bindingId;
	sched.event_time = eventtime;
	
	if (ioctl(fd, QOT_WAIT, &sched) != -1)
	{
		printf("Wait till %llu nsec \n", sched.event_time);
	}
}

void qot_wait_until_cycles(int bindingId, uint64_t cycles)
{
	qot_sched sched;
	sched.binding_id = bindingId;
	sched.event_cycles = cycles;
	
	if (ioctl(fd, QOT_WAIT_CYCLES, &sched) != -1)
	{
		printf("Wait till %llu cycles\n", sched.event_cycles);
	}
}

void qot_schedule_event(int bindingId, uint64_t eventtime, uint64_t high, uint64_t low, uint16_t repetitions)
{
	qot_sched sched;
	sched.binding_id = bindingId;
	sched.event_time = eventtime;
	sched.high = high;
	sched.low = low;
	sched.limit = repetitions;
	
	if (ioctl(fd, QOT_SET_SCHED, &sched) != -1)
	{
		printf("Schedule set at %llu nsec \n", sched.event_time);
	}
}

/*
struct timespec qot_get_next_event(int bindingId)
{
	qot_sched sched;
	
	// prints the upcoming schedule's details
	if (ioctl(fd, QOT_GET_SCHED, &sched) != -1)
	{
		printf("Next schedule at %lu sec, %lu nsec \n", sched.event_time.tv_sec, sched.event_time.tv_nsec);
	}
}
*/


