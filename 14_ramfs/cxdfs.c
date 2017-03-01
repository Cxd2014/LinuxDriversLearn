#include <linux/module.h>
#include <linux/fs.h>
#include <linux/pagemap.h>
#include <linux/highmem.h>
#include <linux/time.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/backing-dev.h>
#include <linux/ramfs.h>
#include <linux/sched.h>
#include <linux/parser.h>
#include <linux/magic.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/mm.h>

#define CXDFS_MAGIC		0x12344321
#define CXDFS_DEFAULT_MODE	0755

static const struct super_operations cxdfs_ops;
static const struct inode_operations cxdfs_dir_inode_operations;


static int cxd__set_page_dirty_no_writeback(struct page *page)
{
	if (!PageDirty(page))
		SetPageDirty(page);
	return 0;
}

/*
* 该结构中为ramfs的“地址空间”结构。"地址空间"结构，
* 将缓存数据与其来源之间建立关联。该结构用于ramfs向内存模块申请内存页，并提供
* 从后备数据来源读取数据并填充缓冲区以及将缓冲区中的数据写入后备设备等操作。
*/
const struct address_space_operations cxdfs_aops = {
	.readpage	= simple_readpage,
	.write_begin	= simple_write_begin,
	.write_end	= simple_write_end,
	.set_page_dirty = cxd__set_page_dirty_no_writeback,
};

/* 文件函数操作集 */
const struct file_operations cxdfs_file_operations = {
	.read		= do_sync_read,
	.aio_read	= generic_file_aio_read,
	.write		= do_sync_write,
	.aio_write	= generic_file_aio_write,
	.mmap		= generic_file_mmap,
	.fsync		= noop_fsync,
	.splice_read	= generic_file_splice_read,
	.splice_write	= generic_file_splice_write,
	.llseek		= generic_file_llseek,
};

/* 节点函数操作集 */
const struct inode_operations cxdfs_file_inode_operations = {
	.setattr	= simple_setattr,
	.getattr	= simple_getattr,
};

/* 该文件系统的后备存储设备 */
static struct backing_dev_info cxdfs_backing_dev_info = {
	.name		= "cxdfs",
	.ra_pages	= 0,	/* No readahead */
	.capabilities	= BDI_CAP_NO_ACCT_AND_WRITEBACK |
			  BDI_CAP_MAP_DIRECT | BDI_CAP_MAP_COPY |
			  BDI_CAP_READ_MAP | BDI_CAP_WRITE_MAP | BDI_CAP_EXEC_MAP,
};

/* 创建VFS的inode节点，并指定该节点的函数操作集 */
struct inode *cxdfs_get_inode(struct super_block *sb,
				const struct inode *dir, int mode, dev_t dev)
{
	struct inode * inode = new_inode(sb);

	if (inode) {
		inode_init_owner(inode, dir, mode);
		inode->i_mapping->a_ops = &cxdfs_aops;
		inode->i_mapping->backing_dev_info = &cxdfs_backing_dev_info;
		mapping_set_gfp_mask(inode->i_mapping, GFP_HIGHUSER);
		mapping_set_unevictable(inode->i_mapping);
		inode->i_atime = inode->i_mtime = inode->i_ctime = CURRENT_TIME;
		switch (mode & S_IFMT) {
		default:
			init_special_inode(inode, mode, dev);
			break;
		case S_IFREG: /* 创建文件的inode并指定函数操作集 */
			inode->i_op = &cxdfs_file_inode_operations;
			inode->i_fop = &cxdfs_file_operations;
			break;
		case S_IFDIR: /* 创建目录的inode并指定函数操作集 */
			inode->i_op = &cxdfs_dir_inode_operations;
			inode->i_fop = &simple_dir_operations;

			/* directory inodes start off with i_nlink == 2 (for "." entry) */
			inc_nlink(inode);
			break;
		case S_IFLNK: /* 创建链接的inode并指定函数操作集 */
			inode->i_op = &page_symlink_inode_operations;
			break;
		}
	}
	return inode;
}

/*
 * File creation. Allocate an inode, and we're done..
 */
/* SMP-safe */
static int
cxdfs_mknod(struct inode *dir, struct dentry *dentry, int mode, dev_t dev)
{
	/* 获取一个 inode 节点 */
	struct inode * inode = cxdfs_get_inode(dir->i_sb, dir, mode, dev);
	int error = -ENOSPC;
	printk("cxdfs_mknod\n");
	if (inode) {
		 /* 初始化 inode 节点 */
		d_instantiate(dentry, inode);
		dget(dentry);	/* Extra count - pin the dentry in core */
		error = 0;
		dir->i_mtime = dir->i_ctime = CURRENT_TIME;
	}
	return error;
}

/* 创建目录 */
static int cxdfs_mkdir(struct inode * dir, struct dentry * dentry, int mode)
{
	int retval = cxdfs_mknod(dir, dentry, mode | S_IFDIR, 0);
	if (!retval)
		inc_nlink(dir);
	return retval;
}

/* 创建普通文件 */
static int cxdfs_create(struct inode *dir, struct dentry *dentry, int mode, struct nameidata *nd)
{
	return cxdfs_mknod(dir, dentry, mode | S_IFREG, 0);
}

/* 创建链接文件 */
static int cxdfs_symlink(struct inode * dir, struct dentry *dentry, const char * symname)
{
	struct inode *inode;
	int error = -ENOSPC;

	inode = cxdfs_get_inode(dir->i_sb, dir, S_IFLNK|S_IRWXUGO, 0);
	if (inode) {
		int l = strlen(symname)+1;
		error = page_symlink(inode, symname, l);
		if (!error) {
			d_instantiate(dentry, inode);
			dget(dentry);
			dir->i_mtime = dir->i_ctime = CURRENT_TIME;
		} else
			iput(inode);
	}
	return error;
}

/* 目录操作函数集 */
static const struct inode_operations cxdfs_dir_inode_operations = {
	.create		= cxdfs_create,
	.lookup		= simple_lookup,
	.link		= simple_link,
	.unlink		= simple_unlink,
	.symlink	= cxdfs_symlink,
	.mkdir		= cxdfs_mkdir,
	.rmdir		= simple_rmdir,
	.mknod		= cxdfs_mknod,
	.rename		= simple_rename,
};

/* 超级块操作函数集 */
static const struct super_operations cxdfs_ops = {
	.statfs		= simple_statfs,           /* 文件系统的统计信息 */
	.drop_inode	= generic_delete_inode,    /* 当节点的引用计数为0时，删除该节点 */
	.show_options	= generic_show_options,/* 显示挂载选项 */
};

struct cxdfs_mount_opts {
	umode_t mode;
};

enum {
	Opt_mode,
	Opt_err
};

static const match_table_t tokens = {
	{Opt_mode, "mode=%o"},
	{Opt_err, NULL}
};

struct cxdfs_fs_info {
	struct cxdfs_mount_opts mount_opts;
};

static int cxdfs_parse_options(char *data, struct cxdfs_mount_opts *opts)
{
	substring_t args[MAX_OPT_ARGS];
	int option;
	int token;
	char *p;

	opts->mode = CXDFS_DEFAULT_MODE;

	while ((p = strsep(&data, ",")) != NULL) {
		if (!*p)
			continue;

		token = match_token(p, tokens, args);
		switch (token) {
		case Opt_mode:
			if (match_octal(&args[0], &option))
				return -EINVAL;
			opts->mode = option & S_IALLUGO;
			break;
		/*
		 * We might like to report bad mount options here;
		 * but traditionally cxdfs has ignored all mount options,
		 * and as it is used as a !CONFIG_SHMEM simple substitute
		 * for tmpfs, better continue to ignore other mount options.
		 */
		}
	}

	return 0;
}

/* 初始化VFS的超级块 */
int cxdfs_fill_super(struct super_block *sb, void *data, int silent)
{
	struct cxdfs_fs_info *fsi;
	struct inode *inode = NULL;
	struct dentry *root;
	int err;

	save_mount_options(sb, data);

	fsi = kzalloc(sizeof(struct cxdfs_fs_info), GFP_KERNEL);
	sb->s_fs_info = fsi;
	if (!fsi) {
		err = -ENOMEM;
		goto fail;
	}

	/* 解析 mount 命令挂载时传递的参数 */
	err = cxdfs_parse_options(data, &fsi->mount_opts);
	if (err)
		goto fail;

	sb->s_maxbytes		= MAX_LFS_FILESIZE;    /* 文件的最大大小 */
	sb->s_blocksize		= PAGE_CACHE_SIZE;     /* 块大小 */
	sb->s_blocksize_bits	= PAGE_CACHE_SHIFT;
	sb->s_magic		= CXDFS_MAGIC;             /* 此文件系统的魔数 */
	sb->s_op		= &cxdfs_ops;              /* 指定超级块操作函数集 */
	sb->s_time_gran		= 1; 

	 /* 获取一个 inode 节点 */
	inode = cxdfs_get_inode(sb, NULL, S_IFDIR | fsi->mount_opts.mode, 0);
	if (!inode) {
		err = -ENOMEM;
		goto fail;
	}
	/* 将此节点作为根节点 */
	root = d_alloc_root(inode);
	sb->s_root = root;
	if (!root) {
		err = -ENOMEM;
		goto fail;
	}

	return 0;
fail:
	kfree(fsi);
	sb->s_fs_info = NULL;
	iput(inode);
	return err;
}

int cxdfs_get_sb(struct file_system_type *fs_type,
	int flags, const char *dev_name, void *data, struct vfsmount *mnt)
{
	return get_sb_nodev(fs_type, flags, data, cxdfs_fill_super, mnt);
}

static void cxdfs_kill_sb(struct super_block *sb)
{
	kfree(sb->s_fs_info);
	kill_litter_super(sb);
}

static struct file_system_type cxdfs_fs_type = {
	.name		= "cxdfs",
	.get_sb		= cxdfs_get_sb,  /* 文件系统的挂载函数 */
	.kill_sb	= cxdfs_kill_sb, /* 文件系统的卸载函数 */
};


static int __init init_cxdfs_fs(void)
{
	printk("init_cxdfs_fs\n");
	return register_filesystem(&cxdfs_fs_type);
}

static void __exit exit_cxdfs_fs(void)
{
	printk("exit_cxdfs_fs\n");
	unregister_filesystem(&cxdfs_fs_type);
}

module_init(init_cxdfs_fs);
module_exit(exit_cxdfs_fs);
MODULE_LICENSE("GPL");

