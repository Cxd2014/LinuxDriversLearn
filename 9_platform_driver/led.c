/*
* 分配、设置、注册一个 platfrom_device 结构体
*/

#include <linux/module.h>
#include <linux/version.h>
#include <linux/init.h>

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/serial_core.h>
#include <linux/platform_device.h>

static struct resource led_resource[] = {
	[0] = {
		.start = 0x7F008800,         //寄存器的起始地址
		.end   = 0x7F008800 + 8 - 1, //寄存器的终止地址
		.flags = IORESOURCE_MEM,     
	},
	[1] = {
		.start = 6,//只需要改变这里的值就能控制哪个引脚的LED，而不需要修改另一个文件，这就是分层的好处
		.end   = 6,//这是中断号
		.flags = IORESOURCE_IRQ,//申请中断资源，注意这里的中断资源在led_drv.c中没有用到，只是
								//用到中断号来表示那个GPIO引脚
	},
};


static void cxd_led_release (struct device *dev)
{
	//这个函数不能省略
}


static struct platform_device cxd_led = {
	.name			= "myled",
	.id             = -1, 
	.num_resources  = ARRAY_SIZE(led_resource),
	.resource       = led_resource,
	.dev            = {
						.release = cxd_led_release,
					  },
};


static int cxd_led_init(void)
{
	int err;
	err = platform_device_register(&cxd_led);
	if (err){
		printk("platform_device_register failed!\n");
		return err;
	}

	return 0;
}

static void cxd_led_exit(void)
{
	platform_device_unregister(&cxd_led);
}

module_init(cxd_led_init);
module_exit(cxd_led_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("CXD");
MODULE_VERSION("1.0");
MODULE_DESCRIPTION("S3C6410 LED Driver");