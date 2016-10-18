#!/bin/bash

# usage:
#   ./bbbcontrol.sh [phc2phc|qotdaemon] [options]

USAGE="usage: ./bbbcontrol.sh [loadmodules|command] [options]"

IPADDRESSES="bbb-alpha bbb-bravo bbb-charlie"

echo "controlling ${IPADDRESSES}"

# parse for command name
if [ $# -eq 0 ]
then
	# no command given
	echo $USAGE
	exit 1

elif [ $1 = "test" ]
then
	# just test ssh connection
	COMMAND="echo 'hello, world'"

elif [ $1 = "loadmodules" ]
then
	# load qot modules using capes
	COMMAND="capes BBB-AM335X"

else
	# not a recognized bbb command so just execute whatever was passed in
	COMMAND=$1
fi

shift # shift over args

for node in $IPADDRESSES
do
	# this line runs the command remotely and pipes all I/O so nothing hangs
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
