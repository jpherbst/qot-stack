#!/bin/bash

# usage:
#   ./bbbcontrol.sh [phc2phc|qotdaemon] [options]

USAGE="usage: ./bbbcontrol.sh [loadmodules|command] [options]"

IPADDRESSES="bbb-alpha bbb-bravo bbb-charlie"

echo "controlling ${IPADDRESSES}"

if [ $# -eq 0 ]
then
	echo $USAGE
	exit 1
elif [ $1 = "loadmodules" ]
then
	COMMAND="capes BBB-AM335X"
else
	COMMAND=$1
fi

shift # shift over args

for node in $IPADDRESSES
do
	(ssh root@$node "$COMMAND $* </dev/null >/dev/null 2>&1 &" >/dev/null 2>&1) &
	echo "running on $node with pid=$!"
	pids+=" $!"
done

for p in $pids
do
	if wait $p
	then
		echo "Process $p success"
	else
		echo "Process $p failure"
	fi
done
