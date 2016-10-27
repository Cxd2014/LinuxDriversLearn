/*
* netfilter加令牌桶算法，实现UDP数据包的限流功能
* 基于数据包个数的限流
* 每4秒只能发送一个UDP数据包，多余的数据包将缓存起来，
* 最多缓存10个数据包，多余的扔掉
*/


/*
NF_ACCEPT：继续正常的报文处理；
NF_DROP：将报文丢弃；
NF_STOLEN：由钩子函数处理了该报文，不要再继续传送；
NF_QUEUE：将报文入队，通常交由用户程序处理；
NF_REPEAT：再次调用该钩子函数。
*/

#include <linux/module.h>  
#include <linux/kernel.h>  
#include <linux/init.h>    

#include <linux/timer.h>

#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>

#include <linux/ip.h>
#include <linux/skbuff.h>
#include <linux/icmp.h>
#include <linux/udp.h>

#define UDP_PROTOCOL 17
#define MAX_NUM_PACKAGE 10
#define TIME_SECOND 4

static struct timer_list icmp_filter_timer;
static int index = 0;

static struct sk_buff *cache_skb[MAX_NUM_PACKAGE];
static int count = 0;

int (*cache_skb_send_fn)(struct sk_buff *);

unsigned int icmp_filter_hook(unsigned int hooknum,
    struct sk_buff *pskb,
    const struct net_device *in,
    const struct net_device *out,
    int (*okfn)(struct sk_buff *))  
{
    struct iphdr *iph;
    int udp_ret = NF_ACCEPT;
    
    iph = ip_hdr(pskb);

    if(iph->protocol == UDP_PROTOCOL){
        if(iph->daddr != 16777343){ /* 过滤 127.0.0.1 地址 */
            cache_skb_send_fn = okfn;  
            if(index > 0){
                index--;      
            }else{
                if(count < MAX_NUM_PACKAGE){
                    cache_skb[count] = pskb;
                    count++;
                    udp_ret = NF_STOLEN; 
                }else{
                    printk("dropped the data package!\n");
                    udp_ret = NF_DROP; 
                }          
            }
        }
    }
    return udp_ret;
}

static void icmp_filter_timer_callback(unsigned long data)
{
    if(count > 0){ /* 发送缓存的数据包 */
        count--;
        if(cache_skb_send_fn(cache_skb[count]) != 0)
            printk("cache data send filed!\n");
    }

    if(index < MAX_NUM_PACKAGE) /* 增加令牌 */
        index++;

    mod_timer(&icmp_filter_timer, jiffies + TIME_SECOND*HZ);
}

static struct nf_hook_ops icmp_filter_ops = {
    .hook		= icmp_filter_hook,
    .owner		= THIS_MODULE,
    .pf			= PF_INET,
    .hooknum	= NF_INET_POST_ROUTING,
    .priority	= NF_IP_PRI_MANGLE,
};

static int __init icmp_filter_init(void)
{
    nf_register_hook(&icmp_filter_ops);
    
    setup_timer(&icmp_filter_timer, icmp_filter_timer_callback, 0);
    mod_timer(&icmp_filter_timer, jiffies + TIME_SECOND*HZ);

    printk("icmp_filter_init\n");
    return 0;
}

static void __exit icmp_filter_exit(void)
{
    nf_unregister_hook(&icmp_filter_ops);
    del_timer(&icmp_filter_timer);
    printk("icmp_filter_exit\n");
}

module_init(icmp_filter_init);
module_exit(icmp_filter_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("CXD");
MODULE_DESCRIPTION("icmp ping package filter");
 