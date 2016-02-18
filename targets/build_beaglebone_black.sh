#!/bin/bash
pushd am335x
dtc -O dtb -o BBB-AM335X-00A0.dtbo -b 0 -@ BBB-AM335X-00A0.dts
scp BBB-AM335X-00A0.dtbo root@quadrotor_controller:/export/rootfs/lib/firmware
scp ../../src/modules/qot_am335x/qot_am335x.ko root@quadrotor_controller:/export/rootfs/lib/modules/4.1.12-bone-rt-r16/kernel/drivers/misc/
ssh root@bbb-bravo depmod