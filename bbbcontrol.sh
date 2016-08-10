#!/bin/bash

# usage:
#   ./bbbcontrol.sh [phc2phc|qotdaemon] [options]

USAGE="usage: ./bbbcontrol.sh [command] [options]"

IPADDRESSES="bbb-alpha bbb-bravo bbb-charlie"

echo "controlling ${IPADDRESSES}"

if [ $# -eq 0 ]
then
	echo $USAGE
	exit 1
fi

COMMAND=$1

shift # shift over args

for node in $IPADDRESSES
do
	echo $node
	ssh root@$node "$COMMAND $* </dev/null >/dev/null 2>&1 &" # redirect all i/o to /dev/null and start in background
done
