obj-m += qot_timeline.o
obj-m += qot_core.o
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