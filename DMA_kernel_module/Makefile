PWD := $(shell pwd)
obj-m += hello_world_kernel_module.o
#make KERNEL=/home/andreas/linuxKernel-4.9-Arm CROSS=/home/andreas/gcc-linaro-7.3.1-2018.05-i686_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-

all:
	make ARCH=arm CROSS_COMPILE=$(CROSS) -C $(KERNEL) SUBDIRS=$(PWD) modules
clean:
	make -C $(KERNEL) SUBDIRS=$(PWD) clean
