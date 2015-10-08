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
	int32_t bid = qot_bind_timeline("MY_UNIQUE_ID", 1e6, 1e3);
	if (bid < 0)
	{
		printf("Could not bind to timeline\n");
		return 1;
	}

	// Iterate 100 times
	for (int i = 0; i < 100; i++)
	{
		// This will store the time
		struct timespec ts;
		uint64_t res, acc;

		// Get the time 
		ret = qot_gettime(bid, &ts, &acc, &res);
		if (ret < 0)
		{
			printf("Could not get the time\n");
			return 2;
		}

		// Increment by one second
		ts.tv_sec += 1;

		// Print the global time
		printf("Global time: %lld.%u\n (acc: %llu, res: %llu)\n", 
			(long long) ts.tv_sec, (unsigned int) ts.tv_nsec, 
			(unsigned long long) acc, (unsigned long long)res);

		// Sleep 
		qot_wait_until(bid, &ts);
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