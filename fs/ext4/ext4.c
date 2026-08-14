#include <stdio.h>
#include <init.h>
#include <malloc.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <block.h>
#include <dirent.h>
#include <fs.h>
#include <fs/ext4_fs.h>
#include <fs/ext4.h>

#define MAX_MNT_LEN 256
#define SECT_SIZE   (1 << 9) // FIXME

static int ext4_lookup(struct inode *parent, struct dentry *dentry,
						struct nameidata *nd);

static const struct inode_operations ext4_reg_inode_operations = {
};

static const struct inode_operations ext4_dir_inode_operations = {
	.lookup = ext4_lookup,
};

static int ext4_open(struct file *fp, struct inode *inode);
static int ext4_close(struct file *fp);
static ssize_t ext4_read(struct file *fp, void *buff, size_t size, loff_t *off);
static ssize_t ext4_write(struct file *fp, const void *buff, size_t size, loff_t *off);

static const struct file_operations ext4_reg_file_operations = {
	.open  = ext4_open,
	.close = ext4_close,
	.read  = ext4_read,
	.write = ext4_write,
};

static int ext4_readdir(struct file *fp, void *dirent, filldir_t filldir);

static const struct file_operations ext4_dir_file_operations = {
	.readdir = ext4_readdir,
};

static struct inode *ext4_alloc_inode(struct super_block *sb)
{
	struct ext4_inode_info *eii;

	eii = zalloc(sizeof(*eii));
	if (!eii)
		return NULL;

	eii->vfs_inode.i_sb = sb; // FIXME

	return &eii->vfs_inode;
}

static ssize_t __ext4_read_buff(struct super_block *sb, size_t offset, void *buff, size_t size)
{
	ssize_t ret, start_blk;
	struct bio *bio;

	start_blk = offset / SECT_SIZE;

	bio = bio_alloc();
	if (!bio)
		return -ENOMEM;

	bio->size = (offset % SECT_SIZE + size + SECT_SIZE - 1) & ~(SECT_SIZE - 1);
	bio->data = malloc(bio->size);
	if (!bio->data) {
		ret = -ENOMEM;
		goto L1;
	}

	bio->bdev = sb->s_bdev;
	bio->sect = start_blk;
	submit_bio(READ, bio);

	memcpy(buff, bio->data + offset % SECT_SIZE, size);
	ret = size;

	free(bio->data);
L1:
	bio_free(bio);
	return ret;
}

static ssize_t __ext4_read_block(struct super_block *sb, void *buff, int blk_no)
{
	return __ext4_read_buff(sb, blk_no * sb->s_blocksize, buff, sb->s_blocksize);
}

static size_t get_start_layer(size_t start_block, size_t index_per_block)
{
	if(start_block < EXT4_IND_BLOCK)
		return 0;
	else if(start_block < EXT4_IND_BLOCK + index_per_block)
		return 1;
	else if(start_block < EXT4_IND_BLOCK + index_per_block +
			index_per_block * index_per_block)
		return 2;
	else
		return 3;
}

static size_t get_ind_block(struct super_block *sb,
			struct ext4_inode *inode,ssize_t start_block, __le32 block_indexs[], size_t len)
{
	struct ext4_sb_info *e4_sbi = sb->s_fs_info;
	size_t block_size = 1 << (e4_sbi->e4_sb.s_log_block_size + 10);
	size_t index_per_block = block_size / sizeof(__le32);
	__le32 buff[index_per_block];
	size_t i;

	start_block = start_block < 0 ? 0 : start_block;

	if(len <= 0)
		return 0;

	__ext4_read_block(sb, buff, inode->i_block[EXT4_IND_BLOCK]);

	for(i = 0; i < len && i < index_per_block - start_block; i++)
		block_indexs[i] = buff[i + start_block];

	return i;
}

static size_t get_dind_block(struct super_block *sb,
			struct ext4_inode *inode,ssize_t start_block, __le32 block_indexs[], size_t len)
{
	struct ext4_sb_info *e4_sbi = sb->s_fs_info;
	size_t block_size = 1 << (e4_sbi->e4_sb.s_log_block_size + 10);
	size_t index_per_block = block_size / sizeof(__le32);
	__le32 buff[index_per_block];
	__le32 dbuff[index_per_block];
	int i = 0, j, k;

	if(len <= 0)
		return 0;

	__ext4_read_block(sb, buff, inode->i_block[EXT4_DIND_BLOCK]);

	start_block = start_block < 0 ? 0 : start_block;

	k = start_block % index_per_block;

	for(j = start_block / index_per_block; j < index_per_block; j++) {
		__ext4_read_block(sb, dbuff, buff[j]);

		while (i < len && k < index_per_block)
			block_indexs[i++] = dbuff[k++];

		if(i >= len)
			return i;

		k = 0;
	}

	return i;
}

static size_t get_tind_block(struct super_block *sb,
			struct ext4_inode *inode, ssize_t start_block, __le32 block_indexs[], size_t len)
{
	size_t block_size = sb->s_blocksize;
	size_t index_per_block = block_size / sizeof(__le32);
	__le32 buff[index_per_block];
	__le32 dbuff[index_per_block];
	__le32 tbuff[index_per_block];
	int i = 0 /*? FIXME*/, j, k, h;

	if(len <= 0)
		return 0;

	__ext4_read_block(sb, buff, inode->i_block[EXT4_TIND_BLOCK]);

	start_block = start_block < 0 ? 0 : start_block;

	h = start_block % index_per_block;

	for(j = start_block / (index_per_block * index_per_block); j < index_per_block; j++) {
		__ext4_read_block(sb, dbuff, buff[j]);

		for(k = start_block / index_per_block; k < index_per_block; k++) {
			__ext4_read_block(sb, tbuff, dbuff[k]);

			for(; i < len && h < index_per_block; i++, h++)
				block_indexs[i] = tbuff[h];

			if(i >= len)
				return i;

			h = 0;
		}
	}

	return i;
}

static int get_block_indexs(struct super_block *sb,
			struct ext4_inode *inode,size_t start_block, __le32 block_indexs[], size_t len)
{
	struct ext4_sb_info *e4_sbi = sb->s_fs_info;
	size_t index_per_block = (1 << (e4_sbi->e4_sb.s_log_block_size + 10)) / sizeof(__le32);
	size_t start_layer;
	size_t i = 0;

	if (NULL == inode)
		return -1;

	if(len <= 0)
		return 0;

	start_layer = get_start_layer(start_block, index_per_block);

	if (0 == start_layer) {
		for(i = 0; i < len && i < EXT4_IND_BLOCK - start_block; i++)
			block_indexs[i] = inode->i_block[i + start_block];

		start_layer = -1;
	}

	if(i < len && (start_layer == 1 || start_layer == -1)) {
		i += get_ind_block(sb, inode, start_block - EXT4_IND_BLOCK,
							block_indexs + i, len - i);

		start_layer = -1;
	}

	if(i < len && (start_layer == 2 || start_layer == -1)) {
		i += get_dind_block(sb, inode, start_block - EXT4_IND_BLOCK - index_per_block
							, block_indexs + i, len - i);

		start_layer = -1;
	}

	if(i < len && (start_layer == 3 || start_layer == -1)) {
		i += get_tind_block(sb, inode, start_block - EXT4_IND_BLOCK - index_per_block
							- index_per_block * index_per_block, block_indexs + i, len - i);
	}

	return i;
}

static bool is_extent(struct inode *in)
{
	struct ext4_sb_info *sb_info;
	struct ext4_super_block *ext4_sb;
	struct ext4_inode *ext4_in;

	sb_info = in->i_sb->s_fs_info;
	ext4_sb = &sb_info->e4_sb;
	ext4_in = EXT4_I(in)->i_e4in;

	if ((ext4_sb->s_feature_incompat & EXT4_FEATURE_INCOMPAT_EXTENTS) &&
		(ext4_in->i_flags & EXT4_INODE_EXTENT))
		return true;

	return false;
}

static int __extent_idx(struct ext4_extent_header *extent_header)
{
	// FIXME
	GEN_DBG("extent_idx not support\n");

	return 0;
}

static int ext4_get_extent_blkbums(struct inode *in, size_t skip, __le32 block[], size_t nums)
{
	struct ext4_inode *ext4_in;
	struct ext4_inode_info *ext4_ini;
	struct ext4_extent_header *extent_header;
	struct ext4_extent *extent;
	struct ext4_extent_idx *extent_idx;
	int i;
	int iter;

	ext4_ini = EXT4_I(in);
	ext4_in  = ext4_ini->i_e4in;

	extent_header = (struct ext4_extent_header *)ext4_in->i_block;
	if (extent_header->eh_magic != EXT4_EXT_MAGIC) {
		GEN_DBG("check extent magic num error!\n");
		return -EINVAL;
	}

	if (extent_header->eh_depth > 0) {
		extent_idx = (void *)extent_header + sizeof(*extent_header);
		for (i = 0; i < extent_header->eh_entries; i++) {
			// ...
			__extent_idx(extent_header);
			extent_idx += sizeof(*extent_idx);
		}

		return 0;
	}

	extent = (void *)extent_header + sizeof(*extent_header);
	i = 0;
	iter = 0;

	while (i < extent_header->eh_entries) {
		size_t start = extent->ee_block;
		size_t len   = extent->ee_len;
		size_t addr  = extent->ee_start_lo; // FIXME

		while (iter < nums &&
				(iter + skip) >= start &&
				(iter + skip) < (start + len)) {
			block[iter] = addr + (iter + skip - start);
			iter++;
		}

		if (iter == nums) {
			break;
		}

		extent += sizeof(*extent);
		i++;
	}

	return 0;
}

static int ext4_get_blknums(struct inode *in, size_t skip, __le32 block[], size_t nums)
{
	if (is_extent(in)) {
		return ext4_get_extent_blkbums(in, skip, block, nums);
	}

	// FIXME
	return get_block_indexs(in->i_sb, EXT4_I(in)->i_e4in, skip, block, nums);
}

#if 0
static inline bool is_flex_bg(struct ext4_super_block *e4_sb)
{
	return (e4_sb->s_feature_incompat & EXT4_FEATURE_INCOMPAT_FLEX_BG) != 0;
}
#endif

static struct ext4_inode *ext4_get_inode(struct super_block *sb, int ino)
{
	int grp_no, ino_no, blk_no, count;
	struct ext4_sb_info *e4_sbi = sb->s_fs_info;
	struct ext4_inode *e4_in;
	struct ext4_super_block *e4_sb;
	struct ext4_group_desc *gde;
	size_t off;

	ino--;
	e4_sb = &e4_sbi->e4_sb;

	count = (1 << (e4_sb->s_log_block_size + 10)) / e4_sb->s_inode_size;

	grp_no = ino / e4_sb->s_inodes_per_group;
	gde = &e4_sbi->gdt[grp_no];


	e4_in = malloc(e4_sb->s_inode_size);
	// if (NULL == e4_in) ...

	ino_no = ino % e4_sb->s_inodes_per_group;
	blk_no = ino_no / count;
	ino_no = ino_no % count;

	off = (gde->bg_inode_table_lo + blk_no) * sb->s_blocksize + ino_no * e4_sb->s_inode_size;
	__ext4_read_buff(sb, off, e4_in, e4_sb->s_inode_size);

	return e4_in;
}

bool ext4_check_fstype(void *buff, size_t size)
{
	struct ext4_super_block *e4_sb = buff;

	if (size >= sizeof(struct ext4_super_block) && 0xEF53 == e4_sb->s_magic)
		return true;

	return false;
}

struct inode *ext4_iget(struct super_block *sb, unsigned long ino)
{
	struct inode *inode;
	struct ext4_inode *e4_in;
	struct ext4_inode_info *e4_ini;

	e4_in = ext4_get_inode(sb, ino);
	// ...

	inode = ext4_alloc_inode(sb);
	if (!inode) {
		// ...
		return NULL;
	}

	inode->i_ino  = ino;
	inode->i_mode = le16_to_cpu(e4_in->i_mode);
	inode->i_size = le32_to_cpu(e4_in->i_size_lo);

	e4_ini = EXT4_I(inode);
	e4_ini->i_e4in = e4_in; // FIXME

	if (S_ISREG(inode->i_mode)) {
		inode->i_op = &ext4_reg_inode_operations;
		inode->i_fop = &ext4_reg_file_operations;
	} else if (S_ISDIR(inode->i_mode)) {
		inode->i_op = &ext4_dir_inode_operations;
		inode->i_fop = &ext4_dir_file_operations;
	} else {
		BUG();
	}

	return inode;
}

static int ext4_fill_super(struct super_block *sb)
{
	int ret;
	int group_count;
	unsigned int bpg; // blocks per groups
	char buff[KB(1)];
	struct ext4_sb_info *e4_sbi;
	struct ext4_super_block *e4_sb;
	struct ext4_group_desc *gdt;
	struct bio *bio;
	size_t off;

	bio = bio_alloc();
	if (!bio)
		return -ENOMEM;

	bio->bdev = sb->s_bdev;
	bio->sect = 1024 / SECT_SIZE;
	bio->size = sizeof(buff);
	bio->data = buff;
	submit_bio(READ, bio);
	// TODO: check flags here
	bio_free(bio);

	if (ext4_check_fstype(buff, sizeof(struct ext4_super_block)) == false) {
		GEN_DBG("Invalid EXT4 magic number!\n"); // ((struct ext4_super_block *)buff)->s_magic);
		ret = -EINVAL;
		goto L1;
	}

	e4_sbi = zalloc(sizeof(*e4_sbi));
	if (!e4_sbi) {
		ret = -ENOMEM;
		goto L1;
	}

#if 1 // FIXME
	e4_sb = &e4_sbi->e4_sb;
	memcpy(e4_sb, buff, sizeof(*e4_sb));
#endif

	sb->s_fs_info = e4_sbi;
	sb->s_blocksize = 1024 << e4_sb->s_log_block_size;

	bpg = e4_sb->s_blocks_per_group;
	group_count = (e4_sb->s_blocks_count_lo + bpg - 1) / bpg;
	DPRINT("super block information:\n"
		"label = \"%s\", inode size = %d, block size = %d\n",
		e4_sb->s_volume_name[0] ? e4_sb->s_volume_name : "<N/A>",
		e4_sb->s_inode_size, sb->s_blocksize);

	gdt = malloc(group_count * sizeof(struct ext4_group_desc));
	if (NULL == gdt) {
		ret = -ENOMEM;
		goto L2;
	}
	e4_sbi->gdt = gdt;
	off = (e4_sb->s_first_data_block + 1) * sb->s_blocksize;

	__ext4_read_buff(sb, off, gdt, group_count * sizeof(struct ext4_group_desc));

	DPRINT("group descrition:\n"
		"block groups = %d, free blocks = %d, free inodes = %d\n",
		group_count, gdt->bg_free_blocks_count_lo, gdt->bg_free_inodes_count_lo);

	return 0;

L2:
	free(e4_sbi);
L1:
	return ret;
}

static int ext4_read_super(struct super_block *sb, void *data, int flags)
{
	int ret;
	struct inode *in;
	struct dentry *root;

	ret = ext4_fill_super(sb);
	if (ret < 0) {
		// ...
		return ret;
	}

	GEN_DBG("\n");

	in = ext4_iget(sb, 2);
	if (!in) {
		// ...
		return -EINVAL;
	}

	root = d_make_root(in);
	if (!root)
		return EINVAL;

	sb->s_root = root;

	return 0;
}

static struct dentry *ext4_mount(struct file_system_type *fs,
			    int flags, const char *dev_name, void *data)
{
	return mount_bdev(fs, flags, dev_name, data, ext4_read_super);
}

// FIXME
static void ext4_kill_sb(struct super_block *sb)
{
}

static unsigned long ext4_inode_by_name(struct inode *inode, struct qstr *unit)
{
	struct super_block *sb = inode->i_sb;
	struct ext4_sb_info *e4_sbi;
	struct ext4_inode_info *e4_ini = EXT4_I(inode);
	struct ext4_dir_entry_2 *e4_de;
	struct ext4_inode *parent = e4_ini->i_e4in;
	char buff[inode->i_sb->s_blocksize];
	size_t len = 0;
	int blocks, i;
	size_t block_size;

	e4_sbi = sb->s_fs_info;
	block_size = 1024 << e4_sbi->e4_sb.s_log_block_size;

	blocks = (parent->i_size_lo + block_size - 1) / block_size;
	__le32 block_indexs[blocks];

	ext4_get_blknums(inode, 0, block_indexs, blocks);

	for (i = 0; i < blocks; i++) {
		__ext4_read_block(sb, buff, block_indexs[i]);

		e4_de = (struct ext4_dir_entry_2 *)buff;

		while (e4_de->rec_len > 0 && len < parent->i_size_lo && len < (i + 1) * block_size) {
			e4_de->name[e4_de->name_len] = '\0';

			DPRINT("%s: inode = %d, e4_de size = %d, name size = %d, block = %d\n",
				e4_de->name, e4_de->inode, e4_de->rec_len, e4_de->name_len, i);

			if (unit->len == e4_de->name_len && \
				!strncmp(e4_de->name, unit->name, e4_de->name_len))
				return  e4_de->inode;

			e4_de = (struct ext4_dir_entry_2 *)((char *)e4_de + e4_de->rec_len);
			len += e4_de->rec_len;
		}
	}

	GEN_DBG("\"%s\" not found!\n", unit->name);

	return 0;
}

static int ext4_open(struct file *fp, struct inode *inode)
{
	return 0;
}

static int ext4_close(struct file *fp)
{
	return 0;
}

static ssize_t ext4_read(struct file *fp, void *buff, size_t size, loff_t *off)
{
	struct inode *in;
	struct dentry *de;
	size_t cur_blk;
	size_t offset;
	size_t blks;
	__le32 blk_num[(size - 1) / fp->f_dentry->d_sb->s_blocksize + 2];
	size_t blk_size = fp->f_dentry->d_sb->s_blocksize;
	int ret;
	int i = 0;
	size_t count = 0;

	de = fp->f_dentry;
	in = de->d_inode;

	if (fp->f_pos >= in->i_size)
		return 0;

	cur_blk = fp->f_pos / in->i_sb->s_blocksize;
	offset  = fp->f_pos % in->i_sb->s_blocksize;

	if (offset != 0) {
		blks = (size - (blk_size - offset) - 1) / blk_size + 2;
	} else {
		blks = (size - 1) / blk_size + 1;
	}

	ret = ext4_get_blknums(in, cur_blk, blk_num, blks);
	if (ret < 0) {
		GEN_DBG("Fail to get blk num 0x%x\n", cur_blk);
		return ret;
	}

	if (offset != 0) {
		__ext4_read_buff(in->i_sb, blk_num[0] * blk_size, buff, blk_size - (offset + 1));
		count += blk_size - (offset + 1);
		i = 1;
	}

	for (; i < blks - 1; i++) {
		__ext4_read_block(in->i_sb, buff + i * blk_size, blk_num[i]);
		count += blk_size;
	}

	__ext4_read_buff(in->i_sb, blk_num[i] * blk_size, buff + count, size - count);

	fp->f_pos += size;

	return size;
}

static ssize_t ext4_write(struct file *fp, const void *buff, size_t size, loff_t *off)
{
	return 0;
}

static int ext4_lookup(struct inode *parent, struct dentry *dentry, struct nameidata *nd)
{
	unsigned long ino;
	struct inode *inode;

	ino = ext4_inode_by_name(parent, &dentry->d_name);
	if (!ino) {
		// ...
		return -ENOENT;
	}

	inode = ext4_iget(parent->i_sb, ino);
	if (!inode) {
		// ...
		return -EIO;
	}

	// dentry->d_inode = inode;
	d_add(dentry, inode);

	return 0;
}


#if 0
static int ext4_inode_read_block(struct inode *in, int blk_no, char *blk_buf)
{
	struct ext4_inode_info *e4_info = EXT4_I(in);
	int blk_index[in->i_size / in->i_sb->s_blocksize];

	get_block_indexs(in->i_sb, e4_info->i_e4in, 0, (__le32 *)blk_index, ARRAY_ELEM_NUM(blk_index));

	__ext4_read_block(in->i_sb, blk_buf, blk_index[blk_no]);

	return 0;
}
#endif

static bool is_hbtree_dir(struct inode *in)
{
	struct ext4_sb_info *fs_info;
	struct ext4_super_block *ext4_sb;
	struct ext4_inode *ext4_in;

	fs_info = in->i_sb->s_fs_info;
	ext4_sb = &fs_info->e4_sb;
	ext4_in = EXT4_I(in)->i_e4in;

	if ((ext4_sb->s_feature_compat & EXT4_FEATURE_COMPAT_DIR_INDEX) &&
		(ext4_in->i_flags & EXT4_INDEX_FL))
		return true;

	return false;
}

static int ext4_dx_readdir(struct file *fp, void *dirent, filldir_t filldir)
{
	// FIXME

	GEN_DBG("Not support dx read!\n");

	return 0;
}

static int ext4_readdir(struct file *fp, void *dirent, filldir_t filldir)
{
	struct inode *in;
	struct dentry *de;
	size_t cur_blk;
	size_t offset;
	__le32 blk_num;
	int ret;
	char blk_buff[fp->f_dentry->d_sb->s_blocksize];
	struct ext4_dir_entry_2 *ext4_de;

	de = fp->f_dentry;
	in = de->d_inode;

	if (is_hbtree_dir(in)) {
		return ext4_dx_readdir(fp, dirent, filldir);
	}

	if (fp->f_pos >= in->i_size)
		return 0;

	cur_blk = fp->f_pos / in->i_sb->s_blocksize;
	offset  = fp->f_pos % in->i_sb->s_blocksize;

	ret = ext4_get_blknums(in, cur_blk, &blk_num, 1);
	if (ret < 0) {
		GEN_DBG("Fail to get blk num 0x%x\n", cur_blk);
		return ret;
	}

	__ext4_read_block(in->i_sb, blk_buff, blk_num);

	ext4_de = (struct ext4_dir_entry_2 *)(blk_buff + offset);
	filldir(dirent, ext4_de->name, ext4_de->name_len, ext4_de->rec_len, ext4_de->inode, ext4_de->file_type);

	fp->f_pos += ext4_de->rec_len;

	return ext4_de->rec_len;
}

extern int ck_ext4_feature(uint32_t fc,uint32_t frc,uint32_t fi);
static int ext4_check_fs_type(const char *bdev_name)
{
	struct ext4_super_block *e4_sb;
	struct bio *bio;
	char buff[EXT4_SUPER_BLK_SIZE];
	struct block_device *bdev;
	uint32_t fc, frc, fi;
	int ret;

	bdev = bdev_get(bdev_name);
	if (NULL == bdev) {
		DPRINT("bdev %s not found!\n", bdev_name);
		return -ENODEV;
	}

	bio = bio_alloc();
	if (!bio)
		return -ENOMEM;

	bio->bdev = bdev;
	bio->sect = 1024 / SECT_SIZE;
	bio->size = sizeof(buff);
	bio->data = buff;
	submit_bio(READ, bio);
	// TODO: check flags here
	bio_free(bio);

	e4_sb = (struct ext4_super_block *)buff;

	if (0xEF53 != e4_sb->s_magic) {
		DPRINT("%s is not \"ext4\" fs!\n", bdev_name);
		return -EINVAL;
	}

	fc = e4_sb->s_feature_compat;
	fi = e4_sb->s_feature_incompat;
	frc = e4_sb->s_feature_ro_compat;


	ret = ck_ext4_feature(fc, frc, fi);

#ifdef CONFIG_DEBUG
	extern void ext_sb_list(struct ext4_super_block * esb);
	if (ret == 0) {
		ext_sb_list(e4_sb);
	}
#endif

	return ret;
}


static struct file_system_type ext4_fs_type = {
	.name    = "ext4",
	.mount   = ext4_mount,
	.kill_sb = ext4_kill_sb,
	.check_fs_type = ext4_check_fs_type,
};

static int __init ext4_init(void)
{
	return register_filesystem(&ext4_fs_type);
}

module_init(ext4_init);
