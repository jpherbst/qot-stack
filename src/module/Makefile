obj-m += qot.o
qot-objs := qot_module.o
qot-objs := qot_clock.o
all:
	make -C ../../thirdparty/Kronux M=$(PWD) ARCH=arm modules

clean:
	make -C ../../thirdparty/Kronux M=$(PWD) ARCH=arm clean
