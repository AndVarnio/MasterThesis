PWD := $(shell pwd)
obj-m += dma_proxy.o

#INC=-I /home/andreas/MasterThesis/larsCubeDMAcode/linux-4.9/tools/testing/radix-tree

#INC=-I /home/andreas/gcc-linaro-7.3.1-2018.05-i686_arm-linux-gnueabihf/arm-linux-gnueabihf/libc/usr/include

all:
	make ARCH=arm CROSS_COMPILE=$(CROSS) -C $(KERNEL) SUBDIRS=$(PWD) modules
clean:
	make -C $(KERNEL) SUBDIRS=$(PWD) clean
