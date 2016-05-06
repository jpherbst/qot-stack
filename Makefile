SHELL := /bin/bash

obj-m += qot_core.o
obj-m += qot_am335x.o

KERNELDIR ?= /export/bb-kernel/KERNEL
KERNELVER ?= 4.1.12-bone-rt-r16

all:
	dtc -O dtb -o BBB-AM335X-00A0.dtbo -b 0 -@ targets/am335x/BBB-AM335X-00A0.dts

clean:
	rm *.dtbo

install:
	sudo cp -v src/modules/qot/*.ko /export/rootfs/lib/modules/$(KERNELVER)/kernel/drivers/misc
	sudo cp -v src/modules/qot_am335x/*.ko /export/rootfs/lib/modules/$(KERNELVER)/kernel/drivers/misc
	sudo cp -v *.dtbo /export/rootfs/lib/firmware
	sudo cp -v ./src/qot-daemon/test.sh /export/rootfs/home
	sudo cp targets/common/80-qot.rules /export/rootfs/etc/udev/rules.d/
	sudo cp targets/am335x/capes /export/rootfs/usr/bin/
	sudo chmod 755 /export/rootfs/usr/bin/capes

reload:
	ssh root@172.17.11.0 depmod
	ssh root@172.17.11.0 ldconfig
