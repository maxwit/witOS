#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <fcntl.h>
#include <assert.h>
#include <fs.h>
#include <init.h>

static struct file_system_type *fs_type_list;
static LIST_HEAD(g_mount_list);

int register_filesystem(struct file_system_type *fstype)
{
	struct file_system_type **p;

	for (p = &fs_type_list; *p; p = &(*p)->next) {
		if (!strcmp((*p)->name, fstype->name))
			return -EBUSY;
	}

	fstype->next = NULL;
	*p = fstype;

	return 0;
}

struct file_system_type *get_fs_type(const char *name)
{
	struct file_system_type *p;

	for (p = fs_type_list; p; p = p->next) {
		if (!strcmp(p->name, name))
			return p;
	}

	return NULL;
}

static int do_mount(const char *type, unsigned long flags, const char *dev_name,
	struct vfsmount *vfsmnt)
{
	struct dentry *root;
	struct file_system_type *fstype;

	fstype = get_fs_type(type);
	if (NULL == fstype) {
		DPRINT("fail to find file system type %s!\n", type == NULL ? "" : type);
		return -ENOENT;
	}

	root = fstype->mount(fstype, flags, dev_name, NULL);
	if (!root) {
		DPRINT("fail to mount %s!\n", dev_name);
		return -EIO; // FIXME!
	}

	vfsmnt->root = root;
	vfsmnt->fstype = fstype;
	vfsmnt->dev_name = dev_name;

	return 0;
}

static int add_mount(struct mount *mnt, struct path *path,
	unsigned int flags)
{
	// int ret;
	// struct mount *mnt;

#if 0
	if (!(flags & MS_NODEV)) {
		list_for_each_entry(mnt, &g_mount_list, mnt_hash) {
			if (mnt->dev_name && !strcmp(mnt->dev_name, dev_name)) {
				GEN_DBG("device \"%s\" already mounted!\n", dev_name);
				return -EBUSY;
			}
		}
	}
#endif

	mnt->mountpoint = path->dentry;
	mnt->mnt_parent = path->mnt;

	list_add_tail(&mnt->mnt_hash, &g_mount_list);

	return 0;
}

int sys_mount(const char *dev_name, const char *path,
		const char *type, unsigned long flags)
{
	int ret;
	struct nameidata nd;
	struct mount *mnt;

	if (NULL == type)
		return -EINVAL;

	ret = path_walk(path, &nd); // FIXME: directory only!
	if (ret < 0) {
		GEN_DBG("\"%s\" does NOT exist!\n");
		return ret;
	}

	mnt = zalloc(sizeof(*mnt));
	if (NULL == mnt)
		return -ENOMEM;

	ret = do_mount(type, flags, dev_name, &mnt->vfsmnt);
	if (ret < 0)
		goto L1;

	ret = add_mount(mnt, &nd.path, flags);
	if (ret < 0)
		goto L1;

	return 0;

L1:
	free(mnt);
	return ret;
}

int __init mount_root(const char *dev_name, const char *type,
	unsigned long flags)
{
	int ret;
	struct mount *mnt;
	struct path path, sysroot;

	if (NULL == type)
		return -EINVAL;

	mnt = zalloc(sizeof(*mnt));
	if (NULL == mnt)
		return -ENOMEM;

	ret = do_mount(type, flags, dev_name, &mnt->vfsmnt);
	if (ret < 0)
		goto L1;

	path.dentry = NULL;
	path.mnt = NULL;
	ret = add_mount(mnt, &path, flags);
	if (ret < 0)
		goto L1;

	sysroot.dentry = mnt->vfsmnt.root;
	sysroot.mnt = mnt;
	set_fs_root(&sysroot);
	set_fs_pwd(&sysroot);

	return 0;

L1:
	free(mnt);
	return ret;
}

int GAPI mount(const char *source, const char *target, const char *type,
	unsigned long flags)
{
	return sys_mount(source, target, type, flags);
}

int GAPI umount(const char *mnt)
{
	return 0;
}

struct mount *lookup_mnt(struct path *path)
{
	struct mount *mnt;

	list_for_each_entry(mnt, &g_mount_list, mnt_hash) {
		if (mnt->mountpoint == path->dentry)
			return mnt;
	}

	return NULL;
}

// TODO: move to app layer
int list_mount()
{
	int count = 0;
	struct mount *mnt;

	printf("NO. device   mnt  type\n"
		"-------------------------\n");
	list_for_each_entry(mnt, &g_mount_list, mnt_hash) {
		count++;
		printf("(%d) %-8s %-4s %s\n", count,
			mnt->vfsmnt.dev_name ? mnt->vfsmnt.dev_name : "none",
			mnt->mountpoint ? mnt->mountpoint->d_name.name : "/", // FIXME
			mnt->vfsmnt.fstype->name);
	}

	return count;
}
