obj-m	+= lcd.o
KDIR = /root/desktop/linux-2.6.38
all :
	make -C $(KDIR) M=$(PWD) modules CROSS_COMPILE=arm-linux- ARCH=arm
clean :
	rm -f  *.o *.mod.o *.mod.c *.symvers *.bak *.order

