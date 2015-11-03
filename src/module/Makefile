SHELL := /bin/bash

obj-m += qot_core.o
obj-m += qot_am335x.o
obj-m += pwm_test.o

KERNELDIR ?= ../../thirdparty/bb-kernel/KERNEL
KERNELVER ?= 4.1.12-bone-rt-r16
CROSSCOMP ?= ../dl/gcc-linaro-4.9-2015.02-3-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-

all:
	make -C $(KERNELDIR) M=$(PWD) ARCH=arm CROSS_COMPILE="${CROSSCOMP}" modules
	dtc -O dtb -o ROSELINE-QOT-00A0.dtbo -b 0 -@ ROSELINE-QOT-00A0.dts

clean:
	make -C $(KERNELDIR) M=$(PWD) ARCH=arm CROSS_COMPILE="${CROSSCOMP}" clean
	rm *.dtbo

install:
	sudo cp -v *.ko /export/rootfs/lib/modules/$(KERNELVER)/kernel/drivers
	sudo cp -v *.dtbo /export/rootfs/lib/firmware
	sudo cp 80-qot.rules /export/rootfs/etc/udev/rules.d/
	sudo cp capes /export/rootfs/usr/bin/
	sudo chmod 755 /export/rootfs/usr/bin/capes

reload:
	ssh root@10.42.0.108 depmod
	ssh root@10.42.0.108 ldconfig
