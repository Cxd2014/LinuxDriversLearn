/*
*	中断实现按键驱动
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/poll.h>
#include <linux/irq.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <linux/interrupt.h>
#include <asm/uaccess.h>
#include <mach/hardware.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/miscdevice.h>

#include <mach/map.h>
#include <mach/regs-clock.h>
#include <mach/regs-gpio.h>

#include <plat/gpio-cfg.h>
#include <mach/gpio-bank-n.h>


#define DEVICE_NAME "keys"

static int major;//存放主设备号
static struct class *gpio_key_class;//定义一个类

/*生成一个等待队列头wait_queue_head_t,名字为gpio_key_waitq*/
static DECLARE_WAIT_QUEUE_HEAD(gpio_key_waitq);

/* 中断事件标志, 中断服务程序将它置1，key_read将它清0 */
static volatile int ev_press = 0;

//记录按键值
static int key_val;

/* 定义一个结构体 */
struct gpio_key_desc
{
	unsigned int irq;
	unsigned int number;
	char *name;
};

/* 定义一个结构体数组 */
static struct gpio_key_desc keys_irq[] = 
{
	{IRQ_EINT(0), 0, "KEY0"},
    {IRQ_EINT(1), 1, "KEY1"},
    {IRQ_EINT(2), 2, "KEY2"},
    {IRQ_EINT(3), 3, "KEY3"},
};


/* 中断处理函数 */
static irqreturn_t gpio_key_irq(int irq, void *dev_id)
{
	

	struct gpio_key_desc *key_irq = (struct gpio_key_desc *)dev_id;
	
	printk("irq=%d\n" ,irq);

	key_val = key_irq->number;//读取按键值

	ev_press = 1;
    wake_up_interruptible(&gpio_key_waitq);//唤醒休眠进程

	return IRQ_RETVAL(IRQ_HANDLED);
}

static int gpio_key_open(struct inode *inode, struct file *file)
{
	int i;
	int err = 0;
	
	for(i=0;i<4;i++)//申请中断
	{
				/* 申请中断  中断号       中断处理函数  中断类型-上升沿             */
		err = request_irq(keys_irq[i].irq,gpio_key_irq,IRQF_TRIGGER_FALLING,keys_irq[i].name,&keys_irq[i]);
		if(err)
		{
			printk("request_irq failed! irq = %d",i);
			break;
		}
	}
	return 0;
}


ssize_t gpio_key_read(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
	int tmp;
	
	/*当中断发生时，先执行中断程序，然后进入 wait_event_interruptible 函数
	 *判断 ev_press = 1 时不休眠，执行下面的程序，当 ev_press = 0 时休眠
	 */
	wait_event_interruptible(gpio_key_waitq, ev_press);//进入休眠状态

	tmp = copy_to_user(buf,&key_val,1);//将按键值传递给应用程序
	if(tmp)
		printk("copy_from_user failed!\n");

	ev_press = 0;
	return 0;
}

static int gpio_key_close(struct inode *inode, struct file *file)
{
	int i;
	for(i=0;i<4;i++)/* 释放中断 */
	{
		free_irq(keys_irq[i].irq,&keys_irq[i]);
	}
	return 0;
}

static const struct file_operations gpio_key_fops = {
	.owner = THIS_MODULE,
	.open = gpio_key_open,
	.read = gpio_key_read,
	.release = gpio_key_close,
};


static int __init gpio_key_init(void) 
{
	struct device *cd;

	/*注册字符设备驱动*/
	major = register_chrdev(0,DEVICE_NAME,&gpio_key_fops); 
	if(major<0 ){
		printk(DEVICE_NAME" cannot register device\n");
		return major;
	}

	/*创建一个类*/
	gpio_key_class = class_create(THIS_MODULE, DEVICE_NAME); 
	if (IS_ERR(gpio_key_class))
		return PTR_ERR(gpio_key_class);
	
	/*在类里面创建一个设备，使系统在/dev目录下生成一个设备节点 keys */
	cd = device_create(gpio_key_class, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);
	if (IS_ERR(cd))
		return PTR_ERR(cd);

	printk(DEVICE_NAME" init sucess\n");
	
	return 0;
}

static void __exit gpio_key_exit(void) 
{
	unregister_chrdev(major,DEVICE_NAME);
	device_destroy(gpio_key_class,MKDEV(major,0) );
	class_destroy(gpio_key_class);

	printk(DEVICE_NAME"exit sucess\n");
}


module_init(gpio_key_init);
module_exit(gpio_key_exit);

MODULE_AUTHOR("CXD");
MODULE_DESCRIPTION("GPIO KEY driver");
MODULE_LICENSE("GPL");
