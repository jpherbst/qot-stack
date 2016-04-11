APP1  = qotd
DEP = /home/roseline/qot-stack/src/api/c/qot

KERNELDIR ?= /export/bb-kernel/KERNEL
KERNELVER ?= 4.1.6-bone-rt-r16

CROSS_COMPILE=  /export/bb-kernel/dl/gcc-linaro-4.9-2015.02-3-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-

CC = $(CROSS_COMPILE)gcc

ARCH = arm 

CCFLAGS := -I$(KERNEL_DIR) -I/qot-stack/src/modules/qot -lrt -lm -DARCH_$(ARCH) -pthread 
CCFLAGS += -O3

all: 
	$(CC) -o $(APP1)  $(APP1).c $(DEP).c $(CCFLAGS) 

install:
	sudo scp -v $(APP1) root@192.168.7.2:/home/apps 
	#sudo scp -v $(APP1) root@192.168.127.204:/home/apps 
	#sudo scp -v $(APP1) root@192.168.127.202:/home/apps 


clean:
	rm -f $(APP1)
	

