#include "QoTclock.h"

#include <sys/ioctl.h>

int fd;
const char *filename = "/dev/timeline";	// name tbd with scheduler

// ----------------- Functions ---------------------
uint16_t qot_init_clock(uint16_t timeline, struct timespec accuracy, struct timespec resolution)
{
	//open ioctl handle with the scheduler
	fd = open(filename, O_RDWR);
	
	qot_clock clk;
	clk.timeline = timeline;
	clk.accuracy = accuracy;
	clk.resolution = resolution;
	
	// add this clock to the qot clock list through scheduler
	if (ioctl(fd, QOT_INIT_CLOCK, &clk) == 0)
	{
		uint16_t binding_id;
		if(ioctl(fd, QOT_RETURN_BINDING, &binding_id) == 0)
			printf("Clock created with clock id %i and binding id %i\n", timeline, binding_id);
		else
			return -1;
	}
	
	return binding_id;
}

void qot_release_clock(uint16_t bindingId)
{
	// delete this clock from the qot clock list
	if (ioctl(fd, QOT_RELEASE_CLOCK, &bindingId) == 0)
	{
		printf("Clock released with binding id %i\n", bindingId);
	}
	
	close(fd);
}

void qot_set_accuracy(uint16_t bindingId, struct timespec accuracy)
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

void qot_set_resolution(uint16_t bindingId, struct timespec resolution)
{
	qot_clock clk;
	clk.resolution = resolution;
	clk.binding_id = bindingId;
	
	if (ioctl(fd, QOT_SET_RESOLUTION, &clk) == 0)
	{
		printf("Clock resolution updated %i\n", bindingId);
	}
}

void qot_wait_until(qot_clock clk, struct timespec eventtime)
{
	qot_sched sched;
	sched.clk_timeline = clk.timeline;
	sched.event_time = eventtime;
	
	if (ioctl(fd, QOT_WAIT, &sched) != -1)
	{
		printf("Wait till %lu sec, %lu nsec \n", sched.event_time.tv_sec, sched.event_time.tv_nsec);
	}
}

void qot_wait_until_cycles(qot_clock clk, uint64_t cycles)
{
	qot_sched sched;
	sched.clk_timeline = clk.timeline;
	sched.event_cycles = cycles;
	
	if (ioctl(fd, QOT_WAIT_CYCLES, &sched) != -1)
	{
		printf("Wait till %lu cycles\n", sched.event_cycles);
	}
}

void qot_schedule_event(qot_clock clk, struct timespec eventtime, struct timespec high, struct timespec low, uint16_t repetitions)
{
	qot_sched sched;
	sched.clk_timeline = clk.timeline;
	sched.event_time = eventtime;
	sched.high = high;
	sched.low = low;
	sched.limit = repetitions;
	
	if (ioctl(fd, QOT_SET_SCHED, &sched) != -1)
	{
		printf("Schedule set at %lu sec, %lu nsec \n", sched.event_time.tv_sec, sched.event_time.tv_nsec);
	}
}

struct timespec qot_get_next_event(qot_clock clk)
{
	qot_sched sched;
	
	// prints the upcoming schedule's details
	if (ioctl(fd, QOT_GET_SCHED, &sched) != -1)
	{
		printf("Next schedule at %lu sec, %lu nsec \n", sched.event_time.tv_sec, sched.event_time.tv_nsec);
	}
}


