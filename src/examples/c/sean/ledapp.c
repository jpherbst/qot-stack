// c headers
#include <stdio.h>
#include <stdint.h>
#include <signal.h>

// qot header
#include "../../../api/c/qot.h"

// printing for debugging; basically print if DEBUG is 1, nothing o.w.
#define DEBUG 1
#define debugprintf(...) (DEBUG ? printf(__VA_ARGS__) : 0)

// 1=running, 0=not
static int running = 1;

// number of nodes in the setup
static const int NNODES = 6;

// constants for qot stuff
#define TIMELINE_UUID    "my_timeline0"
#define APPLICATION_NAME "default0"

#define TIMELINEX 11
#define APPNAMEX   7

// turn led on/off
// we write these strings to the led 'brightness' device to turn them on/off
static const char * const LED_OFF = "0";
static const char * const LED_ON  = "1";

// index into led_files array
static const int USR0 = 0;
static const int USR1 = 1;
static const int USR2 = 2;
static const int USR3 = 3;

// names of led brightness files
// LED_FILES[0] gives the USR0 led, and so on
static const char * const LED_FILES[] = {
	"/sys/class/leds/beaglebone:green:usr0/brightness",
	"/sys/class/leds/beaglebone:green:usr1/brightness",
	"/sys/class/leds/beaglebone:green:usr2/brightness",
	"/sys/class/leds/beaglebone:green:usr3/brightness"
};

// gpio pin value
static const int PIN_LOW  = 0;
static const int PIN_HIGH = 1;

// GPIO44 pin for reading
static const char * const GPIO44 = "/sys/class/gpio/gpio44/value";

// time for led to stay on
// set to 0.5 seconds
static const int LED_TIME_ON = 500; // ms

// Period of cycle
static const uint64_t PERIOD = 1000000; // us (micro)

// for SIGINT
static void exit_handler(int s)
{
	printf("Exit requested\n");
	running = 0;
}

// fcn decs
int wait_for_start_signal(void);
int my_sleep(timeline_t *tml, utimelength_t *utl);
int led_ctl(int led, const char * const code);
void print_utimepoint(const utimepoint_t *t);


int main(int argc, char **argv)
{
	// Variable declaration
	int ret = 0;
	qot_return_t qot_ret = 0;

	// set qot stuff params
	char uuid[] = TIMELINE_UUID;
	char app_name[] = APPLICATION_NAME;
	timelength_t res = { .sec = 0, .asec = 1e9 };
	timeinterval_t acc = { .below.sec = 0, .below.asec = 1e12, 
	                       .above.sec = 0, .above.asec = 1e12 };
	utimelength_t led_time_on, periodtl;
	TL_FROM_mSEC(led_time_on.estimate, LED_TIME_ON);
	TL_FROM_uSEC(periodtl.estimate, PERIOD);

	debugprintf("led_time_on: %d\n", LED_TIME_ON);
	debugprintf("periodtl:    %llu\n", PERIOD);

	// parse args

	// read in slot
	int slot = 0;
	if (argc < 2) {
		printf("Requires argument.\n\
ledapp [slot]\n");
		return 1;
	}

	// parse for slot
	if (argc >= 2) {
		slot = atoi(argv[1]);
		argc--, argv++;
	}
	if (slot >= NNODES) { // not allowed
		printf("slot arg greater than allowed\n");
		return 1;
	}
	debugprintf("my slot is %d\n", slot);

	// calculate slot offset
	utimelength_t slot_offset;
	TL_FROM_uSEC(slot_offset.estimate, slot * (PERIOD / NNODES));

	// parse for timeline#
	if (argc >= 2) {
		uuid[TIMELINEX] = argv[1][0];
		app_name[APPNAMEX] = argv[1][0];
		argc--, argv++;
	}
	debugprintf("timeline uuid: %s\n", uuid);


	// create
	debugprintf("creating timeline..\n");
	timeline_t *my_tml = timeline_t_create();
	if (my_tml == NULL) {
		printf("Unable to create the timeline_t data structure\n");
		ret = 1;
		goto exit_destroy;
	}
	debugprintf("  =>created\n");

	// bind
	debugprintf("Binding to timeline %s..\n", uuid);
	if (timeline_bind(my_tml, uuid, app_name, res, acc)) {
		printf("Failed to bind to timeline %s\n", uuid);
		ret = 1;
		goto exit_destroy;
	}
	debugprintf("  => bound\n");

	// read init time
	utimepoint_t uwake;
	debugprintf("Getting time..\n");
	if (timeline_gettime(my_tml, &uwake)) {
		printf("Could not read timeline reference time\n");
		ret = 1;
		goto exit_unbind;
	}
	debugprintf("  => got\n");
	print_utimepoint(&uwake);

	// success; register exit_handler
	signal(SIGINT, exit_handler);


	//////////////////////////////////////////////
	// timeline created and bound at this point.
	//////////////////////////////////////////////


	// block until we get signal to start
	debugprintf("Waiting for start signal..\n");
	if (ret = wait_for_start_signal()) {
			printf("error waiting for start signal\n");
			goto exit_unbind;
	}

	// get and print time at reception of start signal
	debugprintf("got start signal. getting time..\n");
	if (timeline_gettime(my_tml, &uwake)) {
		printf("Could not read timeline reference time\n");
		ret = 1;
		goto exit_unbind;
	}
	debugprintf("  => got\n");

	print_utimepoint(&uwake);

	// wait for my slot
	debugprintf("waiting for my slot..\n");
	if (ret = my_sleep(my_tml, &slot_offset)) {
		printf("my_sleep failed\n");
		goto exit_unbind;
	}

	// main loop
	debugprintf("starting blinking\n");
	utimepoint_t time_now, led_off, next_slot;
	next_slot.interval.below.sec = 0; next_slot.interval.below.asec = 1e12;
	next_slot.interval.above.sec = 0; next_slot.interval.above.asec = 1e12;
	led_off.interval = next_slot.interval;
	while (running) {
		debugprintf("getting time\n");
		if (timeline_gettime(my_tml, &time_now)) {
			printf("Could not get time\n");
			ret = 1;
			goto exit_unbind;
		}
		led_off = time_now;

		// calculate timepoint
		timepoint_add(&(led_off.estimate), &(led_time_on.estimate));

		debugprintf("turning leds on\n");
		// turn all leds on (except heartbeat led USR0)
		led_ctl(USR3, LED_ON);
		led_ctl(USR2, LED_ON);
		led_ctl(USR1, LED_ON);
		//debugprintf("\b\b\b\b\b\b\b\b\b\bturned on \b");
		fflush(stdout);

		// wait until we have to turn led off
		debugprintf("waiting until ledoff\n");
		if (timeline_waituntil(my_tml, &led_off)) {
			printf("timeline_waituntil() failed\n");
			ret = 1;
			goto exit_unbind;
		}

		debugprintf("turning ledoff\n");
		led_ctl(USR3, LED_OFF);
		led_ctl(USR2, LED_OFF);
		led_ctl(USR1, LED_OFF);
		//debugprintf("\b\b\b\b\b\b\b\b\b\bturned off");
		fflush(stdout);

		debugprintf("getting time\n");
		if (timeline_gettime(my_tml, &time_now)) {
			printf("Could not get time\n");
			ret = 1;
			goto exit_unbind;
		}
		led_off = next_slot = time_now;

		// calculate timepoint
		timepoint_add(&(next_slot.estimate), &(slot_offset.estimate));

		// wait til next period
		debugprintf("waiting for next period\n");
		if (timeline_waituntil(my_tml, &next_slot)) {
			printf("waituntil failed\n");
			goto exit_unbind;
		}
	}
	debugprintf("\nstopped blinking\n");


	// cleanup
exit_unbind:
	debugprintf("Unbinding..\n");
	if (timeline_unbind(my_tml))
		printf("Failed to unbind from timeline %s\n", uuid);
	debugprintf("  => unbinded\n");

exit_destroy:
	debugprintf("Destroying timeline..\n");
	timeline_t_destroy(my_tml);
	debugprintf("  => destroyed\n");

exit_for_real:
	return qot_ret;
}

// reads 1 char from gpio pin and returns value. (one of {0,1}) or -1 if error
int read_gpio()
{
	//debugprintf("reading gpio\n");
	FILE *pin_fp = fopen(GPIO44, "r");
	if (pin_fp == NULL) {
		//printf("Error opening GPIO file\n");
		return -1;
	}

	char pin_state;
	// read one char from the pin file
	fread(&pin_state, sizeof(char), 1, pin_fp);

	fclose(pin_fp);

	if (pin_state == '0')
		return PIN_LOW;
	else if (pin_state == '1')
		return PIN_HIGH;
	else {
		printf("pin_state was not '0' or '1'");
		return -1;
	}
}

static const char LOADING_CHAR[] = "|/-\\";

// wait for signal from button/GPIO pin
int wait_for_start_signal()
{
	// spin until input pin goes high
	int ret, i;
	i = 0;

	while (1) {
		debugprintf("\b%c", LOADING_CHAR[i]);

		if ((ret = read_gpio()) == PIN_HIGH)
			break;
		else if (ret == -1 || !running)
			return -1;

		if (i < 3)
			i++;
		else
			i = 0;
	}

	debugprintf("\n");
	return 0;
}


// my sleep
int my_sleep(timeline_t *tml, utimelength_t *utl)
{
	// get current time
	utimepoint_t t;
	if (timeline_gettime(tml, &t)) {
		printf("my_sleep: Could not get time\n");
		return 1;
	}
	// add length of time we want to sleep
	timepoint_add(&(t.estimate), &(utl->estimate));

	// wait
	if (timeline_waituntil(tml, &t)) {
		printf("my_sleep: waituntil failed\n");
		return 1;
	}

	return 0;
}


// turn on led
// ret: 0 on success, 1 on error
int led_ctl(int led, const char * const code)
{
	int ret = 0;
	FILE *fp = fopen(LED_FILES[led], "w");
	if (fp == NULL) {
		printf("failed to open\n");
		ret = 1;
		goto leave;
	}

	if (fprintf(fp, "%s", code) <= 0) {
		printf("failed to write '%s'\n", (code == LED_ON ? "on" : "off"));
		ret = 1;
		goto leave;
	}

leave:
	fclose(fp);	
	return ret;
}

void print_utimepoint(const utimepoint_t *t)
{
	printf("est: %lli.%llu\n", t->estimate.sec, t->estimate.asec);
	printf("unc: %llu.%llu below, %llu.%llu above\n",
	  t->interval.below.sec, t->interval.below.asec,
	  t->interval.above.sec, t->interval.above.asec);
}
