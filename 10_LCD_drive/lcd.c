/* head file copy from s3c2410fb.c */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/cpufreq.h>

#include <asm/io.h>
#include <asm/div64.h>

#include <asm/mach/map.h>
#include <mach/regs-gpio.h>

static volatile unsigned long *MIFPCON;
static volatile unsigned long *SPCON;
static volatile unsigned long *VIDCON0;
static volatile unsigned long *VIDCON1;
static volatile unsigned long *VIDTCON0;
static volatile unsigned long *VIDTCON1;
static volatile unsigned long *VIDTCON2;
static volatile unsigned long *WINCON0;
static volatile unsigned long *VIDOSD0A;
static volatile unsigned long *VIDOSD0B;
static volatile unsigned long *VIDOSD0C;
static volatile unsigned long *VIDW00ADD0B0;
static volatile unsigned long *VIDW00ADD1B0;
static volatile unsigned long *VIDW00ADD2;
static volatile unsigned long *DITHMODE;


static volatile unsigned long *GPECON;
static volatile unsigned long *GPFCON;
static volatile unsigned long *GPICON;
static volatile unsigned long *GPJCON;


/*S70 Data Manual's 14 page
  *HSPW == HS pulse width                            1 ~ 40         ==hsync_len	= 10
  *HBPD == HS Blanking                                 46           ==left_margin	= 36
  *HFPD == HS Front Porch                             16~210~354    ==right_margin	= 80
  *LINEVAL == Horizontal Display Area       800
  */
  
#define HSPW 			(10)
#define HBPD			 	(36)
#define HFPD 				(80)
#define HOZVAL			(799)

/*S70 Data Manual's 14 page
  *VSPW == VS pulse width                       1 ~ 20         ==vsync_len	= 8
  *VBPD == VS Blanking                            23           ==upper_margin	= 15
  *VFPD == VS Front Porch                       7~22~147       ==lower_margin	= 22
  *LINEVAL == Vertical Display Area     480
  */
#define VSPW				(8)        
#define VBPD 			(15)
#define VFPD 				(22)
#define LINEVAL 			(479)


static struct fb_info *s3c_lcd;

static u32 pseudo_pal[16];

static inline unsigned int chan_to_field(unsigned int chan, struct fb_bitfield *bf)
{
	chan &= 0xffff;
	chan >>= 16 - bf->length;
	return chan << bf->offset;
}

static int s3c_lcdfb_setcolreg(unsigned int regno, unsigned int red,
			     unsigned int green, unsigned int blue,
			     unsigned int transp, struct fb_info *info)
{
	unsigned int val;
	
	if (regno > 16)
		return 1;

	/* use RGB the three color create the val*/
	val  = chan_to_field(red,	&info->var.red);
	val |= chan_to_field(green, &info->var.green);
	val |= chan_to_field(blue,	&info->var.blue);
	
	pseudo_pal[regno] = val;
	return 0;
}

static struct fb_ops s3c_lcdfb_ops = {
	.owner		= THIS_MODULE,
	.fb_setcolreg	= s3c_lcdfb_setcolreg,
	.fb_fillrect	= cfb_fillrect,
	.fb_copyarea	= cfb_copyarea,
	.fb_imageblit	= cfb_imageblit,
};

static int lcd_init(void)/*The entry of lcd driver*/
{
	int ret;
	dma_addr_t map_dma;
	/*1. allocation a fb_info structure */
	s3c_lcd = framebuffer_alloc(0,NULL);
	if (!s3c_lcd)
		return -ENOMEM;

	/*2.1 setting the fix Parameter */
	strcpy(s3c_lcd->fix.id,"CXD_lcd");
	s3c_lcd->fix.smem_len = 800*480*4;
	s3c_lcd->fix.type = FB_TYPE_PACKED_PIXELS;
	s3c_lcd->fix.visual = FB_VISUAL_TRUECOLOR;   /* True color */
	s3c_lcd->fix.line_length = 800*4;
	
	/*2.2 setting the variable Parameter */
	s3c_lcd->var.xres                       = 800;
	s3c_lcd->var.yres                       = 480;
	s3c_lcd->var.xres_virtual        = 800;
	s3c_lcd->var.yres_virtual         = 480;
	s3c_lcd->var.bits_per_pixel     = 32;
	/* RGB = 8:8:8 */
	s3c_lcd->var.red.offset          = 16;
	s3c_lcd->var.red.length         = 8;
	s3c_lcd->var.green.offset       = 8;
	s3c_lcd->var.green.length      = 8;
	s3c_lcd->var.blue.offset         = 0;
	s3c_lcd->var.blue.length        = 8;
	s3c_lcd->var.activate             = FB_ACTIVATE_NOW;
	
	/*2.3 setting the Operation function */
	s3c_lcd->fbops = &s3c_lcdfb_ops;

	/*2.4 setting the Other  Parameter*/
	s3c_lcd->screen_size = 800*480*4; 
	s3c_lcd->pseudo_palette = pseudo_pal;


	/* 3.1 setting GPIO for LCD */
	GPECON = ioremap(0x7F008080,4);
	GPFCON = ioremap(0x7F0080A0,4);
	GPICON = ioremap(0x7F008100,4);
	GPJCON = ioremap(0x7F008120,4);

	*GPICON = 0xAAAAAAAA;                         
	*GPJCON = 0x00AAAAAA;   
	
          *GPFCON &= ~(0x3<<30);  //clear 0
	*GPFCON  |=(1<<30);        // set value	    
	
	*GPECON &= ~(0xf);//clear 0
	*GPECON |= 0x1; // set value

	MIFPCON                 = ioremap(0x7410800C,4);
	SPCON                      = ioremap(0x7F0081A0,4);
	VIDCON0                 = ioremap(0x77100000,4);
	VIDCON1                 = ioremap(0x77100004,4);
	VIDTCON0              = ioremap(0x77100010,4);
	VIDTCON1              = ioremap(0x77100014,4);
	VIDTCON2             = ioremap(0x77100018,4);
	WINCON0              = ioremap(0x77100020,4);
	VIDOSD0A             = ioremap(0x77100040,4);
	VIDOSD0B             = ioremap(0x77100044,4);
	VIDOSD0C             = ioremap(0x77100048,4);
	VIDW00ADD0B0 = ioremap(0x771000A0,4);
	VIDW00ADD1B0 = ioremap(0x771000D0,4);
	VIDW00ADD2      = ioremap(0x77100100,4);
	DITHMODE           = ioremap(0x77100170,4);
	
	// normal mode
	*MIFPCON &= ~(1<<3);

	// RGB I/F Style
	*SPCON =  (*SPCON & ~(0x3)) | 1;

	/*  VIDCON0 set :
	  *   (0<<26) == RGB I/F
	  *   (0<<17) == RGB Parallel format (RGB)
	  *   (0<<16) == Select CLKVAL_F update timing control = always
	  *   (3<<6) == VCLK Video Clock Source / (CLKVAL+1)         133Mh/(3+1) = 33.25Mh
	  *   (0<<5) == VCLK Free run control ( Only valid at the RGB IF mode) = Normal mode
	  *   (1<<4) == Select the clock source as direct or divide using CLKVAL_F register = Divided by CLKVAL_F
	  *   (0<<2) == Select the Video Clock source = HCLK
	  *   (3<<0) == Enable the video output and the Display control signal
	  */
	*VIDCON0 = (0<<26)|(0<<17)|(0<<16)|(3<<6)|(0<<5)|(1<<4)|(0<<2)|(3<<0);


	/*VIDCON1 set :
	  * 1<<5 == This bit indicates the HSYNC pulse polarity = inverted
	  * 1<<6 == This bit indicates the VSYNC pulse polarity = inverted
	  */
	*VIDCON1 |= 1<<5 | 1<<6;


        /*VIDTCON0 set :
	 *See the top define 
	 */
	*VIDTCON0 = VBPD<<16 | VFPD<<8 | VSPW<<0;
	*VIDTCON1 = HBPD<<16 | HFPD<<8 | HSPW<<0;
	*VIDTCON2 = (LINEVAL << 11) | (HOZVAL << 0);


        /*WINCON0 set :
	 *1<<0 == Enable the video output and the VIDEO control signal
	 *0xB<<2 == Select the BPP (Bits Per Pixel) mode Window image = unpacked 24 BPP (non-palletized R:8-G:8-B:8 )
	 */
	*WINCON0 |= 1<<0;
	*WINCON0 &= ~(0xf << 2);//clear 0
	*WINCON0 |= 0xB<<2;

	//Set the window size and Coordinate system
#define LeftTopX     0
#define LeftTopY     0
#define RightBotX   799
#define RightBotY   479
	*VIDOSD0A = (LeftTopX<<11) | (LeftTopY << 0);
	*VIDOSD0B = (RightBotX<<11) | (RightBotY << 0);
	*VIDOSD0C = (LINEVAL + 1) * (HOZVAL + 1);

	/* allocation Frame buffer */
	//s3c_lcd->screen_base = dma_alloc_writecombine(NULL, s3c_lcd->fix.smem_len, &s3c_lcd->fix.smem_start, GFP_KERNEL);
	s3c_lcd->screen_base = dma_alloc_writecombine(NULL, s3c_lcd->fix.smem_len, &map_dma, GFP_KERNEL);
	if (!s3c_lcd->screen_base)
		return -ENOMEM;
	
	//tell the lcd controller the Frame buffer start adress and end adress
	*VIDW00ADD0B0 = s3c_lcd->fix.smem_start;
	*VIDW00ADD1B0 = (s3c_lcd->fix.smem_start + s3c_lcd->fix.smem_len)&(0xffffff);

	printk("s3c_lcd->fix.smem_start = %lu\n",s3c_lcd->fix.smem_start);
		
	/* register the structure*/
	ret = register_framebuffer(s3c_lcd);
	if (ret < 0) 
		printk(KERN_ERR "Failed to register framebuffer device: %d\n",ret);

	printk("LCD init succeed!\n");
	return 0;
}


static void lcd_exit(void)
{
	unregister_framebuffer(s3c_lcd);
	dma_free_writecombine(NULL, s3c_lcd->fix.smem_len, s3c_lcd->screen_base ,s3c_lcd->fix.smem_start);
	iounmap(GPECON);
	iounmap(GPFCON);
	iounmap(GPICON);
	iounmap(GPJCON);
	iounmap(MIFPCON);
	iounmap(SPCON);
	iounmap(VIDCON0);
	iounmap(VIDCON1);
	iounmap(VIDTCON1);
	iounmap(VIDTCON2);
	iounmap(WINCON0);
	iounmap(VIDOSD0A);
	iounmap(VIDOSD0B);
	iounmap(VIDOSD0C);
	iounmap(VIDW00ADD0B0);
	iounmap(VIDW00ADD0B0);
	iounmap(VIDW00ADD2);
	iounmap(DITHMODE);
	framebuffer_release(s3c_lcd);
	printk("LCD exit succeed!");
}



module_init(lcd_init);
module_exit(lcd_exit);
MODULE_AUTHOR("CXD");
MODULE_DESCRIPTION("Framebuffer driver for the s3c6410");
MODULE_LICENSE("GPL");

