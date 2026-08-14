#include <malloc.h>
#include <errno.h>
#include <string.h>
#include <fs.h>

#define MAX_FDS 256

static struct file *fd_array[MAX_FDS];
static struct fs_struct g_fs;

struct super_block *sget(struct file_system_type *type, void *data)
{
	struct super_block *sb;

	sb = zalloc(sizeof(*sb));
	if (!sb) {
		// ...
		return NULL;
	}

	sb->s_bdev = data;

	return sb;
}

struct dentry *mount_bdev(struct file_system_type *fs_type,
	int flags, const char *dev_name, void *data,
	int (*fill_super)(struct super_block *, void *, int))
{
	int ret;
	struct super_block *sb;
	struct block_device *bdev;

	bdev = bdev_get(dev_name);
	if (NULL == bdev) {
		DPRINT("fail to open block device \"%s\"!\n", dev_name);
		return NULL;
	}

	sb = sget(fs_type, bdev);
	if (!sb)
		return NULL;

	ret = fill_super(sb, data, 0);
	if (ret < 0)
		return NULL;

	return sb->s_root;
}

struct dentry *d_alloc(struct dentry *parent, const struct qstr *str)
{
	struct dentry *de = __d_alloc(parent->d_sb, str);

	de->d_parent = parent;
	list_add_tail(&de->d_child, &parent->d_subdirs);

	return de;
}

struct dentry *__d_alloc(struct super_block *sb, const struct qstr *str)
{
	char *name;
	struct dentry *de;

	de = zalloc(sizeof(*de));
	if (!de)
		return NULL;

	de->d_sb = sb;
	de->d_parent = de;
	INIT_LIST_HEAD(&de->d_child);
	INIT_LIST_HEAD(&de->d_subdirs);

	de->d_name.len = str->len;
	if (str->len >= DNAME_INLINE_LEN) {
		name = malloc(str->len + 1);
		if (!name) {
			free(de);
			return NULL;
		}
	} else {
		name = de->d_iname;
	}
	de->d_name.name = name;
	strncpy(name, str->name, str->len);
	name[str->len] = '\0';

	return de;
}

struct dentry *d_make_root(struct inode *root_inode)
{
	struct dentry *root_dir = NULL;

	if (root_inode) {
		static const struct qstr name = {.name = "/", .len = 1};

		root_dir = __d_alloc(root_inode->i_sb, &name);
		if (root_dir)
			root_dir->d_inode = root_inode;
	}

	return root_dir;
}

void dput(struct dentry *dentry)
{
	list_del(&dentry->d_child);
}

struct inode *iget(struct super_block *sb, unsigned long ino)
{
	struct inode *inode;
	// TODO: search

	inode = zalloc(sizeof(*inode)); // FIXME
	// if null

	inode->i_sb = sb;
	inode->i_ino = ino;

	return inode;
}

int get_unused_fd()
{
	int fd;

	for (fd = 0; fd < MAX_FDS; fd++) {
		if (!fd_array[fd])
			return fd;
	}

	return -EBUSY;
}

int fd_install(int fd, struct file *fp)
{
	fd_array[fd] = fp;
	return 0;
}

struct file *fget(unsigned int fd)
{
	if (fd < 0 || fd >= MAX_FDS)
		return NULL; // -EINVAL;

	return fd_array[fd];
}

int vfs_mkdir(struct inode *dir, struct dentry *dentry, int mode)
{
	int errno = -ENOTSUPP;

	if (dir->i_op->mkdir)
		errno = dir->i_op->mkdir(dir, dentry, mode);

	return errno;
}

int vfs_mknod(struct inode *dir, struct dentry *dentry, int mode, dev_t devt)
{
	if (!dir->i_op->mknod)
		return -ENOTSUPP;

	return dir->i_op->mknod(dir, dentry, mode, devt);
}

struct fs_struct *get_curr_fs()
{
	return &g_fs;
}

void set_fs_root(const struct path *root)
{
	g_fs.root = *root;
}

void set_fs_pwd(const struct path *pwd)
{
	g_fs.pwd = *pwd;
}

void get_fs_root(struct path *root)
{
	*root = g_fs.root;
}

void get_fs_pwd(struct path *pwd)
{
	*pwd = g_fs.pwd;
}

long sys_mkdir(const char *name, unsigned int /*FIXME*/ mode)
{
	int ret;
	struct nameidata nd;
	struct dentry *de;
	struct qstr unit;

	// ret = path_walk(name, &nd);

	get_fs_pwd(&nd.path);

	while ('/' == *name) name++; // FIXME
	unit.name = name;
	unit.len = strlen(name);

	de = d_alloc(nd.path.dentry, &unit);

	ret = vfs_mkdir(nd.path.dentry->d_inode, de, mode | S_IFDIR);
	if (ret < 0) {
		// FIXME: use d_free() instead
		list_del(&de->d_child);
		free(de);
	}

	return ret;
}

long sys_chdir(const char *path)
{
	int ret;
	struct nameidata nd;

	ret = path_walk(path, &nd);
	if (ret < 0)
		return ret;

	set_fs_pwd(&nd.path);

	return 0;
}

#define PATH_STACK_SIZE 256

long sys_getcwd(char *buff, unsigned long size)
{
	int top = 0, count;
	struct dentry *stack[PATH_STACK_SIZE];
	struct path cwd, root;

	get_fs_pwd(&cwd);
	get_fs_root(&root);

	while (cwd.dentry != root.dentry) {
		if (cwd.dentry == cwd.mnt->vfsmnt.root) {
			follow_up(&cwd);
			continue; // yes, we'd jump to next loop
		}

		stack[top] = cwd.dentry;
		top++;
		if (top >= PATH_STACK_SIZE) {
			// ret = -EOVERFLOW;
			break;
		}

		cwd.dentry = cwd.dentry->d_parent;
	}

	if (0 == top) {
		buff[0] = '/';
		buff[1] = '\0';
		return 1;
	}

	count = 0;
	while (--top >= 0) {
		buff[count++] = '/';
		strcpy(buff + count, stack[top]->d_name.name);
		count += stack[top]->d_name.len;

		if (count >= size) {
			// ret = -EOVERFLOW;
			break;
		}
	}

	return count;
}

void d_instantiate(struct dentry *entry, struct inode * inode)
{
	entry->d_inode = inode;
}
