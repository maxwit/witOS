#include <fs.h>
#include <errno.h>
#include <malloc.h>

#define MAX_CDEV 256

struct cdev {
	int minor;
	// const char *name;
	struct cdev *next;
};

struct cdev_type {
	// int major;
	const struct file_operations *fops;
	const char *name;
	struct cdev *cdev_list;
};

static struct cdev_type *g_cdev_type[MAX_CDEV];

int register_chrdev(int major, const struct file_operations *fops, const char *name)
{
	int i;
	struct cdev_type *cdt;

	if (!major) {
		for (i = MAX_CDEV - 1; i > 0; i--)
			if (!g_cdev_type[i])
				break;

		major = i;
	}

	if (major <= 0 || major >= MAX_CDEV || g_cdev_type[major])
		return -EBUSY;

	// TODO
	cdt = zalloc(sizeof(*cdt));
	// if

	cdt->fops = fops;
	cdt->name = name;
	// cdt->major = major;

	g_cdev_type[major] = cdt;

	return major;
}

int device_create(dev_t devt, const char *fmt, ...)
{
	int major = MAJOR(devt);
	struct cdev *cdev, **p;

	cdev = zalloc(sizeof(*cdev));
	// ..

	cdev->minor = MINOR(devt);
	// cdev->name = fmt; // FIXME
	cdev->next = NULL;

	for (p = &g_cdev_type[major]->cdev_list; *p; p = &(*p)->next)
		if ((*p)->minor == cdev->minor)
			return -EBUSY;

	*p = cdev;

	dev_mknod(fmt, 0666 | S_IFCHR, devt);

	return 0;
}

int devfs_cdev_open(struct file *fp, struct inode *inode)
{
	unsigned int major = imajor(inode), minor = iminor(inode);
	struct cdev *cdev;
	int (*open)(struct file *, struct inode *);

	if (major >= MAX_CDEV || !g_cdev_type[major])
		return -ENODEV;

	cdev = g_cdev_type[major]->cdev_list;
	while (cdev) {
		if (cdev->minor == minor)
			break;

		cdev = cdev->next;
	}

	if (!cdev)
		return -ENODEV;

	fp->f_op = g_cdev_type[major]->fops;
	if (!fp->f_op)
		return -ENOTSUPP;

	open = fp->f_op->open;
	if (open)
		return open(fp, inode);

	return 0;
}
