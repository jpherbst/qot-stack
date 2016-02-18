#!/bin/bash
if [ "$#" -ne 1 ]; then
	echo "usage: build_beaglebone_black.sh root@host"
	exit 1
fi
pushd am335x
dtc -O dtb -o BBB-AM335X-00A0.dtbo -b 0 -@ BBB-AM335X-00A0.dts
scp BBB-AM335X-00A0.dtbo $1:/lib/firmware
scp ../../src/modules/qot_am335x/qot_am335x.ko $1:/lib/modules/4.1.12-bone-rt-r16/kernel/drivers/misc/
ssh $1 depmod