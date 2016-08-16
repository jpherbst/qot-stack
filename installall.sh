#!/bin/bash

IPADDRESSES="bbb-alpha bbb-bravo bbb-charlie"

for IP in $IPADDRESSES
do
	(make install_sd IPADDR=$IP >/dev/null 2>&1) &
	echo "install at IPADDR=$IP. background pid=$!"
	pids+=" $!"
done

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
