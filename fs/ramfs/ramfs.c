#include <stdio.h>
#include <init.h>
#include <malloc.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <block.h>
#include <dirent.h>
#include <fs.h>

struct ramfs_super_block {
	struct list_head *r_list;
};

struct ramfs_inode {
	struct inode vfs_inode;
	union {
		void *data;
	};
};

static unsigned long g_inode_count = 1;

static int ramfs_open(struct file *fp, struct inode *inode);
static int ramfs_close(struct file *fp);
static ssize_t ramfs_read(struct file *fp, void *buff, size_t size, loff_t *off);
static ssize_t ramfs_write(struct file *fp, const void *buff, size_t size, loff_t *off);

static const struct inode_operations ramfs_reg_inode_operations = {
};

static const struct file_operations ramfs_reg_file_operations = {
	.open  = ramfs_open,
	.close = ramfs_close,
	.read  = ramfs_read,
	.write = ramfs_write,
};

static int ramfs_lookup(struct inode *parent, struct dentry *dentry,
	struct nameidata *nd);
static int ramfs_mkdir(struct inode *, struct dentry *, umode_t);

static const struct inode_operations ramfs_dir_inode_operations = {
	.lookup = ramfs_lookup,
	.mkdir  = ramfs_mkdir,
};

static int ramfs_opendir(struct file *fp, struct inode *inode);
static int ramfs_readdir(struct file *, void *, filldir_t);

static const struct file_operations ramfs_dir_file_operations = {
	.open    = ramfs_opendir,
	.readdir = ramfs_readdir,
};

static inline struct ramfs_inode *RAM_I(struct inode *inode)
{
	return container_of(inode, struct ramfs_inode, vfs_inode);
}

static struct inode *ramfs_alloc_inode(struct super_block *sb)
{
	struct inode *inode;
	struct ramfs_inode *rin;

	rin = zalloc(sizeof(*rin));
	if (!rin)
		return NULL;

	inode = &rin->vfs_inode;
	inode->i_sb  = sb;
	inode->i_ino = g_inode_count++;

	return &rin->vfs_inode;
}

static inline struct ramfs_inode *ramfs_get_inode(struct super_block *sb, int ino)
{
	return NULL;
}

struct inode *ramfs_inode_create(struct super_block *sb, int mode)
{
	struct inode *inode;
	// struct ramfs_inode *rin;

	inode = ramfs_alloc_inode(sb);
	if (!inode) {
		// ...
		return NULL;
	}

	inode->i_mode = mode;

	// rin = RAM_I(inode);

	if (S_ISREG(inode->i_mode)) {
		inode->i_op = &ramfs_reg_inode_operations;
		inode->i_fop = &ramfs_reg_file_operations;
	} else if (S_ISDIR(inode->i_mode)) {
		inode->i_op = &ramfs_dir_inode_operations;
		inode->i_fop = &ramfs_dir_file_operations;
	} else {
		BUG();
	}

	return inode;
}

#if 0
struct inode *ramfs_iget(struct super_block *sb, unsigned long ino)
{
	struct inode *inode;
	// struct ramfs_inode *rin;

	inode = ramfs_alloc_inode(sb);
	if (!inode) {
		// ...
		return NULL;
	}

	inode->i_ino  = ino;

	// rin = RAM_I(inode);

	if (S_ISREG(inode->i_mode)) {
		inode->i_op = &ramfs_reg_inode_operations;
		inode->i_fop = &ramfs_reg_file_operations;
	} else if (S_ISDIR(inode->i_mode)) {
		inode->i_op = &ramfs_dir_inode_operations;
		inode->i_fop = &ramfs_dir_file_operations;
	} else {
		BUG();
	}

	return inode;
}
#endif

static int ramfs_fill_super(struct super_block *sb)
{
	int ret;
	struct ramfs_super_block *ramfs_sb;

	ramfs_sb = zalloc(sizeof(*ramfs_sb));
	if (!ramfs_sb) {
		ret = -ENOMEM;
		goto L1;
	}

	sb->s_fs_info = ramfs_sb;
	// sb->s_blocksize = RAM_BLK_SIZE;

	return 0;
L1:
	return ret;
}

static struct dentry *ramfs_mount(struct file_system_type *fs_type, int flags,
	const char *bdev_name, void *data)
{
	int ret;
	struct dentry *root;
	struct inode *in;
	struct super_block *sb;

	sb = sget(fs_type, NULL);
	if (!sb)
		return NULL;

	ret = ramfs_fill_super(sb);
	if (ret < 0) {
		// ...
		return NULL;
	}

	in = ramfs_inode_create(sb, S_IFDIR);
	if (!in) {
		// ...
		return NULL;
	}

	root = d_make_root(in);
	if (!root)
		return NULL;

	sb->s_root = root;

	return root;
}

// FIXME
static void ramfs_umount(struct super_block *sb)
{
}

static int ramfs_open(struct file *fp, struct inode *inode)
{
	return 0;
}

static int ramfs_close(struct file *fp)
{
	return 0;
}

static ssize_t ramfs_read(struct file *fp, void *buff, size_t size, loff_t *off)
{
	return 0;
}

static ssize_t
ramfs_write(struct file *fp, const void *buff, size_t size, loff_t *off)
{
	return 0;
}

static int
ramfs_lookup(struct inode *parent, struct dentry *dentry, struct nameidata *nd)
{
	return 0;
}

static int ramfs_opendir(struct file *fp, struct inode *inode)
{
	fp->private_data = fp->f_dentry->d_subdirs.next;
	return 0;
}

static int ramfs_readdir(struct file *fp, void *dirent, filldir_t filldir)
{
	struct dentry *de;
	struct inode *in;
	struct list_head *iter;

	if (fp->f_pos >= fp->f_dentry->d_inode->i_size)
		return 0;

	iter = fp->private_data;
	de = container_of(iter, struct dentry, d_child);

	in = de->d_inode;
	// TODO: fix the offset and type
	filldir(dirent, de->d_name.name, de->d_name.len, 0, in->i_ino, in->i_mode);

	fp->private_data = iter->next;
	fp->f_pos++;

	return sizeof(*de);
}

static int ramfs_mkdir(struct inode *parent, struct dentry *de, umode_t mode)
{
	struct inode *inode;

	inode = ramfs_alloc_inode(parent->i_sb);
	if (!inode)
		return -ENOMEM;

	// inode->i_ino = 1234; // FIXME
	inode->i_mode = mode;

	inode->i_op = &ramfs_dir_inode_operations;
	inode->i_fop = &ramfs_dir_file_operations;

	de->d_inode = inode;

	parent->i_size++;

	return 0;
}

static struct file_system_type ramfs_fs_type = {
	.name    = "ramfs",
	.mount   = ramfs_mount,
	.kill_sb = ramfs_umount,
};

static int __init ramfs_init(void)
{
	return register_filesystem(&ramfs_fs_type);
}

module_init(ramfs_init);
