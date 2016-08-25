#!/bin/bash

# usage:
# ./installall.sh [IP addresses..]
# if no argument, it installs on all IPADDRESSES variable

IPADDRESSES="bbb-alpha bbb-bravo bbb-charlie"

# If there are arguments then replace IPADDRESSES list
if [ $# -ne 0 ]
then
	IPADDRESSES=""
	for arg in "$@"
	do
		IPADDRESSES+=" $arg"
	done
	echo "IPADDRESSES=$IPADDRESSES"
fi

# Run in background for each IP address
for IP in $IPADDRESSES
do
	(make install_sd IPADDR=$IP >/dev/null 2>&1) &
	echo "install at IPADDR=$IP. background pid=$!"
	pids+=" $!"
done

# Wait for each install process, reporting return value
for p in $pids;
do
	if wait $p; then
		echo "Process $p success"
	else
		echo "Process $p fail"
	fi
done

wait

echo 'done'
