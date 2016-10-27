/*
* netfilter加令牌桶算法，实现ping包的限流功能
* 基于数据包个数的限流
* 每4秒只能发送一个ping包，多余的数据包直接扔掉
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

#define ICMP_PROTOCOL 1
#define PING_PACKAGE 0

#define MAX_NUM_PACKAGE 10
#define TIME_SECOND 4

static struct timer_list icmp_filter_timer;
static int index = 0;

unsigned int icmp_filter_hook(unsigned int hooknum,
    struct sk_buff *pskb,
    const struct net_device *in,
    const struct net_device *out,
    int (*okfn)(struct sk_buff *))  
{
    struct iphdr *iph;
    struct icmphdr *icmph;
    int icmp_ret = NF_ACCEPT;
    
    iph = ip_hdr(pskb);

    if(iph->protocol == ICMP_PROTOCOL){
        icmph = icmp_hdr(pskb);
        if(icmph->type == PING_PACKAGE){
            if(index > 0){
                index--;
                icmp_ret = NF_ACCEPT;
            }else{
                printk("ip saddr = %pI4 \n",&(iph->saddr));
                printk("ip daddr = %pI4 \n",&(iph->daddr));
                icmp_ret = NF_DROP; 
            }
        }   
    }

    return icmp_ret;
}

static void icmp_filter_timer_callback(unsigned long data)
{
    if(index < MAX_NUM_PACKAGE)
        index++;

    mod_timer(&icmp_filter_timer, jiffies + TIME_SECOND*HZ);
}

static struct nf_hook_ops icmp_filter_ops = {
    .hook		= icmp_filter_hook,
    .owner		= THIS_MODULE,
    .pf			= PF_INET,
    .hooknum	= NF_IP_POST_ROUTING,
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
 