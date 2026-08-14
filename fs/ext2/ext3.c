#include <stdio.h>
#include <init.h>
#include <malloc.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <block.h>
#include <dirent.h>
#include <fs.h>
#include <fs/ext2_fs.h>
#include <fs/ext2.h>

struct dentry *ext2_mount(struct file_system_type *fs,
			    int flags, const char *dev_name, void *data);
// FIXME
void ext2_kill_sb(struct super_block *sb);

int ext2_check_fs_type(const char *bdev_name);

static struct file_system_type ext3_fs_type = {
	.name    = "ext3",
	.mount   = ext2_mount,
	.kill_sb = ext2_kill_sb,
	.check_fs_type = ext2_check_fs_type,
};

static int __init ext2_init(void)
{
	return register_filesystem(&ext3_fs_type);
}

module_init(ext2_init);
