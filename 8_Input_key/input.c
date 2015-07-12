/*
* 输入子系统
* 参考 drivers/input/keyboard/gpio_keys.c
* 测试的时候执行命令 cat /dev/tty1 在按下按键就会显示字符
*/

#include <linux/module.h>

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

#define DEVICE_NAME "keys"

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

/*定义一个 input_dev 结构体 */
static struct input_dev *key_input;

/* 定义一个定时器防按键抖动 */
static struct timer_list key_timer;

//记录按键值
static int key_val;

static int keyboard[] = 
{
	KEY_A,KEY_B,KEY_C,KEY_ENTER,
};


/* 定时器处理函数 */
static void key_timer_function(unsigned long tdata)
{
	//按键按下
	input_event(key_input, EV_KEY,keyboard[key_val], 1); //上报事件
	input_sync(key_input);
	//printk("key_timer_function %d\n",keyboard[key_val]);

	//按键松开
	input_event(key_input, EV_KEY,keyboard[key_val], 0); //上报事件
	input_sync(key_input);

}


/* 中断处理函数 */
static irqreturn_t gpio_key_irq(int irq, void *dev_id)
{
	struct gpio_key_desc *key_irq = (struct gpio_key_desc *)dev_id;
	
	/* 100ms后启动定时器 */	
	mod_timer(&key_timer, jiffies+HZ/10);

	key_val = key_irq->number;//读取按键值

	//printk("gpio_key_irq\n");
	return IRQ_RETVAL(IRQ_HANDLED);
}



static int __init gpio_key_init(void) 
{
	int i,error;
	/* 分配一个input_dev结构体 */
	key_input = input_allocate_device();
	if (!key_input) {
		printk("failed to allocate input_dev\n");
	}

	key_input->name = DEVICE_NAME;
	/* 设置产生哪一类事件 */
	set_bit(EV_KEY,key_input->evbit); //按键类事件 include/linux/input.h 的 input_dev 结构体中定义的事件

	/* 产生这一类事件里的哪一些事件 */
	set_bit(KEY_A,key_input->keybit);            //按下按键产生字母 A
	set_bit(KEY_B,key_input->keybit);            //按下按键产生字母 B
	set_bit(KEY_C,key_input->keybit);        //按下按键产生 ENTER
	set_bit(KEY_ENTER,key_input->keybit);
	
	/* 注册 */
	error = input_register_device(key_input);
	if (error) {
		printk("Unable to register input device!\n");
	}

	for(i=0;i<4;i++)//申请中断
	{
				/* 申请中断  中断号       中断处理函数  中断类型-上升沿             */
		error = request_irq(keys_irq[i].irq,gpio_key_irq,IRQF_TRIGGER_FALLING,keys_irq[i].name,&keys_irq[i]);
		if(error)
		{
			printk("request_irq failed! irq = %d",i);
			break;
		}
	}

	//初始化定时器
	init_timer(&key_timer);
	key_timer.function = key_timer_function;
	add_timer(&key_timer); 

	printk(DEVICE_NAME" init sucess\n");

	return 0;
}



static void __exit gpio_key_exit(void) 
{
	int i;
	for(i=0;i<4;i++)/* 释放中断 */
	{
		free_irq(keys_irq[i].irq,&keys_irq[i]);
	}

	del_timer(&key_timer); //注销定时器

	input_unregister_device(key_input);
	input_free_device(key_input);

	printk(DEVICE_NAME" exit sucess\n");
}


module_init(gpio_key_init);
module_exit(gpio_key_exit);

MODULE_AUTHOR("CXD");
MODULE_DESCRIPTION("GPIO KEY driver");
MODULE_LICENSE("GPL");
