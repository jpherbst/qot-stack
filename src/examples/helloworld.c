#include <stdint.h>
#include <stdio.h>
#include <time.h>

// Include the QoT API
#include "../api/qot.h"

// Main entry point of application
int main(int argc, char *argv[])
{
	// This will store the time
	struct timespec ts;
	uint32_t bid, cid;

	// Intialize the QoT subsystem
	qot_init();

	// Bind to a timeline
	bid = qot_bind("MY_SPECIAL_TIMELINE")
	if (bid < 0)
	{
		printf("Could not bind to timeline\n");
		return 1;
	}

	// Get the POSIX clock associated with this timeline
	int cid = qot_getclkid(bid);
	if (cid < 0)
	{
		printf("Could not obtain clock id\n");
		return 2;
	}

	// Iterate 100 times
	for (int i = 0; i < 100; i++)
	{
		// Get the time 
		clock_gettime(clkid, &ts);

		// Print the global time
		printf("global time: %lld.%u\n", ts->sec, ts->nsec);

		// Sleep for a second
		sleep(1);
	}

	// Clean up the qot subsystem
	qot_free();

	return 0;
}