/*
* 参考友善之臂的内核 drivers/char/mini6410_leds.c
* 用混杂设备驱动模型实现LED驱动
*/


#include <linux/miscdevice.h>
#include <linux/delay.h>
#include <asm/irq.h>
#include <mach/hardware.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/string.h>
#include <linux/list.h>
#include <linux/pci.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <asm/unistd.h>

#include <mach/map.h>
#include <mach/regs-clock.h>
#include <mach/regs-gpio.h>

#include <plat/gpio-cfg.h>
#include <mach/gpio-bank-e.h>

#define DEVICE_NAME "leds"

#define S3C64XX_GPKCON			(S3C64XX_GPK_BASE + 0x00)
#define S3C64XX_GPKDAT			(S3C64XX_GPK_BASE + 0x08)


static int gpio_led_open(struct inode *inode, struct file *file)
{	
	unsigned int tmp;
	
	tmp = readl(S3C64XX_GPKCON);//设置GPK的4567引脚为输出状态
	tmp = (tmp & ~(0xffffU<<16))|(0x1111U<<16);
	writel(tmp, S3C64XX_GPKCON);
	
	printk("open success\n");
	return 0;
}

static ssize_t gpio_led_write(struct file *file, const char __user *buf,size_t count, loff_t *ppos)
{
	int val;
	unsigned int tmp;

	tmp = copy_from_user(&val,buf,count);//将应用程序传递的参数存到val变量中
	if(tmp)
		printk("copy_from_user failed!\n");
	
	if(val==1)
	{
		tmp = readl(S3C64XX_GPKDAT); 
		tmp = (tmp & ~(0xf<<4));
		writel(tmp, S3C64XX_GPKDAT);
	}
	else	
	{
		tmp = readl(S3C64XX_GPKDAT);
		tmp = (tmp |(0xf<<4));
		writel(tmp, S3C64XX_GPKDAT);
	}
	
	printk("write success\n");
	return 0;
}


static const struct file_operations gpio_led_fops = {
	.owner	= THIS_MODULE,
	.open	= gpio_led_open,
	.write	 = gpio_led_write,
	
};

static struct miscdevice gpio_led_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = DEVICE_NAME,
	.fops = &gpio_led_fops,
};

static int __init gpio_led_init(void)
{
	int ret;
	
	ret = misc_register(&gpio_led_misc);//注册混杂设备驱动
	
	if (ret) 
		printk(DEVICE_NAME "can't misc_register !\n");
	
	return ret;
}
static void __exit gpio_led_exit(void)
{
	misc_deregister(&gpio_led_misc);
}

module_init(gpio_led_init);
module_exit(gpio_led_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("CXD");


