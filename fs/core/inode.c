#include <fs.h>
#include <errno.h>
#include <string.h>

// FIXME!
int dev_mknod(const char *path, int mode, dev_t devt)
{
	int ret;
	struct qstr name;
	struct nameidata nd;
	struct dentry *parent, *dentry;

	ret = path_walk("/dev", &nd);
	if (ret < 0)
		return ret;

	parent = nd.path.dentry;
	name.name = path;
	name.len  = strlen(path);

	dentry = d_alloc(parent, &name);
	if (!dentry)
		return -ENOMEM;

	ret = vfs_mknod(parent->d_inode, dentry, mode, devt);
	if (ret < 0) {
		GEN_DBG("fail to create \"%s\" (errno = %d)!\n", path, ret);
		return ret;
	}

	return 0;
}
