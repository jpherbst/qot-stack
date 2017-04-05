/* Read timestamp from GPS. Reference: derek molley    */

#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/timex.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <termios.h> // for UART

#include <linux/ptp_clock.h>// for ptp pin

/* Add servo and clock adjustment code from LinuxPTP */
#include "linuxptp/servo.h"
#include "linuxptp/clockadj.h"

#include "gps.h"

/* Useful definitions */
#define NSEC_PER_SEC  ((int64_t)1000000000)
#define NSEC_PER_MSEC ((int64_t)1000000)
#define FD_TO_CLOCKID(fd) ((~(clockid_t) (fd) << 3) | 3)
#ifndef CLOCK_INVALID
#define CLOCK_INVALID -1
#endif

// #define PTP_PIN_SETFUNC    _IOW(PTP_CLK_MAGIC, 7, struct ptp_pin_desc)
// struct ptp_pin_desc {
//     /*
//      * Hardware specific human readable pin name. This field is
//      * set by the kernel during the PTP_PIN_GETFUNC ioctl and is
//      * ignored for the PTP_PIN_SETFUNC ioctl.
//      */
//     char name[64];
//     /*
//      * Pin index in the range of zero to ptp_clock_caps.n_pins - 1.
//      */
//     unsigned int index;
//     /*
//      * Which of the PTP_PF_xxx functions to use on this pin.
//      */
//     unsigned int func;
//     /*
//      * The specific channel to use for this function.
//      * This corresponds to the 'index' field of the
//      * PTP_EXTTS_REQUEST and PTP_PEROUT_REQUEST ioctls.
//      */
//     unsigned int chan;
//     /*
//      * Reserved for future use.
//      */
//     unsigned int rsv[5];
// };


static int running = 1;

static void exit_handler(int s)
{
   printf("Exit requested \n");
   running = 0;
}

int main(int argc, char *argv[]){
   // Set up uart and ptp
   printf("Program starts!\n");

/*   printf("Set up uart4 ...\n");
   int uart_fd, count;
   if ((uart_fd = open("/dev/ttyO4", O_RDWR | O_NOCTTY | O_NDELAY))<0){
      perror("UART: Failed to open the file.\n");
      return -1;
   }

   struct termios options;
   tcgetattr(uart_fd, &options);
   options.c_cflag = B9600 | CS8 | CREAD | CLOCAL;
   options.c_iflag = IGNPAR | ICRNL;
   tcflush(uart_fd, TCIFLUSH);
   tcsetattr(uart_fd, TCSANOW, &options);
   printf("Set uart4 success\n");
*/
   char *device_m = "/dev/ptp1";               /* PTP device */
   int index_m = 1;                            /* Channel index, '1' corresponds to 'TIMER6' */
   int fd_m;                                   /* device file descriptor */
   clockid_t clkid;
	
   int c, cnt;
   struct ptp_clock_caps caps;                 /* Clock capabilities */
   struct ptp_pin_desc desc;                   /* Pin configuration */
   struct ptp_extts_event event;               /* PTP event */
   struct ptp_extts_request extts_request;     /* External timestamp req */
	
	/* How we plan to discipline the clock */
  printf("Plan to discipline the clock ...\n");
	int max_ppb;
	double ppb, weight;
	uint64_t local_ts;
	int64_t offset;
	struct servo *servo;

  struct timespec tmspec;
  uint64_t gpstime;

  printf("Set up ptp pin ...\n");
  /* Open the character device */
  fd_m = open(device_m, O_RDWR);
  if (fd_m < 0) {
    fprintf(stderr, "opening device %s: %s\n", device_m, strerror(errno));
    return -1;
  }
  printf("Device opened %d\n", fd_m);
  memset(&desc, 0, sizeof(desc));
  desc.index = index_m;
  desc.func = 1;              // '1' corresponds to external timestamp
  desc.chan = index_m;
  if (ioctl(fd_m, PTP_PIN_SETFUNC, &desc)) {
    printf("Set pin func failed for %d\n", fd_m);
    return -1;
  }
  printf("Set pin func succees for %d\n", fd_m);

  /* Get Clock ID for the processor clock */
  clkid = FD_TO_CLOCKID(fd_m);
  if (CLOCK_INVALID == clkid) {
    printf("error: failed to read clock id\n");
    return -1;
  }
  printf("Clkid successfully got as %d\n", clkid);

  // Request timestamps from the pin
  memset(&extts_request, 0, sizeof(extts_request));
  extts_request.index = index_m;
  extts_request.flags = PTP_ENABLE_FEATURE | PTP_RISING_EDGE;
  if (ioctl(fd_m, PTP_EXTTS_REQUEST, &extts_request)) {
    printf("Requesting timestamps failed for %d\n", fd_m);
    return -1;
  }
  printf("Requesting timestamps success for %d\n", fd_m);
/******* Setup Linuxptp for processor clock adjustment *******/
	
  printf("Setup Linuxptp for processor clock adjustment...\n");
	/* Determine slave max frequency adjustment */
	if (ioctl(fd_m, PTP_CLOCK_GETCAPS, &caps)) {
		printf("error: cannot get capabilities\n");
	}
	max_ppb = caps.max_adj;
	if (!max_ppb) {
		printf("error: clock is not adjustable\n");
	}
	printf("MAX processor adj freq %i\n", max_ppb);

	/* Initialize clock discipline */
	clockadj_init(clkid);
	
	/* Get the current ppb error */
	ppb = clockadj_get_freq(clkid);
	printf("processor freq %+7.0f\n", ppb);

	/* Create a servo */
	servo = servo_create(
						 //cfg,			/* Servo configuration */
						 CLOCK_SERVO_PI,	/* Servo type */
						 -ppb,			/* Current frequency adjustment */
						 max_ppb, 		/* Max frequency adjustment */
						 0				/* 0: hardware, 1: software */
						 );
	
	/* Set the servo sync interval (in fractional seconds) */
	servo_sync_interval(
						servo,
						1
            //ptp_clock_double(&perout_request.period)
						);
	
	/* This will save the servo state */
	enum servo_state state;
  double dmax, dmin;
/***********************************************************/

	 printf("Adjustment begins...\n");
   signal(SIGINT, exit_handler);
   while (running) {
      printf("\nTrying to read events %d\n", running++);

        /* Read events coming in */
        cnt = read(fd_m, &event, sizeof(event));
        if (cnt != sizeof(event)) {
           printf("Cannot read event");
           break;
        }

        clock_gettime(CLOCK_REALTIME, &tmspec);

        printf("System Time: %lld.%09u\n", (long long)tmspec.tv_sec, tmspec.tv_nsec);
        printf("Core Time: %lld.%09u\n", event.t.sec, event.t.nsec);

         uint64_t ptp_ts = event.t.sec * NSEC_PER_SEC +  event.t.nsec;
         gpstime = tmspec.tv_sec * NSEC_PER_SEC + tmspec.tv_nsec;
		  
    		 int64_t tmp = ((int64_t) event.t.sec -(int64_t) tmspec.tv_sec) * NSEC_PER_SEC;
         tmp += ((int64_t) event.t.nsec - (int64_t) tmspec.tv_nsec);

    		 printf("Offset: %lld\n", tmp);

    		 /*** TIME SYNCHRONIZATION: ALIGN PROCESSOR CLOCK TO GPS CLOCK ***/
    		  /* Local timestamp and offset */
    		  local_ts = gpstime;
    		  offset = tmp;
    		  weight = 1.0;
    		  
    		  /* Update the clock */
    		  printf("Update the clock...\n");

              /* Update the clock */
          ppb = servo_sample(
              CLOCK_SERVO_PI,
              servo,      /* Servo object */
              offset,     /* T(slave) - T(master) */
              local_ts,     /* T(master) */
              //weight,     /* Weighting */
              &state,       /* Next state */
              &dmax,
              &dmin
            );

    		  printf("ppb:%+7.0f\n", ppb);

    		  /* What we do depends on the servo state */
    		   switch (state) {
    			   case SERVO_UNLOCKED:
              printf("SERVO_UNLOCKED...\n");
    				  break;
    			   case SERVO_JUMP:
              printf("SERVO_JUMP...\n");
    				  clockadj_step(clkid, -offset);
              break;
    			   case SERVO_LOCKED:
              printf("SERVO_LOCKED... \n");
    				  clockadj_set_freq(clkid, -ppb);
    				  break;
    		   }

        /* Demo of invoking output compare functionality */
           struct timespec start;
           start.tv_sec = 0;
           start.tv_nsec = 100000234; // Start 100000234 nanoseconds after the current Core Time

           int period_msec = 1000; // 1000 millisecond = 1 second
           int duty = 50; // 50 % duty cycle

           struct timespec stop;
           clock_gettime(CLOCK_REALTIME, &tmspec);
           stop.tv_sec = tmspec.tv_sec + 10; // Stop 10 seconds after the current System (GPS) Time
           stop.tv_nsec = 0;

           if(gps_compare(start, period_msec, duty, QOT_TRIGGER_RISING, stop)){
              running = 0; // Error happened, exit
           }
      }

	/* Destroy the servo */
	servo_destroy(servo);

   /* Disable the pin */
   memset(&desc, 0, sizeof(desc));
   desc.index = index_m;
   desc.func = 0;              // '0' corresponds to no function
   desc.chan = index_m;
   if (ioctl(fd_m, PTP_PIN_SETFUNC, &desc)) {
      printf("Disable pin func failed for %d\n", fd_m);
   }
   
   /* Close the character device */
   close(fd_m);

   printf("Program finishes!\n");
   return 0;
}
