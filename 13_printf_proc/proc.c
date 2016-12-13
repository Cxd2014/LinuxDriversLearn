#include <linux/module.h>  
#include <linux/kernel.h>  
#include <linux/proc_fs.h>  
#include <linux/sched.h>  
#include <asm/uaccess.h>  
#include <linux/slab.h>

/* /proc/uart 文件的最大容量 4KB */
#define STRINGLEN 4096

static int srt_len = 0;
static char *global_buffer = NULL;  
struct proc_dir_entry *uart_file;  

extern int (*telnet_uart_match)(const unsigned char *s, unsigned int count);
static int uart_ct_match(const unsigned char *s, unsigned int count);

static int uart_ct_match(const unsigned char *s, unsigned int count)
{
    if(srt_len+count >= STRINGLEN){
        srt_len = 0;
        memset(global_buffer,0,STRINGLEN);
    }
    memcpy(global_buffer+srt_len,s,count);
    srt_len = srt_len + count;
    return 0;
}

static int proc_read_uart(char *page, char **start, off_t off, int count, int *eof, void *data) 
{    
    int len = sprintf(page, global_buffer);  
    return len;  
}  

static int __init proc_uart_init(void) 
{  
    uart_file = create_proc_entry("uart", S_IRUGO, NULL);  

    uart_file->read_proc = proc_read_uart;  
    uart_file->write_proc = NULL;  


    global_buffer = kmalloc(STRINGLEN,GFP_KERNEL);
    if(global_buffer == NULL){
        printk("kmalloc error");
        return 0;
    }
        
    memset(global_buffer,0,STRINGLEN);
    telnet_uart_match = uart_ct_match;
    return 0;  
}  
   
static void __exit proc_uart_exit(void) 
{  
    telnet_uart_match = NULL;
    kfree(global_buffer);
    remove_proc_entry("uart",NULL); 
}  
   
module_init(proc_uart_init);  
module_exit(proc_uart_exit);  