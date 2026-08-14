#pragma once

#include <types.h>
#include <list.h>
#include <time.h>
#include <block.h>

/*
 * Attribute flags.  These should be or-ed together to figure out what
 * has been changed!
 */
#define ATTR_MODE	(1 << 0)
#define ATTR_UID	(1 << 1)
#define ATTR_GID	(1 << 2)
#define ATTR_SIZE	(1 << 3)
#define ATTR_ATIME	(1 << 4)
#define ATTR_MTIME	(1 << 5)
#define ATTR_CTIME	(1 << 6)
#define ATTR_ATIME_SET	(1 << 7)
#define ATTR_MTIME_SET	(1 << 8)
#define ATTR_FORCE	(1 << 9) /* Not a change, but a change it */
#define ATTR_ATTR_FLAG	(1 << 10)
#define ATTR_KILL_SUID	(1 << 11)
#define ATTR_KILL_SGID	(1 << 12)
#define ATTR_FILE	(1 << 13)
#define ATTR_KILL_PRIV	(1 << 14)
#define ATTR_OPEN	(1 << 15) /* Truncating from open(O_TRUNC) */
#define ATTR_TIMES_SET	(1 << 16)

/* fs/block_dev.c */
#define BDEVNAME_SIZE	32	/* Largest string for a blockdev identifier */
#define BDEVT_SIZE	10	/* Largest string for MAJ:MIN for blkdev */

#define MS_RDONLY	 1	/* Mount read-only */
#define MS_NOSUID	 2	/* Ignore suid and sgid bits */
#define MS_NODEV	 4	/* Disallow access to device special files */
#define MS_NOEXEC	 8	/* Disallow program execution */
#define MS_SYNCHRONOUS	16	/* Writes are synced at once */
#define MS_REMOUNT	32	/* Alter flags of a mounted FS */
#define MS_MANDLOCK	64	/* Allow mandatory locks on an FS */
#define MS_DIRSYNC	128	/* Directory modifications are synchronous */
#define MS_NOATIME	1024	/* Do not update access times. */
#define MS_NODIRATIME	2048	/* Do not update directory access times */
#define MS_BIND		4096
#define MS_MOVE		8192
#define MS_REC		16384
#define MS_VERBOSE	32768	/* War is peace. Verbosity is silence.
				   MS_VERBOSE is deprecated. */
#define MS_SILENT	32768
#define MS_POSIXACL	(1<<16)	/* VFS does not apply the umask */
#define MS_UNBINDABLE	(1<<17)	/* change to unbindable */
#define MS_PRIVATE	(1<<18)	/* change to private */
#define MS_SLAVE	(1<<19)	/* change to slave */
#define MS_SHARED	(1<<20)	/* change to shared */
#define MS_RELATIME	(1<<21)	/* Update atime relative to mtime/ctime. */
#define MS_KERNMOUNT	(1<<22) /* this is a kern_mount call */
#define MS_I_VERSION	(1<<23) /* Update inode I_version field */
#define MS_STRICTATIME	(1<<24) /* Always perform atime updates */
#define MS_NOSEC	(1<<28)
#define MS_BORN		(1<<29)
#define MS_ACTIVE	(1<<30)
#define MS_NOUSER	(1<<31)

/*
 * Superblock flags that can be altered by MS_REMOUNT
 */
#define MS_RMT_MASK	(MS_RDONLY|MS_SYNCHRONOUS|MS_MANDLOCK|MS_I_VERSION)

struct mount;
struct super_block;
struct inode;
struct dentry;
struct file;

struct qstr {
	const char *name;
	unsigned int len;
};

struct path {
	struct mount *mnt;
	struct dentry *dentry;
};

struct nameidata {
	struct path path;
	unsigned int flag;
};

struct file_system_type {
	const char *name;
	struct file_system_type *next;

	struct dentry *(*mount)(struct file_system_type *, int,
						const char *, void *);
	void (*kill_sb)(struct super_block *);
	int (*check_fs_type)(const char *);
};

int register_filesystem(struct file_system_type *);

struct file_system_type *get_fs_type(const char *);

struct vfsmount {
	struct dentry *root;
	struct file_system_type *fstype;
	const char *dev_name;
};

struct mount {
	struct mount *mnt_parent;
	struct dentry *mountpoint;
	struct vfsmount vfsmnt;
	struct list_head mnt_hash;
};

struct block_buff {
	// __u32  blk_id;
	__u8    *blk_base;
	size_t  blk_size;
	size_t  max_size;
	__u8    *blk_off;
};

struct linux_dirent;
typedef int (*filldir_t)(void *, const char *, int, loff_t, u64, unsigned);

struct file_operations {
	int (*open)(struct file *, struct inode *);
	int (*close)(struct file *);
	ssize_t (*read)(struct file *, void *, size_t, loff_t *);
	ssize_t (*write)(struct file *, const void *, size_t, loff_t *);
	int (*ioctl)(struct file *, int, unsigned long);
	loff_t (*lseek)(struct file *, loff_t, int);
	int (*readdir)(struct file *, void *, filldir_t);
};

// describe an opened file
struct file {
	loff_t f_pos;
	unsigned int flags;
	// unsigned int mode;
	const struct file_operations *f_op;
	struct dentry *f_dentry;
	// struct mount *vfsmnt;
	struct block_buff blk_buf; // for block device, to be removed!
	u64   f_version;
	void *private_data;
};

#define S_IFMT  00170000
#define S_IFSOCK 0140000
#define S_IFLNK	 0120000
#define S_IFREG  0100000
#define S_IFBLK  0060000
#define S_IFDIR  0040000
#define S_IFCHR  0020000
#define S_IFIFO  0010000
#define S_ISUID  0004000
#define S_ISGID  0002000
#define S_ISVTX  0001000

#define S_ISLNK(m)	(((m) & S_IFMT) == S_IFLNK)
#define S_ISREG(m)	(((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m)	(((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK(m)	(((m) & S_IFMT) == S_IFBLK)
#define S_ISFIFO(m)	(((m) & S_IFMT) == S_IFIFO)
#define S_ISSOCK(m)	(((m) & S_IFMT) == S_IFSOCK)

#define S_IRWXU 00700
#define S_IRUSR 00400
#define S_IWUSR 00200
#define S_IXUSR 00100

#define S_IRWXG 00070
#define S_IRGRP 00040
#define S_IWGRP 00020
#define S_IXGRP 00010

#define S_IRWXO 00007
#define S_IROTH 00004
#define S_IWOTH 00002
#define S_IXOTH 00001

#define S_IRWXUGO	(S_IRWXU|S_IRWXG|S_IRWXO)
#define S_IALLUGO	(S_ISUID|S_ISGID|S_ISVTX|S_IRWXUGO)
#define S_IRUGO		(S_IRUSR|S_IRGRP|S_IROTH)
#define S_IWUGO		(S_IWUSR|S_IWGRP|S_IWOTH)
#define S_IXUGO		(S_IXUSR|S_IXGRP|S_IXOTH)

struct iattr {
	unsigned int	ia_valid;
	umode_t		ia_mode;
	uid_t		ia_uid;
	gid_t		ia_gid;
	loff_t		ia_size;
	struct timespec	ia_atime;
	struct timespec	ia_mtime;
	struct timespec	ia_ctime;
};

struct kstat {
	loff_t size;
};

struct inode_operations {
	int (*lookup)(struct inode *, struct dentry *, struct nameidata *);
	int (*create)(struct inode *, struct dentry *, umode_t, struct nameidata *);
	int (*mkdir)(struct inode *,struct dentry *, umode_t);
	int (*rmdir)(struct inode *,struct dentry *);
	int (*mknod)(struct inode *,struct dentry *, int, dev_t);
	int (*getattr) (struct vfsmount *, struct dentry *, struct kstat *);
};

#define I_DIRTY_SYNC		(1 << 0)
#define I_DIRTY_DATASYNC	(1 << 1)
#define I_DIRTY_PAGES		(1 << 2)
#define __I_NEW			3
#define I_NEW			(1 << __I_NEW)
#define I_WILL_FREE		(1 << 4)
#define I_FREEING		(1 << 5)
#define I_CLEAR			(1 << 6)
#define __I_SYNC		7
#define I_SYNC			(1 << __I_SYNC)
#define I_REFERENCED		(1 << 8)
#define __I_DIO_WAKEUP		9
#define I_DIO_WAKEUP		(1 << I_DIO_WAKEUP)

#define I_DIRTY (I_DIRTY_SYNC | I_DIRTY_DATASYNC | I_DIRTY_PAGES)

/* Inode flags - they have nothing to superblock flags now */

#define S_SYNC		1	/* Writes are synced at once */
#define S_NOATIME	2	/* Do not update access times */
#define S_APPEND	4	/* Append-only file */
#define S_IMMUTABLE	8	/* Immutable file */
#define S_DEAD		16	/* removed, but still open directory */
#define S_NOQUOTA	32	/* Inode is not counted to quota */
#define S_DIRSYNC	64	/* Directory modifications are synchronous */
#define S_NOCMTIME	128	/* Do not update file c/mtime */
#define S_SWAPFILE	256	/* Do not truncate: swapon got its bmaps */
#define S_PRIVATE	512	/* Inode is fs-internal */
#define S_IMA		1024	/* Inode has an associated IMA struct */
#define S_AUTOMOUNT	2048	/* Automount/referral quasi-directory */
#define S_NOSEC		4096	/* no suid or xattr security attributes */

struct inode {
	unsigned long i_ino;
	loff_t        i_size;
	__u32         i_mode;
	int           i_nlink;
	int           i_count;
	int           i_version;

	uid_t			i_uid;
	gid_t			i_gid;
	unsigned int	i_flags;
	dev_t			i_rdev;

	struct super_block            *i_sb;
	const struct inode_operations *i_op;
	const struct file_operations  *i_fop;
	void *i_private;
	unsigned long i_state;

	struct timespec		i_atime;
	struct timespec		i_mtime;
	struct timespec		i_ctime;

	blkcnt_t i_blocks;
};

#define iminor(i) (MINOR(i->i_rdev))
#define imajor(i) (MAJOR(i->i_rdev))

#define DT_UNKNOWN	0
#define DT_FIFO		1
#define DT_CHR		2
#define DT_DIR		4
#define DT_BLK		6
#define DT_REG		8
#define DT_LNK		10
#define DT_SOCK		12
#define DT_WHT		14

#define DNAME_INLINE_LEN 36

// directory entry
struct dentry {
	struct qstr d_name;
	char d_iname[DNAME_INLINE_LEN];
	struct inode *d_inode;
	struct super_block *d_sb;
	struct dentry *d_parent;
	struct list_head d_subdirs, d_child;
	// int d_type; // FIXME
};

struct dentry *__d_alloc(struct super_block *sb, const struct qstr *str);

struct dentry *d_alloc(struct dentry *parent, const struct qstr *str);

struct dentry *d_make_root(struct inode *root_inode);

void dput(struct dentry *dentry);

static inline void d_add(struct dentry *dentry, struct inode *inode)
{
	dentry->d_inode = inode;
}

struct inode *iget(struct super_block *sb, unsigned long ino);

// copy from Linux man page
struct linux_dirent {
	unsigned long  d_ino;	  /* Inode number */
	unsigned long  d_off;	  /* Offset to next linux_dirent */
	unsigned short d_reclen;  /* Length of this linux_dirent */
	unsigned char  d_type;    // FIXME
	char		   d_name[256];  /* Filename (null-terminated) */
	/* length is actually (d_reclen - 2 - offsetof(struct linux_dirent, d_name) */
};

/**
int filldir(struct linux_dirent *, const char * name, int namlen, loff_t offset,
		   unsigned long ino, unsigned int type);
*/

struct super_block {
	__u32 s_blocksize;
	unsigned char s_blocksize_bits;
	unsigned long s_flags;
	struct block_device *s_bdev;
	struct dentry *s_root;
	void *s_fs_info;
	void *driver_context;
	unsigned long s_magic;
	dev_t s_dev;
	unsigned char s_dirt;
};

struct super_block *sget(struct file_system_type *type, void *data);

// FIXME
int get_unused_fd();
int fd_install(int fd, struct file *fp);
struct file *fget(unsigned int fd);

//
struct fs_struct {
	struct path root, pwd;
};

void set_fs_root(const struct path *);
void set_fs_pwd(const struct path *);
void get_fs_root(struct path *);
void get_fs_pwd(struct path *);

int path_walk(const char *path, struct nameidata *nd);
int follow_up(struct path *path);

int vfs_mkdir(struct inode *dir, struct dentry *dentry, int mode);
int vfs_mknod(struct inode *dir, struct dentry *dentry, int mode, dev_t devt);

// FIXME
int dev_mknod(const char * path,int mode,dev_t devt);

struct dentry *mount_bdev(struct file_system_type *, int, const char *,
	void *, int (*fill_super)(struct super_block *, void *, int));

void d_instantiate(struct dentry *entry, struct inode * inode);

static inline void inode_dec_link_count(struct inode *inode)
{
	inode->i_nlink--;
}

// FIXME: to be moved
static inline u16 old_encode_dev(dev_t dev)
{
	return (MAJOR(dev) << 8) | MINOR(dev);
}

static inline dev_t old_decode_dev(u16 val)
{
	return MKDEV((val >> 8) & 255, val & 255);
}

int device_create(dev_t devt, const char *fmt, ...);

int register_chrdev(int major, const struct file_operations *, const char *);
