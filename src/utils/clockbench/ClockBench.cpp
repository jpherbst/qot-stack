// Copyright 2014 by Bill Torpey. All Rights Reserved.
// This work is licensed under a Creative Commons Attribution-NonCommercial-NoDerivs 3.0 United States License.
// http://creativecommons.org/licenses/by-nc-nd/3.0/us/deed.en

#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <vector>
#include <cmath>

// Include the QoT API
extern "C"
{
#include "../../api/c/qot.h"
}

// Basic onfiguration
#define TIMELINE_UUID    "my_test_timeline"
#define APPLICATION_NAME "clockbench"

using namespace std;
#define ONE_BILLION  1000000000L

// NOTE: if you change this value you prob also need to adjust later code that does "& (BUCKETS-1);//0xff" to use modulo arithmetic instead 
const int BUCKETS = 1048576;         // how many samples to collect per iteration
const int ITERS = 100;           // how many iterations to run
double CPU_FREQ = 1;

inline unsigned long long cpuid_rdtsc() {
  unsigned int lo, hi;
  asm volatile (
     "cpuid \n"
     "rdtsc"
   : "=a"(lo), "=d"(hi) /* outputs */
   : "a"(0)             /* inputs */
   : "%ebx", "%ecx");     /* clobbers*/
  return ((unsigned long long)lo) | (((unsigned long long)hi) << 32);
}

inline unsigned long long rdtsc() {
  unsigned int lo, hi;
  asm volatile (
     "rdtsc"
   : "=a"(lo), "=d"(hi) /* outputs */
   : "a"(0)             /* inputs */
   : "%ebx", "%ecx");     /* clobbers*/
  return ((unsigned long long)lo) | (((unsigned long long)hi) << 32);
}

inline unsigned long long rdtscp() {
  unsigned int lo, hi;
  asm volatile (
     "rdtscp"
   : "=a"(lo), "=d"(hi) /* outputs */
   : "a"(0)             /* inputs */
   : "%ebx", "%ecx");     /* clobbers*/
  return ((unsigned long long)lo) | (((unsigned long long)hi) << 32);
}

// macro to call clock_gettime w/different values for CLOCK
#define do_clock(CLOCK) \
do { \
   for (int i = 0; i < BUCKETS; ++i) { \
      clock_gettime(CLOCK, &now[i]); \
   } \
   for (int i = 0; i < BUCKETS; ++i) { \
      timestamp[i] = (now[i].tv_sec * ONE_BILLION) + now[i].tv_nsec; \
   } \
   deltaT x(#CLOCK, timestamp); \
   x.print(); \
   x.dump(file); \
} while(0)



struct deltaT   {

   deltaT(const char* name, vector<long> v) : sum(0), sum2(0), min(-1), max(0), avg(0), median(0), stdev(0), name(name)
   {
      // create vector with deltas between adjacent entries
      x = v;
      count = x.size() -1;
      for (int i = 0; i < count; ++i) {
         x[i] = x[i+1] - x[i];
      }

      for (int i = 0; i < count; ++i) {
         sum    += x[i];
         sum2   += (x[i] * x[i]);
         if (x[i] > max) max = x[i];
         if ((min == -1) || (x[i] < min)) min = x[i];
      }

      avg = sum / count;
      median = min + ((max - min) / 2);
      stdev =  sqrt((count  * sum2 - (sum * sum)) / (count * count));
   }

   void print()
   {
      printf("%25s\t", name);
      printf("%ld\t%7.2f\t%7.2f\t%7.2f\t%7.2f\t%7.2f\n", count, min, max, avg, median, stdev);
   }

   void dump(FILE* file)
   {
      if (file == NULL)
         return;

      fprintf(file, "%25s,", name);
      // for (int i = 0 ; i < count; ++i ) {
      //    fprintf(file, "\t%ld", x[i]);
      // }
      fprintf(file,"%ld,%7.2f,%7.2f,%7.2f,%7.2f,%7.2f\n", count, min, max, avg, median, stdev);
      //fprintf(file, "\n");
   }

   vector<long> x;
   long   count;
   double sum;
   double sum2;
   double min;
   double max;
   double avg;
   double median;
   double stdev;
   const char* name;
};

int main(int argc, char** argv)
{
   if (argc > 1) {
      CPU_FREQ = strtod(argv[1], NULL);
   }

   FILE* file = NULL;

   if (argc > 2) {
      file = fopen(argv[2], "w");
   }

   // Grab the timeline
   const char *u = TIMELINE_UUID;

   // Grab the application name
   const char *m = APPLICATION_NAME;

   timeline_t *my_timeline;
   qot_return_t retval;
   timelength_t resolution;
   timeinterval_t accuracy;

	// Accuracy and resolution values
	resolution.sec = 0;
	resolution.asec = 1e9;

	accuracy.below.sec = 0;
	accuracy.below.asec = 1e12;
	accuracy.above.sec = 0;
	accuracy.above.asec = 1e12;

   my_timeline = timeline_t_create();
   if(!my_timeline)
   {
    	printf("Unable to create the timeline_t data structure\n");
		return QOT_RETURN_TYPE_ERR;
   }

   // Bind to a timeline
   printf("Binding to timeline %s ........\n", u);
   if(timeline_bind(my_timeline, u, m, resolution, accuracy))
   {
		printf("Failed to bind to timeline %s\n", u);
		timeline_t_destroy(my_timeline);
		return QOT_RETURN_TYPE_ERR;
   }

   vector<long> timestamp;
   vector<struct timespec> now;
   vector<utimepoint_t> now_tp;
   now_tp.resize(BUCKETS);
   now.resize(BUCKETS);
   timestamp.resize(BUCKETS);

   printf("%25s\t", "Method");
   printf("%s\t%7s\t%7s\t%7s\t%7s\t%7s\n", "samples", "min", "max", "avg", "median", "stdev");

   for (int j = 0; j < ITERS; j++)
   {
      #if _POSIX_TIMERS > 0
      #ifdef CLOCK_REALTIME
      do_clock(CLOCK_REALTIME);
      #endif

      // #ifdef CLOCK_REALTIME_COARSE
      // do_clock(CLOCK_REALTIME_COARSE);
      // #endif

      // #ifdef CLOCK_REALTIME_HR
      // do_clock(CLOCK_REALTIME_HR);
      // #endif

      #ifdef CLOCK_MONOTONIC
      do_clock(CLOCK_MONOTONIC);
      #endif

      // #ifdef CLOCK_MONOTONIC_RAW
      // do_clock(CLOCK_MONOTONIC_RAW);
      // #endif

      // #ifdef CLOCK_MONOTONIC_COARSE
      // do_clock(CLOCK_MONOTONIC_COARSE);
      // #endif
      #endif


      // {
      //    for (int i = 0; i < BUCKETS; ++i) {
      //       //int n = i & (BUCKETS-1);//0xff;
      //       timestamp[i] = cpuid_rdtsc();
      //    }
      //    for (int i = 0; i < BUCKETS; ++i) {
      //       timestamp[i] = (long) ((double) timestamp[i] / CPU_FREQ);
      //    }
      //    deltaT x("cpuid+rdtsc", timestamp);
      //    x.print();
      //    x.dump(file);
      // }

      // // will throw SIGILL on machine w/o rdtscp instruction
      // #ifdef RDTSCP
      // {
      //    for (int i = 0; i < BUCKETS; ++i) {
      //       timestamp[i] = rdtscp();
      //    }
      //    for (int i = 0; i < BUCKETS; ++i) {
      //       timestamp[i] = (long) ((double) timestamp[i] / CPU_FREQ);
      //    }
      //    deltaT x("rdtscp", timestamp);
      //    x.print();
      //    x.dump(file);
      // }
      // #endif

      {
         for (int i = 0; i < BUCKETS; ++i) {
            timestamp[i] = rdtsc();
         }
         for (int i = 0; i < BUCKETS; ++i) {
            timestamp[i] = (long) ((double) timestamp[i] / CPU_FREQ);
         }
         deltaT x("rdtsc", timestamp);
         x.print();
         x.dump(file);
      }

      {
         for (int i = 0; i < BUCKETS; ++i) {
            timeline_gettime(my_timeline, &now_tp[i]);
         }
         for (int i = 0; i < BUCKETS; ++i) {
            timestamp[i] = (long) floor(now_tp[i].estimate.sec*ONE_BILLION + now_tp[i].estimate.asec/ONE_BILLION);
         }
         deltaT x("QOT_CORE", timestamp);
         x.print();
         x.dump(file);
      }
   }

   printf("Using CPU frequency = %f\n", CPU_FREQ);

   // Unbind from timeline
   if(timeline_unbind(my_timeline))
   {
		printf("Failed to unbind from timeline %s\n", u);
		timeline_t_destroy(my_timeline);
		return QOT_RETURN_TYPE_ERR;
   }
   printf("Unbound from timeline %s\n", u);

   // Free the timeline data structure
   timeline_t_destroy(my_timeline);

   return 0;
}
