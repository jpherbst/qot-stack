SHELL := /bin/bash

obj-m += qot_core.o
obj-m += qot_am335x.o

KERNELDIR ?= /export/bb-kernel/KERNEL
#KERNELVER ?= 4.1.12-bone-rt-r16
KERNELVER ?= 4.6.3-bone-rt-r3

IPADDR ?= 192.168.1.110

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

install_sd:
	# modules and stuff
	scp src/modules/qot/*.ko root@$(IPADDR):/lib/modules/$(KERNELVER)/kernel/drivers/misc
	scp src/modules/qot_am335x/*.ko root@$(IPADDR):/lib/modules/$(KERNELVER)/kernel/drivers/misc
	scp *.dtbo root@$(IPADDR):/lib/firmware
	#sudo scp src/qot-daemon/test.sh root@$(IPADDR):/home
	scp targets/common/80-qot.rules root@$(IPADDR):/etc/udev/rules.d/
	scp targets/am335x/capes root@$(IPADDR):/usr/bin/
	# dynamic libraries and headers
	scp /export/rootfs/usr/local/lib/*.so root@$(IPADDR):/usr/local/lib/
	scp src/api/c/qot.h root@$(IPADDR):/usr/local/include/ 
	scp src/api/cpp/qot.hpp root@$(IPADDR):/usr/local/include/
	# executables
	scp build/src/qot-daemon/qotd root@$(IPADDR):/usr/local/bin/
	ssh root@$(IPADDR) chmod 755 /usr/local/bin/qotd
	scp build/src/service/qotdaemon root@$(IPADDR):/usr/local/bin/
	ssh root@$(IPADDR) chmod 755 /usr/local/bin/qotdaemon
	scp build/src/service/testptp root@$(IPADDR):/usr/local/bin/
	ssh root@$(IPADDR) chmod 755 /usr/local/bin/testptp
	scp build/src/service/phc2phc root@$(IPADDR):/usr/local/bin/
	ssh root@$(IPADDR) chmod 755 /usr/local/bin/phc2phc
	# Install Apps using this template
	scp build/src/examples/c/helloworld root@$(IPADDR):/usr/local/bin/
	ssh root@$(IPADDR) chmod 755 /usr/local/bin/helloworld
	scp build/src/examples/c/sean/ledapp root@$(IPADDR):/usr/local/bin/
	ssh root@$(IPADDR) chmod 755 /usr/local/bin/ledapp
	# Install Apps Ends
	ssh root@$(IPADDR) chmod 755 /usr/bin/capes
	ssh root@$(IPADDR) depmod
	ssh root@$(IPADDR) ldconfig

reload:
	ssh root@$(IPADDR) depmod
	ssh root@$(IPADDR) ldconfig
