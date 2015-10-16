obj-m += qot.o
qot-objs += qot_core.o qot_timeline.o qot_clock.o
#obj-m += qot_am335x.o
#obj-m += qot_am335x_chronos.o
#obj-m += qot_am335x_lowpower.o
#obj-m += qot_am335x_gpsdo.o

KERNELDIR ?= /lib/modules/$(shell uname -r)

all:
	make -C $(KERNELDIR)/build M=$(PWD) modules

clean:
	make -C $(KERNELDIR)/build M=$(PWD) clean

install:
	sudo cp -v *.ko $(KERNELDIR)/kernel/drivers
	sudo depmod