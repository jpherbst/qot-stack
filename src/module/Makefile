obj-m += qot.o
qot-objs += qot_core.o qot_timeline.o qot_clock.o
obj-m += qot_am335x.o
obj-m += polltest.o

KERNELDIR ?= /lib/modules/$(shell uname -r)

all:
	make -C $(KERNELDIR)/build M=$(PWD) modules
	gcc -std=c99 -o pollclient pollclient.c

clean:
	make -C $(KERNELDIR)/build M=$(PWD) clean

install:
	sudo cp -v *.ko $(KERNELDIR)/kernel/drivers
	sudo depmod