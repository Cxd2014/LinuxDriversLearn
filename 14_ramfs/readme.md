### 什么是ramfs

ramfs是一个非常简单的文件系统，他利用Linux的磁盘缓存（数据缓存和目录缓存）机制
实现了一个大小动态可变的基于内存的文件系统。

通常Linux中的所有文件都缓存在内存中。从块设备中读取的数据页缓存在内存中以备再次访问，
但是他们被标记为`clean`（可被释放）以便虚拟内存系统需要时可以使用该内存页。
类似的，当数据被写入到块设备之后此数据块会被立即标记为`clean`，
但是它仍然被故意保留在缓冲区中直到VM需要使用这块内存时才被释放。
目录缓存区也使用相似的机制来加速目录项的访问。

ramfs没有实际的后备存储设备。写入到ramfs中的文件也会被分配目录和数据缓存，
但是没有地方写回。这意味着缓存页不可能被标记为`clean`，因此它们不会被VM释放以便重复利用。
实现ramfs的代码非常简短，因为所有的工作都可以借助Linux现有的缓存机制来完成。

此代码是大部分是参照：/linux-2.6.36/fs/ramfs目录下的源码实现的，
主要目的是了解文件系统的具体挂载过程。

参考文章：

[从ramfs分析文件系统的设计和实现](http://blog.csdn.net/ganggexiongqi/article/details/8921643)

### HowToDo

测试方法：

0. $ make  
1. $ insmod cxdfs.ko  
2. $ cat /proc/filesystems  
3. $ mount -t cxdfs none /mnt  
4. $ cd /mnt  
5. $ mkdir hello  
6. $ rm -rf hello  
7. $ echo "hello" > cxd  
8. $ umount -v /mnt
9. $ rmmod cxdfs
