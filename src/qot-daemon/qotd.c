/*
 * @file helloworld.c
 * @brief QoT Clock Read Latency Estimation Daemon
 * @author Sandeep D'souza 
 * 
 * Copyright (c) Regents of the University of California, 2015. All rights reserved.
 * Copyright (c) Carnegie Mellon University, 2016.
 *
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *    1. Redistributions of source code must retain the above copyright notice, 
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, 
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <fcntl.h>

#include "../api/c/qot.h"

#define DEBUG 0

#define ONE_BILLION  1000000000ULL

#define HISTOGRAM_BINS 400
#define HISTOGRAM_RES 100ULL    // in ns

int histogram[HISTOGRAM_BINS];

int buckets = 1000;         // how many samples to collect per iteration
int iters = 50;           // how many iterations to run

uint64_t *timestamp;
uint64_t count = 0;
uint64_t sum = 0;
uint64_t sum2 = 0;
int64_t min = -1;
uint64_t max = 0;
double avg = 0;
double median = 0;
double stdev = 0;
utimelength_t uncertainity;


void deltaT(const char* name, uint64_t *x, int calc_hist) 
{
   // create vector with deltas between adjacent entries
   count = buckets -1;
   int i;
   double temp;
   int hist_index = 0;
   int hist_overflows = 0;

   sum = 0;
   sum2 = 0;
   min = -1;
   max = 0;
   avg = 0;
   median = 0;
   stdev = 0;

   // Initialize Histogram bins to zero
   if(calc_hist == 1)
   {
      for(i=0; i < HISTOGRAM_BINS; i++)
         histogram[i] = 0;
   }
   

   // Aggregate Stats and Histograms
   for (i = 0; i < count; ++i) 
   {
      x[i] = (x[i+1] - x[i]);
      if(calc_hist == 1)
      {
         hist_index = ((x[i]-1)/HISTOGRAM_RES);
         if(hist_index < HISTOGRAM_BINS && hist_index >= 0)
            histogram[hist_index]++;
         else if (hist_index >= 0)
            hist_overflows++;
      }
   }

   // Compute statistical moments
   for (i = 0; i < count; ++i) {
      sum    += x[i];
      sum2   += (x[i] * x[i]);
      if (x[i] > max) max = x[i];
      if ((min == -1) || (x[i] < min)) min = x[i];
   }

   avg = (double)sum / count;
   median = (double)min + ((double)(max - min) / 2);
   temp = (double)((count  * sum2 - (sum * sum)) / (count * count));
   stdev =  sqrt(temp);

   // Program the uncertainities -> Needs to be refined still primitive
   TL_FROM_nSEC(uncertainity.estimate, (sum/count)+1);
   TL_FROM_nSEC(uncertainity.interval.below, (uint64_t)stdev);
   TL_FROM_nSEC(uncertainity.interval.above, (uint64_t)stdev);
   printf("Calculated Uncertainity Latency = %llu, StdDev = %llu\n", (sum/count)+1, (uint64_t)stdev);
   if (DEBUG)
   {
      if(calc_hist == 1)
      {
         printf("Bin(ns)\tInstances\n");
         for(i=0; i < HISTOGRAM_BINS; i++)
         {
            if(histogram[i] != 0) 
              printf("%llu\t%d\n", (i+1)*HISTOGRAM_RES, histogram[i]);
         }
      }
      printf("Histogram Overflows: %d\n", hist_overflows);
   }
   return;
}

void print(const char* name)
{
   printf("%25s\t", "Method");
   printf("%s\t%7s\t%7s\t%7s\t%7s\t%7s\n", "samples", "min", "max", "avg", "median", "stdev");
   printf("%25s\t", name);
   printf("%llu\t%lld\t%llu\t%7.2f\t%7.2f\t%7.2f\n", count, min, max, avg, median, stdev);
}

int main(int argc, char** argv)
{
   qot_return_t retval;
   timepoint_t *est_now;
   struct timespec sleep_interval;
   int adm_file;
   int i, n;

   // Uncertainity Initialization -> Initialize to zero
   TL_FROM_SEC(uncertainity.estimate, 0);
   TL_FROM_SEC(uncertainity.interval.below, 0);
   TL_FROM_SEC(uncertainity.interval.above, 0);

   // Initialize Sleep Interval
   sleep_interval.tv_sec = 1;
   sleep_interval.tv_nsec = 0;

   // Grab iteration parameters
   if (argc > 1)
      iters = atoi(argv[1]);

   if (argc > 2)
      buckets = atoi(argv[2]);

   // Allocate Memory
   timestamp = (uint64_t*)malloc(buckets*sizeof(uint64_t));
   if(timestamp == NULL)
   {
      printf("Error: Could not allocate memory\n");
      return 0;
   }

   est_now = (timepoint_t*)malloc(buckets*sizeof(timepoint_t));
   if(est_now == NULL)
   {
      printf("Error: Could not allocate memory\n");
      return 0;
   }

   // Open the QoT Admin file 
   adm_file = open("/dev/qotadm", O_RDWR);

   // Initialize Uncertainity to 0
   if(ioctl(adm_file, QOTADM_SET_OS_LATENCY, &uncertainity) < 0)
   {
      return QOT_RETURN_TYPE_ERR;
   }

   while(1)
   {  
      // Polling Loop to read core time
      for (i = 0; i < iters * buckets; ++i) 
      {
         n = i % buckets;
         if(ioctl(adm_file, QOTADM_GET_CORE_TIME_RAW, &est_now[n]) < 0)
         {
            return QOT_RETURN_TYPE_ERR;
         }
      }

      // Data Conversion
      for (i = 0; i < buckets; ++i) 
      {
         timestamp[i] = TP_TO_nSEC(est_now[i]);
      }

      // Statistical Analysis
      deltaT("TIMELINE", timestamp, 1);
      
      if (DEBUG)
         print("TIMELINE");

      if(ioctl(adm_file, QOTADM_SET_OS_LATENCY, &uncertainity) < 0)
      {
         return QOT_RETURN_TYPE_ERR;
      }
      nanosleep(&sleep_interval, NULL);

   }
   
   // Close the qot_adm file
   if (adm_file)
      close(adm_file);

   // Free Memory
   free(timestamp);
   free(est_now);
   return 0;
}