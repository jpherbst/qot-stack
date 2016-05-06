#!/bin/bash
NUM_THREADS=10 
TIMELINE_RANGE=10
LOW_STEP_SIZE=200
UP_STEP_SIZE=1000
# Start Applications
for (( c=1; c<=$NUM_THREADS; c++ ))
do
   TIMELINE=$RANDOM
   MIN_STEP_SIZE=$RANDOM
   PRIO=$RANDOM
   let "TIMELINE = $TIMELINE % $TIMELINE_RANGE"
   let "MIN_STEP_SIZE = $MIN_STEP_SIZE % (UP_STEP_SIZE-LOW_STEP_SIZE) + LOW_STEP_SIZE"
   let "PRIO = ($PRIO % 20) + 1"
   echo "starting helloworld_random $TIMELINE $c $MIN_STEP_SIZE with priority $PRIO"
   chrt $PRIO helloworld_random $TIMELINE $c $MIN_STEP_SIZE &
   sleep 0.$[ ( $RANDOM % 500 )  + 1 ]
done
# run the qot latency daemon
echo "starting the qot latency daemon"
./qotd
echo "Kill the Applications"
pkill -SIGINT -f random


          
