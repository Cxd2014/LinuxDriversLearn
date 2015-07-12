/*
* 分配、设置、注册一个 platfrom_driver 结构体
* 这边的程序不用改变
*/

#include <linux/module.h>
#include <linux/version.h>

#include <linux/init.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/pm.h>
#include <linux/slab.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/gpio_keys.h>
#include <linux/workqueue.h>
#include <linux/gpio.h>

#include <asm/io.h>
#include <asm/uaccess.h>

static int major;
static struct class *led_class;

volatile unsigned long *gpkcon0 = NULL;
volatile unsigned long *gpkdat = NULL;

static int pin;

static int led_open(struct inode *inode ,struct file *file)
{
	//配置gpkcon0为输出状态
	*gpkcon0 &= ~(0xf<<(pin*4));//先清零
	*gpkcon0 |= (0x1<<(pin*4));//在配置为输出状态
	return 0;
}

static ssize_t led_write(struct file *filp,const char __user *buf,size_t count,loff_t *f_pos)
{
	int val = 0;
	int tmp;
	tmp = copy_from_user(&val,buf,count);
	if(tmp)
		printk("copy_from_user failed!\n");

	printk("val = %d\n",val);
	
	if (val == 1)
	{
		*gpkdat &= ~(1<<pin);//点灯	
	}
	else
	{
		*gpkdat |= (1<<pin);//灭灯	
	}
	return 0;
}

static struct file_operations led_fops=
{
	.owner = THIS_MODULE,
	.open = led_open,
	.write = led_write,
};


static int led_probe(struct platform_device *pdev)
{
	//根据platform_device的资源来获取资源
	struct resource *res;
	struct device *cd;

	res = platform_get_resource(pdev,IORESOURCE_MEM,0); //获取内存资源	
	gpkcon0 = ioremap(res->start,res->end - res->start + 1);
	gpkdat  = gpkcon0+2;

	res = platform_get_resource(pdev,IORESOURCE_IRQ,0); //获取中断资源
	pin = res->start;
	
	printk("pin = %d\n",pin);
	
	printk("led_probe, found led\n");
	
	//注册字符设备驱动
	major = register_chrdev(0, "led", &led_fops);
	
	led_class =class_create(THIS_MODULE, "led");
	if (IS_ERR(led_class))
		return PTR_ERR(led_class);

	cd = device_create(led_class, NULL, MKDEV(major, 0), NULL, "led");
	if (IS_ERR(cd))
		return PTR_ERR(cd);
	
	return 0;
}

static int led_remove(struct platform_device *pdev)
{
	device_destroy(led_class,MKDEV(major,0));
	class_destroy(led_class);
	unregister_chrdev(major,"led");
	iounmap(gpkcon0);
	printk("led_remove！\n");
	return 0;
}

static struct platform_driver led_drv = {	
	.probe   = led_probe,
	.remove  = led_remove,
	.driver  = {
		.name	 = "myled",
	},
};


static int led_drv_init(void)
{
	int ret;

	ret = platform_driver_register(&led_drv);
	if (ret != 0)
		pr_err("Failed to register ab8500 regulator: %d\n", ret);

	printk("led init sucess\n");

	return 0;
}

static void led_drv_exit(void)
{
	platform_driver_unregister(&led_drv);
}



module_init(led_drv_init);
module_exit(led_drv_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("CXD");
MODULE_VERSION("1.0");
MODULE_DESCRIPTION("S3C6410 LED Driver");
