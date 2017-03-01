obj-m	+= cxdfs.o
KDIR = /root/work/ap_i22/develop_svn1785/bsp/kernel/bcm47189_wl_9.10.178.7_2.6.36
all :
#   make -C $(KDIR) M=$(PWD) modules CROSS_COMPILE=/projects/hnd/tools/rtl/rsdk-1.5.5-4181-EB-2.6.30-0.9.30.3-110225/bin/rsdk-linux- ARCH=arm
	make -C $(KDIR) M=$(PWD) modules CROSS_COMPILE=/projects/hnd/tools/linux/hndtools-arm-linux-2.6.36-uclibc-4.5.3/bin/arm-brcm-linux-uclibcgnueabi- ARCH=arm

clean :
	rm -f  *.o *.mod.o *.mod.c *.symvers *.bak *.order
