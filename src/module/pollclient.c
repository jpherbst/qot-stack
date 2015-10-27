#include <sys/ioctl.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <sys/poll.h>

#include "polltest.h"

int main(int argc, char* argv)
{
	struct pollfd pfd[1];
	int value;
	int fd = open("/dev/polltest", O_RDWR);
	if (fd < 0)
	{
		printf("Cannot open /dev/polltest\n");
		return -1;
	}
	for (int i = 0; i < 10; i++)
	{
		printf("Polling iteration %d\n", i+1);
		memset(pfd,0,sizeof(pfd));
		pfd[0].fd = fd;
		pfd[0].events = POLLIN;
		if (poll(pfd,1,10000))
		{
			if (pfd[0].revents & POLLIN)
			{
				printf("Poll received!\n");
					// Update this clock
				if (ioctl(fd, POLLTEST_GET_VALUE, &value) == 0)
					printf("-> value is %d\n",value);
				else
					printf("ioctl error!\n");
			}
			else
				printf("Poll timeout!\n");

		}
	}
	close(fd);
	return 0;
}