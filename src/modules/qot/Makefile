obj-m += qot.o
qot-objs := qot_core.o qot_clock.o qot_clock_sysfs.o       \
            qot_timeline.o          \
            qot_timeline_chdev.o    \
            qot_timeline_sysfs.o    \
            qot_scheduler.o         \
            qot_admin.o         	\
            qot_admin_chdev.o       \
            qot_admin_sysfs.o 		\
            qot_user.o    	 	    \
            qot_user_chdev.o        \
	    qot_clock_gl.o \

SRC := $(shell pwd)

all:
        $(MAKE) -C $(KERNEL_SRC) M=$(SRC) V=1 modules

modules_install:
        $(MAKE) -C $(KERNEL_SRC) M=$(SRC) modules_install
