#include <malloc.h>
#include <errno.h>
#include <string.h>
#include <types.h>
#include <fs.h>
#include <fs/fat.h>
#include <drive.h>

static ssize_t fat_read_block(struct fat_fs *fs, void *buff, int blk_no, size_t off, size_t size)
{
	struct block_device *bdev = fs->bdev;
	struct disk_drive *drive = container_of(bdev, struct disk_drive, bdev);

	size_t buf_len = (off + size + drive->sect_size - 1) & ~(drive->sect_size - 1);
	char blk_buf[buf_len];
	int start_blk, cur_blk;

	start_blk = blk_no * fs->clus_size / drive->sect_size;

	for (cur_blk = 0; cur_blk < buf_len / drive->sect_size; cur_blk++)
		drive->get_block(drive, (start_blk + cur_blk) * drive->sect_size, blk_buf + cur_blk * drive->sect_size);

	memcpy(buff, blk_buf + off, size);

	return size;
}

static __u32 fat_get_fat_table(struct fat_fs *fs, __u32 fat_num)
{
	int ret;
	static __u32 old_fat = ~0;
	static __u32 fat_catch[2048];

	if (old_fat == ~0 || fat_num < old_fat || fat_num > (fs->clus_size / sizeof(fat_num) + old_fat - 1)) {
		ret = fat_read_block(fs, fat_catch, fs->dbr.resv_size / fs->dbr.sec_per_clus + fat_num * sizeof(fat_num) / (fs->clus_size),
			0, fs->clus_size);

		if (ret < 0)
			return ret;

		old_fat = (fat_num / fs->clus_size / sizeof(fat_num)) * (fs->clus_size / sizeof(fat_num));
	}

	return fat_catch[fat_num % (fs->clus_size / sizeof(fat_num))];
}

static struct dentry *fat_mount(struct file_system_type *fs_type, unsigned long flags, struct block_device *bdev)
{
	int ret;
	__u16 blk_size;
	__u32 clus_size;
	__u32 data_off;
	__u32 root;
	struct dentry *root_dir;
	struct inode *root_ino;
	struct fat_fs *fat_fs;
	struct fat_boot_sector *dbr;
	struct disk_drive *drive = container_of(bdev, struct disk_drive, bdev);

	fat_fs = zalloc(sizeof(*fat_fs));
	if (fat_fs == NULL) {
		DPRINT("%s(): no mem\n", __func__);
		return NULL;
	}

	dbr = &fat_fs->dbr;

	ret = drive->get_block(drive, 0, dbr);
	if (ret < 0) {
		DPRINT("%s(): read dbr err\n", __func__);
		return NULL;
	}

	blk_size = dbr->sector_size[1] << 8 | dbr->sector_size[0];

	if (blk_size < 512 || blk_size > 4096 || (blk_size & (blk_size - 1))) {
		DPRINT("%s %s() line %d\n", __FILE__, __func__, __LINE__);
		return NULL;
	}

	// TODO: check if bk_size != drive->sect_size ...

	data_off = (dbr->resv_size + dbr->fats * dbr->fat32_length) / dbr->sec_per_clus - 2;

	clus_size = blk_size * dbr->sec_per_clus; // fat block size

	root = dbr->root_cluster;

	fat_fs->root = root;
	fat_fs->bdev = bdev;
	fat_fs->data = data_off;
	fat_fs->clus_size = clus_size;

	// FIXME
	drive->sect_size = blk_size;

	// FIXME
	root_ino = zalloc(sizeof(*root_ino));
	if (!root_ino)
		return NULL;

	root_ino->i_fs = fat_fs;
	root_ino->i_ext = (void *)root;

	root_dir = zalloc(sizeof(*root_dir));
	if (!root_dir)
		return NULL;

	root_dir->inode = root_ino;

	/////

	return root_dir;
}

#if 0
static int fat_umount(struct fat_fs *fs, const char *path, const char *type, unsigned long flags)
{
	// free
	return 0;
}
#endif

static int strxcpy(char *dst, const char *src, __u32 n)
{
	int i, j;

	for (i = 0, j = 0; i < n; i += 2) {
		if (*(src + i) != '\0') {
			*(dst + j) = *(src + i);
			j++;
		} else if (*(src + i) == '\0' && *(src + i + 1) == '\0') {
			*(dst + j) = '\0';
			j++;
		}
	}

	return j;
}

static int fat_find_next_file(struct fat_fs *fs,
			__u32 *block_num, char *name, struct fat_dentry *dir)
{
	char buf[fs->clus_size];
	static struct fat_dentry *dir_pos;
	static __u32 old_block_num;

	name[0] = '\0';

	do {
		if (*block_num != old_block_num
			|| dir_pos - (struct fat_dentry *)buf == fs->clus_size / sizeof(*dir_pos)) // find next cluster
		{
			old_block_num = *block_num;

			if (*block_num > 0x0ffffff8 || *block_num == 0)
				return -1;

			if (dir_pos - (struct fat_dentry *)buf == fs->clus_size / sizeof(*dir_pos)) {
				*block_num = fat_get_fat_table(fs, *block_num);
				if (*block_num < 0)
					goto L1;
				old_block_num = *block_num;
			}

			fat_read_block(fs, buf, fs->data + *block_num, 0, fs->clus_size);

			dir_pos = (struct fat_dentry *)buf;
		}

		DPRINT("name = %s, attribute = %d\n", dir_pos->name, dir_pos->attr);

		switch ((__u8)dir_pos->name[0]) {
		case 0xe5:
			break;

		case 0x0:
			goto L1;

		default:
			switch (dir_pos->attr) {
			case 0x10:
			case 0x20:
				if (!name[0]) {
					int i, j;

					for (i = 0; i < 8 && dir_pos->name[i] != ' '; i++)
							name[i] = dir_pos->name[i];

					name[i++] = '.'; // FIXME

					for (j = 8; j < 11 && dir_pos->name[j] != ' '; j++, i++)
						name[i]	= dir_pos->name[j];

					*(__u16 *)(name + i) = 0;
				}

				memcpy(dir, dir_pos, sizeof(*dir_pos));
				dir_pos++;
				return 1;

			case 0x0f:
				strxcpy(name + ((dir_pos->name[0] & 0x0f) - 1) * 13, (char *)dir_pos + 1, 10);
				strxcpy(name + ((dir_pos->name[0] & 0x0f) - 1) * 13 + 5, (char *)dir_pos + 14, 12);
				strxcpy(name + ((dir_pos->name[0] & 0x0f) - 1) * 13 + 11, (char *)dir_pos + 28, 4);
				break;

			default:
				break;
			}
		}

		dir_pos++;
	}while(dir_pos->name[0]);

L1:
	return -1;
}

static struct dentry *fat_lookup(struct inode *parent, const char *name)
{
	int ret;
	char sname[FILE_NAME_SIZE];
	__u32 blk_num;
	struct dentry *de;
	struct inode *ino;
	struct fat_dentry *fat_de;
	struct fat_fs *fat_fs = parent->i_fs;

	fat_de = (struct fat_dentry *)malloc(sizeof(*fat_de));
	if (fat_de == NULL) {
		DPRINT("%s(): malloc err\n", __func__);
		return NULL;
	}

	blk_num = (__u32)parent->i_ext;

	while (1) {
		// FIXME
		ret = fat_find_next_file(fat_fs, &blk_num, sname, fat_de);
		printf("%s(): ret = %d, sname = %s\n",
			__func__, ret, sname);
		if (ret < 0) {
			DPRINT("can't find file\n");
			goto err;
		}

		if (strcmp(sname, name) == 0) {
			name = strchr(name, '/');
			if (name != NULL) {	// last file
				if (fat_de->attr == 0x10) {
					blk_num = fat_de->clus_hi << 16 | fat_de->clus_lo;
					continue;
				} else
					goto err;
			}

			ino = zalloc(sizeof(*ino));
			if (!ino)
				return NULL;

			ino->mode = ~0;
			ino->i_fs = parent->i_fs;
			ino->i_ext = (void *)blk_num;

			de = zalloc(sizeof(*de));
			if (!de)
				return NULL;

			de->d_ext = fat_de;
			de->inode = ino;

			return de;
		}
	}

err:
	free(fat_de);
	return NULL;
}

#define MAX_FILE_NAME_SIZE 256

// FIXME
static int fat_open(struct file *fp, struct inode *inode)
{
	return 0;
}

static int fat_close(struct file *fp)
{
	return 0;
}

static ssize_t fat_read(struct file *fp, void *buff, size_t size, loff_t *off)
{
	__u32 clus_num, clus_size;
	size_t pos;
	size_t count = 0;
	size_t tmp_size;
	struct fat_fs *fs = fp->de->inode->i_fs;
	struct fat_dentry *de = (struct fat_dentry *)fp->de;

	clus_size = fs->clus_size;
	clus_num = de->clus_hi << 16 | de->clus_lo;
	pos = fp->pos;

	if (size + pos > de->size)
		size = de->size - pos;

	while (pos >= clus_size) {
		clus_num = fat_get_fat_table(fs, clus_num);

#if 0
		if (clus_num < 0)
			return clus_num;
#endif

		pos -= clus_size;
	}

	while (size > count) {
		if (size + pos - count > clus_size)
			tmp_size = clus_size - pos;
		else
			tmp_size = size - count;

		tmp_size = fat_read_block(fs, buff + count, fs->data + clus_num, pos, tmp_size);
		if (tmp_size < 0)
			break;
		count += tmp_size;

		clus_num = fat_get_fat_table(fs, clus_num);
		if (clus_num < 0)
			break;

		pos = 0;
	}

	return count;
}

static ssize_t fat_write(struct file *fp, const void *buff, size_t size, loff_t *off)
{
	return 0;
}

static const struct file_operations fat_fops = {
	.open  = fat_open,
	.close = fat_close,
	.read  = fat_read,
	.write = fat_write,
};

static struct file_system_type fat_fs_type = {
	.name   = "vfat",
	.fops   = &fat_fops,
	.mount  = fat_mount,
	.lookup = fat_lookup,
};

static int __init fat_init(void)
{
	return register_filesystem(&fat_fs_type);
}

module_init(fat_init);
