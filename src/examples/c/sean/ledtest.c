// C headers
#include <stdio.h>
#include <unistd.h>

// qot header
#include "../../../api/c/qot.h"

//#define LED "/sys/class/leds/beaglebone\:green\:usr0/brightness"
#define LED1 "/sys/class/leds/beaglebone:green:usr1/brightness"
#define LED2 "/sys/class/leds/beaglebone:green:usr2/brightness"
#define LED3 "/sys/class/leds/beaglebone:green:usr3/brightness"

int main(int argc, char **argv)
{
	// open led file
	FILE *fp = fopen(LED3, "w");
	if (fp == NULL) {
		printf("failed to open\n");
		goto leave;
	}

	// turn on?
	if (fprintf(fp, "1") <= 0) {
		printf("failed to write 'on'\n");
		goto leave;
	}
	printf("turned on!\n");
	fclose(fp);

	sleep(3);

	fp = fopen(LED3, "w");
	if (fp == NULL) {
		printf("failed to open\n");
		goto leave;
	}

	// write off
	if (fprintf(fp, "0") <= 0) {
		printf("failed to write 'off'\n");
		goto leave;
	}
	printf("turned off!\n");

leave:
	fclose(fp);
	return 0;
}
