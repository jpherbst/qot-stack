#include <time.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

// Include the QoT API
#include "../api/qot.h"

// Main entry point of application
int main(int argc, char *argv[])
{
	// Intialize the QoT subsystem
	qot_init();

	// Bind to a timeline
	int32_t bid = qot_bind("MY_SPECIAL_TIMELINE", 1e6, 1e3);
	if (bid < 0)
	{
		printf("Could not bind to timeline\n");
		return 1;
	}

	// Get the POSIX clock associated with this timeline
	clockid_t cid;
	int32_t ret = qot_getclkid(bid, &cid);
	if (ret < 0)
	{
		printf("Could not obtain clock id\n");
		return 2;
	}

	// Iterate 100 times
	for (int i = 0; i < 100; i++)
	{
		// This will store the time
		struct timespec ts;

		// Get the time 
		clock_gettime(cid, &ts);

		// Print the global time
		printf("global time: %lld.%u\n", (long long) ts.tv_sec, (unsigned int) ts.tv_nsec);

		// Sleep for a second
		sleep(1);
	}

	// Clean up the qot subsystem
	qot_free();

	return 0;
}