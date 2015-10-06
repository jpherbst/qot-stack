obj-m += qot.o
qot-objs := qot_chardev.o
all:
	make -C ../../thirdparty/Kronux M=$(PWD) ARCH=arm modules

clean:
	make -C ../../thirdparty/Kronux M=$(PWD) ARCH=arm clean
