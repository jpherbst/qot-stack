obj-m += qot_am335x.o
SRC := $(shell pwd)

all:
        $(MAKE) -C $(KERNEL_SRC) M=$(SRC) V=1 EXTRA_CFLAGS="$(KERNEL_SRC)" modules

modules_install:
        $(MAKE) -C $(KERNEL_SRC) M=$(SRC) modules_install
