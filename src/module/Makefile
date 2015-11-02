obj-m += qot_core.o
obj-m += qot_am335x.o

KERNELDIR ?= ../../thirdparty/bb-kernel/KERNEL
CROSSCOMP ?= ../dl/gcc-linaro-4.9-2015.02-3-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-

all:
	make -C $(KERNELDIR) M=$(PWD) ARCH=arm CROSS_COMPILE="${CROSSCOMP}" modules

clean:
	make -C $(KERNELDIR) M=$(PWD) ARCH=arm CROSS_COMPILE="${CROSSCOMP}" clean

install:
	sudo cp -v *.ko $(KERNELDIR)/kernel/drivers
	sudo depmod
