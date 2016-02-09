SHELL := /bin/bash

obj-m += qot_core.o
#obj-m += qot_am335x.o
#obj-m += qot_timeline.o
#obj-m += chronos.o
#obj-m += pwm_test.o

KERNELDIR ?= /export/bb-kernel/KERNEL
KERNELVER ?= 4.1.12-bone-rt-r16
CROSSCOMP ?= ../dl/gcc-linaro-4.9-2015.02-3-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-
IP_SERVER ?= root@172.17.5.7
IP_CLIENT ?= root@172.17.11.11

all:
	make -C $(KERNELDIR) M=$(PWD) ARCH=arm CROSS_COMPILE="${CROSSCOMP}" modules

overlays:
	dtc -O dtb -o ROSELINE-QOT-00A0.dtbo -b 0 -@ ROSELINE-QOT-00A0.dts
	dtc -O dtb -o ROSELINE-QOT-ATMEL-00A0.dtbo -b 0 -@ ROSELINE-QOT-ATMEL-00A0.dts
	dtc -O dtb -o ROSELINE-QOT-DECAWAVE-00A0.dtbo -b 0 -@ ROSELINE-QOT-DECAWAVE-00A0.dts
	dtc -O dtb -o ROSELINE-QOT-CHRONOS-00A0.dtbo -b 0 -@ ROSELINE-QOT-CHRONOS-00A0.dts

clean:
	make -C $(KERNELDIR) M=$(PWD) ARCH=arm CROSS_COMPILE="${CROSSCOMP}" clean
	rm *.dtbo

install:
	chmod 755 capes
	scp *.ko $(IP_SERVER):/export/rootfs/lib/modules/$(KERNELVER)/kernel/drivers/misc
	scp *.dtbo $(IP_SERVER):/export/rootfs/lib/firmware
	scp 80-qot.rules $(IP_SERVER):/export/rootfs/etc/udev/rules.d/
	scp capes testptp phc2phc $(IP_SERVER):/export/rootfs/usr/bin/

reload:
	ssh $(IP_CLIENT) depmod
	ssh $(IP_CLIENT) ldconfig
	ssh $(IP_CLIENT) reboot
	sleep 20
	ssh $(IP_CLIENT)

client:
	ssh $(IP_CLIENT)

server:
	ssh $(IP_SERVER)

kill:
	ssh $(IP_SERVER) ntbkill

spawn:
	ssh $(IP_SERVER) ntbspawn

minicom:
	minicom

apps:
	$(KERNELDIR)/${CROSSCOMP}gcc -o testptp testptp.c -lrt
	$(KERNELDIR)/${CROSSCOMP}gcc -o phc2phc -Ilinuxptp-code \
		-D_GNU_SOURCE -DHAVE_CLOCK_ADJTIME \
		-DHAVE_POSIX_SPAWN -DHAVE_ONESTEP_SYNC \
		linuxptp-code/sk.c \
		linuxptp-code/util.c \
		linuxptp-code/config.c \
		linuxptp-code/hash.c \
		linuxptp-code/nullf.c \
		linuxptp-code/pi.c \
		linuxptp-code/ntpshm.c \
		linuxptp-code/linreg.c \
		linuxptp-code/print.c \
		linuxptp-code/servo.c \
		linuxptp-code/clockadj.c \
		phc2phc.c \
		-lrt -lm