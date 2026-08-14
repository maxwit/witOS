#include <stdio.h>
#include <init.h>
#include <malloc.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <dirent.h>
#include <fs.h>
#include <fs/devfs.h>

struct devfs_super_block {
	struct list_head *r_list;
};

struct devfs_inode {
	struct inode vfs_inode;
	char name[FILE_NAME_SIZE];
	struct list_head dev_node;
};

static LIST_HEAD(g_devfs_list);

int devfs_bdev_open(struct file *, struct inode *);

static const struct inode_operations devfs_bdev_inode_operations = {
};

static const struct file_operations devfs_bdev_file_operations = {
	.open  = devfs_bdev_open,
};

int devfs_cdev_open(struct file *, struct inode *);

static const struct inode_operations devfs_cdev_inode_operations = {
};

static const struct file_operations devfs_cdev_file_operations = {
	.open  = devfs_cdev_open,
};

static int devfs_lookup(struct inode *, struct dentry *,
	struct nameidata *);

static int devfs_mknod(struct inode *, struct dentry *, int, dev_t);

static const struct inode_operations devfs_dir_inode_operations = {
	.lookup = devfs_lookup,
	.mknod  = devfs_mknod,
};

static int devfs_opendir(struct file *, struct inode *);
static int devfs_readdir(struct file *, void *, filldir_t);

static const struct file_operations devfs_dir_file_operations = {
	.open    = devfs_opendir,
	.readdir = devfs_readdir,
};

static inline struct devfs_inode *DEV_I(struct inode *inode)
{
	return container_of(inode, struct devfs_inode, vfs_inode);
}

static struct inode *devfs_alloc_inode(struct super_block *sb)
{
	struct inode *inode;
	struct devfs_inode *di;

	di = zalloc(sizeof(*di));
	if (!di)
		return NULL;

	inode = &di->vfs_inode;
	inode->i_sb  = sb;
	inode->i_ino = 1234;

	return &di->vfs_inode;
}

static inline struct devfs_inode *devfs_get_inode(struct super_block *sb, int ino)
{
	return NULL;
}

struct inode *devfs_inode_create(struct super_block *sb, int mode)
{
	struct inode *inode;
	// struct devfs_inode *di;

	inode = devfs_alloc_inode(sb);
	if (!inode) {
		// ...
		return NULL;
	}

	inode->i_mode = mode;

	// di = DEV_I(inode);

	if (S_ISCHR(inode->i_mode)) {
		inode->i_op = &devfs_cdev_inode_operations;
		inode->i_fop = &devfs_cdev_file_operations;
	} else if (S_ISBLK(inode->i_mode)) {
		inode->i_op = &devfs_bdev_inode_operations;
		inode->i_fop = &devfs_bdev_file_operations;
	} else if (S_ISDIR(inode->i_mode)) {
		inode->i_op = &devfs_dir_inode_operations;
		inode->i_fop = &devfs_dir_file_operations;
	} else {
		BUG();
	}

	return inode;
}

static int devfs_fill_super(struct super_block *sb)
{
	int ret;
	struct devfs_super_block *devfs_sb;

	devfs_sb = zalloc(sizeof(*devfs_sb));
	if (!devfs_sb) {
		ret = -ENOMEM;
		goto L1;
	}

	sb->s_fs_info = devfs_sb;
	// sb->s_blocksize = BLK_SIZE;

	return 0;
L1:
	return ret;
}

static struct dentry *devfs_mount(struct file_system_type *fs_type,
	int flags, const char *bdev_name, void *data)
{
	int ret;
	struct dentry *root;
	struct inode *in;
	struct super_block *sb;

	// TODO: check whether devfs already mounted somewhere

	sb = sget(fs_type, NULL);
	if (!sb)
		return NULL;

	ret = devfs_fill_super(sb);
	if (ret < 0) {
		// ...
		return NULL;
	}

	in = devfs_inode_create(sb, S_IFDIR);
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
static void devfs_kill_sb(struct super_block *sb)
{
}

static int devfs_lookup(struct inode *parent, struct dentry *dentry,
	struct nameidata *nd)
{
	struct devfs_inode *di;

	list_for_each_entry(di, &g_devfs_list, dev_node) {
		if (!strncmp(di->name, dentry->d_name.name, dentry->d_name.len)) {
			dentry->d_inode = &di->vfs_inode;
			return 0;
		}
	}

	return -ENOENT;
}

static int devfs_opendir(struct file *fp, struct inode *inode)
{
	fp->private_data = fp->f_dentry->d_subdirs.next;
	return 0;
}

static int devfs_readdir(struct file *fp, void *dirent, filldir_t filldir)
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

	return 1;
}

static int devfs_mknod(struct inode *dir, struct dentry *dentry,
	int mode, dev_t dev)
{
	struct inode *in;
	struct devfs_inode *di;

	in = devfs_inode_create(dir->i_sb, mode);
	if (!in) {
		GEN_DBG("fail to create devfs inode!\n");
		return -ENOMEM;
	}

	in->i_rdev = dev;

	di = DEV_I(in);
	strncpy(di->name, dentry->d_name.name, dentry->d_name.len);

	// dentry->d_inode = in;
	dir->i_size++;

	list_add_tail(&di->dev_node, &g_devfs_list);

	return 0;
}

static struct file_system_type devfs_fs_type = {
	.name    = "devfs",
	.mount   = devfs_mount,
	.kill_sb = devfs_kill_sb,
};

static int __init devfs_init(void)
{
	return register_filesystem(&devfs_fs_type);
}

module_init(devfs_init);
