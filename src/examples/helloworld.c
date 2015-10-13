#include <time.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

// Include the QoT API
#include "../api/qot.h"

// Main entry point of application
int main(int argc, char *argv[])
{
	// Bind to a timeline
	int32_t ret;
	int32_t bid = qot_bind_timeline("MY_UNIQUE_TIMELINE_UUID", 1e6, 1e3);
	if (bid < 0)
	{
		printf("Could not bind to timeline: %d\n",bid);
		return 1;
	}

	// Iterate 100 times
	for (int i = 0; i < 10; i++)
	{
		// This will store the time
		struct timespec ts;

		// Get the time 
		ret = qot_gettime(bid, &ts);
		if (ret)
		{
			printf("Could not get the time: %d\n",ret);
			return 2;
		}

		// Increment by one second
		ts.tv_sec += 1;

		// Print the global time
		printf("Global time: %lld.%u\n\n", 
			(long long) ts.tv_sec, (unsigned int) ts.tv_nsec);

		// Sleep for a second
		usleep(1000000);
	}

	// Unbind from timeline
	ret = qot_unbind_timeline(bid);
	if (ret < 0)
	{
		printf("Could not unbind from timeline\n");
		return 1;
	}

	return 0;
}