/*
 * Reference 
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
#include <asm/io.h>
#include <linux/device.h>


#define DEVICE_NAME "leds" 
#define S3C64XX_GPKCON			(S3C64XX_GPK_BASE + 0x00)
#define S3C64XX_GPKDAT			(S3C64XX_GPK_BASE + 0x08)



static int major; //存放主设备号
static struct class *gpio_led_class;//定义一个类


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
		tmp = readl(S3C64XX_GPKDAT); //关灯
		tmp = (tmp & ~(0xf<<4));
		writel(tmp, S3C64XX_GPKDAT);
	}
	else	
	{
		tmp = readl(S3C64XX_GPKDAT);//开灯
		tmp = (tmp |(0xf<<4));
		writel(tmp, S3C64XX_GPKDAT);
	}
	
	printk("write success\n");
	return 0;
}

static const struct file_operations gpio_led_fops = {
	.owner = THIS_MODULE,
	.open = gpio_led_open,
	.write = gpio_led_write,
};

static int __init gpio_led_init(void) 
{	
	struct device *cd;
	
	/*注册字符设备驱动*/
	major = register_chrdev(0,DEVICE_NAME,&gpio_led_fops); 
	if(major<0 ){
		printk(DEVICE_NAME" cannot register device\n");
		return major;
	}
	
	/*创建一个类*/
	gpio_led_class = class_create(THIS_MODULE, DEVICE_NAME); 
	if (IS_ERR(gpio_led_class))
		return PTR_ERR(gpio_led_class);
	
	/*在类里面创建一个设备，使系统在/dev目录下生成一个设备节点 leds */
	cd = device_create(gpio_led_class, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);
	if (IS_ERR(cd))
		return PTR_ERR(cd);

	printk(DEVICE_NAME" init sucess\n");
	
	return 0;
}

static void __exit gpio_led_exit(void) 
{
	unregister_chrdev(major,DEVICE_NAME);
	device_destroy(gpio_led_class,MKDEV(major,0) );
	class_destroy(gpio_led_class);

	printk(DEVICE_NAME"exit sucess\n");
}


module_init(gpio_led_init);
module_exit(gpio_led_exit);

MODULE_AUTHOR("CXD");
MODULE_DESCRIPTION("GPIO LED driver");
MODULE_LICENSE("GPL");

