APP1  = helloworld
APP2 = qot
# APP2  = test-multitask
# APP3  = test-clone
# APP4  = test-clone-manual

KERNEL_DIR = ../../../

CROSS_COMPILE= /home/sandeepdsouza/Beagle_Workspace/bb-kernel/dl/gcc-linaro-4.9-2015.05-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-

CC = $(CROSS_COMPILE)g++

ARCH = arm 

CCFLAGS := -I$(KERNEL_DIR) -I$(KERNEL_DIR)qot-stack/module -lrt -DARCH_$(ARCH) -std=c++11 -pthread
CCFLAGS += -O3

all: 
	# $(CC) -o $(APP1)  $(APP1).c $(CCFLAGS) 
	$(CC) -o $(APP1)  $(APP1).cpp $(APP2).cpp  $(CCFLAGS) 
	# $(CC) -o $(APP3)  $(APP3).c $(CCFLAGS) 
	# $(CC) -o $(APP4)  $(APP4).c $(CCFLAGS) 

clean:
	rm -f $(APP1)
	rm -f $(APP2)
	rm -f $(APP3)
	rm -f $(APP4)

