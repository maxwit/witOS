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

#define MAX_MNT_LEN 256
#define SECT_SIZE   (1 << 9) // FIXME

static int ext2_lookup(struct inode *parent, struct dentry *dentry,
						struct nameidata *nd);

static const struct inode_operations ext2_reg_inode_operations = {
};

static const struct inode_operations ext2_dir_inode_operations = {
	.lookup = ext2_lookup,
};

static int ext2_open(struct file *fp, struct inode *inode);
static int ext2_close(struct file *fp);
static ssize_t ext2_read(struct file *fp, void *buff, size_t size, loff_t *off);
static ssize_t ext2_write(struct file *fp, const void *buff, size_t size, loff_t *off);

static const struct file_operations ext2_reg_file_operations = {
	.open  = ext2_open,
	.close = ext2_close,
	.read  = ext2_read,
	.write = ext2_write,
};

static int ext2_readdir(struct file *fp, void *dirent, filldir_t filldir);

static const struct file_operations ext2_dir_file_operations = {
	.readdir = ext2_readdir,
};

static struct inode *ext2_alloc_inode(struct super_block *sb)
{
	struct ext2_inode_info *eii;

	eii = zalloc(sizeof(*eii));
	if (!eii)
		return NULL;

	eii->vfs_inode.i_sb = sb; // FIXME

	return &eii->vfs_inode;
}

static ssize_t ext2_read_block(struct super_block *sb, void *buff, int blk_no, size_t off, size_t size)
{
	ssize_t ret, start_blk;
	struct bio *bio;
	struct ext2_sb_info *e2_sbi = sb->s_fs_info;
	struct ext2_super_block *e2_sb;

	e2_sb = &e2_sbi->e2_sb;
	start_blk = blk_no << (e2_sb->s_log_block_size + 1);

	bio = bio_alloc();
	if (!bio)
		return -ENOMEM;

	bio->size = (off + size + SECT_SIZE - 1) & ~(SECT_SIZE - 1);
	bio->data = malloc(bio->size);
	if (!bio->data) {
		ret = -ENOMEM;
		goto L1;
	}

	bio->bdev = sb->s_bdev;
	bio->sect = start_blk;
	submit_bio(READ, bio);

	memcpy(buff, bio->data + off, size);
	ret = size;

	free(bio->data);
L1:
	bio_free(bio);
	return ret;
}

static size_t get_start_layer(size_t start_block, size_t index_per_block)
{
	if(start_block < EXT2_IND_BLOCK)
		return 0;
	else if(start_block < EXT2_IND_BLOCK + index_per_block)
		return 1;
	else if(start_block < EXT2_IND_BLOCK + index_per_block +
			index_per_block * index_per_block)
		return 2;
	else
		return 3;
}

static size_t get_ind_block(struct super_block *sb,
			struct ext2_inode *inode,ssize_t start_block, __le32 block_indexs[], size_t len)
{
	struct ext2_sb_info *e2_sbi = sb->s_fs_info;
	size_t block_size = 1 << (e2_sbi->e2_sb.s_log_block_size + 10);
	size_t index_per_block = block_size / sizeof(__le32);
	__le32 buff[index_per_block];
	size_t i;

	start_block = start_block < 0 ? 0 : start_block;

	if(len <= 0)
		return 0;

	ext2_read_block(sb, buff, inode->i_block[EXT2_IND_BLOCK], 0, block_size);

	for(i = 0; i < len && i < index_per_block - start_block; i++)
		block_indexs[i] = buff[i + start_block];

	return i;
}

static size_t get_dind_block(struct super_block *sb,
			struct ext2_inode *inode,ssize_t start_block, __le32 block_indexs[], size_t len)
{
	struct ext2_sb_info *e2_sbi = sb->s_fs_info;
	size_t block_size = 1 << (e2_sbi->e2_sb.s_log_block_size + 10);
	size_t index_per_block = block_size / sizeof(__le32);
	__le32 buff[index_per_block];
	__le32 dbuff[index_per_block];
	int i = 0, j, k;

	if(len <= 0)
		return 0;

	ext2_read_block(sb, buff, inode->i_block[EXT2_DIND_BLOCK], 0, block_size);

	start_block = start_block < 0 ? 0 : start_block;

	k = start_block % index_per_block;

	for(j = start_block / index_per_block; j < index_per_block; j++) {
		ext2_read_block(sb, dbuff, buff[j], 0, block_size);

		while (i < len && k < index_per_block)
			block_indexs[i++] = dbuff[k++];

		if(i >= len)
			return i;

		k = 0;
	}

	return i;
}

static size_t get_tind_block(struct super_block *sb,
			struct ext2_inode *inode, ssize_t start_block, __le32 block_indexs[], size_t len)
{
	size_t block_size = sb->s_blocksize;
	size_t index_per_block = block_size / sizeof(__le32);
	__le32 buff[index_per_block];
	__le32 dbuff[index_per_block];
	__le32 tbuff[index_per_block];
	int i = 0 /*? FIXME*/, j, k, h;

	if(len <= 0)
		return 0;

	ext2_read_block(sb, buff, inode->i_block[EXT2_TIND_BLOCK], 0, block_size);

	start_block = start_block < 0 ? 0 : start_block;

	h = start_block % index_per_block;

	for(j = start_block / (index_per_block * index_per_block); j < index_per_block; j++) {
		ext2_read_block(sb, dbuff, buff[j], 0, block_size);

		for(k = start_block / index_per_block; k < index_per_block; k++) {
			ext2_read_block(sb, tbuff, dbuff[k], 0, block_size);

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
			struct ext2_inode *inode,size_t start_block, __le32 block_indexs[], size_t len)
{
	struct ext2_sb_info *e2_sbi = sb->s_fs_info;
	size_t index_per_block = (1 << (e2_sbi->e2_sb.s_log_block_size + 10)) / sizeof(__le32);
	size_t start_layer;
	size_t i = 0;

	if (NULL == inode)
		return -1;

	if(len <= 0)
		return 0;

	start_layer = get_start_layer(start_block, index_per_block);

	if (0 == start_layer) {
		for(i = 0; i < len && i < EXT2_IND_BLOCK - start_block; i++)
			block_indexs[i] = inode->i_block[i + start_block];

		start_layer = -1;
	}

	if(i < len && (start_layer == 1 || start_layer == -1)) {
		i += get_ind_block(sb, inode, start_block - EXT2_IND_BLOCK,
							block_indexs + i, len - i);

		start_layer = -1;
	}

	if(i < len && (start_layer == 2 || start_layer == -1)) {
		i += get_dind_block(sb, inode, start_block - EXT2_IND_BLOCK - index_per_block
							, block_indexs + i, len - i);

		start_layer = -1;
	}

	if(i < len && (start_layer == 3 || start_layer == -1)) {
		i += get_tind_block(sb, inode, start_block - EXT2_IND_BLOCK - index_per_block
							- index_per_block * index_per_block, block_indexs + i, len - i);
	}

	return i;
}

static struct ext2_inode *ext2_get_inode(struct super_block *sb, int ino)
{
	int grp_no, ino_no, blk_no, count;
	struct ext2_sb_info *e2_sbi = sb->s_fs_info;
	struct ext2_inode *e2_in;
	struct ext2_super_block *e2_sb;
	struct ext2_group_desc *gde;

	ino--;
	e2_sb = &e2_sbi->e2_sb;

	count = (1 << (e2_sb->s_log_block_size + 10)) / e2_sb->s_inode_size;

	grp_no = ino / e2_sb->s_inodes_per_group;
	gde = &e2_sbi->gdt[grp_no];

	ino_no = ino % e2_sb->s_inodes_per_group;
	blk_no = ino_no / count;
	ino_no = ino_no % count;

	e2_in = malloc(e2_sb->s_inode_size);
	// if

	ext2_read_block(sb, e2_in, gde->bg_inode_table + blk_no, ino_no * e2_sb->s_inode_size, e2_sb->s_inode_size);

	return e2_in;
}

bool ext2_check_fstype(void *buff, size_t size)
{
	struct ext2_super_block *e2_sb = buff;

	if (size >= sizeof(struct ext2_super_block) && 0xEF53 == e2_sb->s_magic)
		return true;

	return false;
}

struct inode *ext2_iget(struct super_block *sb, unsigned long ino)
{
	struct inode *inode;
	struct ext2_inode *e2_in;
	struct ext2_inode_info *e2_ini;

	e2_in = ext2_get_inode(sb, ino);
	// ...

	inode = ext2_alloc_inode(sb);
	if (!inode) {
		// ...
		return NULL;
	}

	inode->i_ino  = ino;
	inode->i_mode = le16_to_cpu(e2_in->i_mode);
	inode->i_size = le32_to_cpu(e2_in->i_size);

	e2_ini = EXT2_I(inode);
	e2_ini->i_e2in = e2_in; // FIXME

	if (S_ISREG(inode->i_mode)) {
		inode->i_op = &ext2_reg_inode_operations;
		inode->i_fop = &ext2_reg_file_operations;
	} else if (S_ISDIR(inode->i_mode)) {
		inode->i_op = &ext2_dir_inode_operations;
		inode->i_fop = &ext2_dir_file_operations;
	} else {
		BUG();
	}

	return inode;
}

static int ext2_fill_super(struct super_block *sb)
{
	int ret;
	int group_count;
	unsigned int bpg; // blocks per groups
	char buff[KB(1)];
	struct ext2_sb_info *e2_sbi;
	struct ext2_super_block *e2_sb;
	struct ext2_group_desc *gdt;
	struct bio *bio;

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

	if (ext2_check_fstype(buff, sizeof(struct ext2_super_block)) == false) {
		GEN_DBG("Invalid EXT2 magic number!\n"); // ((struct ext2_super_block *)buff)->s_magic);
		ret = -EINVAL;
		goto L1;
	}

	e2_sbi = zalloc(sizeof(*e2_sbi));
	if (!e2_sbi) {
		ret = -ENOMEM;
		goto L1;
	}

#if 1 // FIXME
	e2_sb = &e2_sbi->e2_sb;
	memcpy(e2_sb, buff, sizeof(*e2_sb));
#endif

	sb->s_fs_info = e2_sbi;
	sb->s_blocksize = 1024 << e2_sb->s_log_block_size;

	bpg = e2_sb->s_blocks_per_group;
	group_count = (e2_sb->s_blocks_count + bpg - 1) / bpg;
	DPRINT("super block information:\n"
		"label = \"%s\", inode size = %d, block size = %d\n",
		e2_sb->s_volume_name[0] ? e2_sb->s_volume_name : "<N/A>",
		e2_sb->s_inode_size, sb->s_blocksize);

	gdt = malloc(group_count * sizeof(struct ext2_group_desc));
	if (NULL == gdt) {
		ret = -ENOMEM;
		goto L2;
	}
	e2_sbi->gdt = gdt;

	ext2_read_block(sb, gdt, e2_sb->s_first_data_block + 1, 0,
		group_count * sizeof(struct ext2_group_desc));

	DPRINT("group descrition:\n"
		"block groups = %d, free blocks = %d, free inodes = %d\n",
		group_count, gdt->bg_free_blocks_count, gdt->bg_free_inodes_count);


	return 0;

L2:
	free(e2_sbi);
L1:
	return ret;
}

static int ext2_read_super(struct super_block *sb, void *data, int flags)
{
	int ret;
	struct inode *in;
	struct dentry *root;

	ret = ext2_fill_super(sb);
	if (ret < 0) {
		// ...
		return ret;
	}

	in = ext2_iget(sb, 2);
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

struct dentry *ext2_mount(struct file_system_type *fs,
			    int flags, const char *dev_name, void *data)
{
	return mount_bdev(fs, flags, dev_name, data, ext2_read_super);
}

// FIXME
void ext2_kill_sb(struct super_block *sb)
{
}

static unsigned long ext2_inode_by_name(struct inode *inode, struct qstr *unit)
{
	struct super_block *sb = inode->i_sb;
	struct ext2_sb_info *e2_sbi;
	struct ext2_inode_info *e2_ini = EXT2_I(inode);
	struct ext2_dir_entry_2 *e2_de;
	struct ext2_inode *parent = e2_ini->i_e2in;
	char buff[parent->i_size];
	size_t len = 0;
	int blocks, i;
	size_t block_size;

	e2_sbi = sb->s_fs_info;
	block_size = 1024 << e2_sbi->e2_sb.s_log_block_size;

	blocks = (parent->i_size + block_size - 1) / block_size;
	__le32 block_indexs[blocks];

	get_block_indexs(sb, parent, 0, block_indexs, blocks);

	for (i = 0; i < blocks; i++) {
		ext2_read_block(sb, buff, block_indexs[i], 0, block_size);

		e2_de = (struct ext2_dir_entry_2 *)buff;

		while (e2_de->rec_len > 0 && len < parent->i_size && len < (i + 1) * block_size) {
			e2_de->name[e2_de->name_len] = '\0';

			DPRINT("%s: inode = %d, e2_de size = %d, name size = %d, block = %d\n",
				e2_de->name, e2_de->inode, e2_de->rec_len, e2_de->name_len, i);

			if (unit->len == e2_de->name_len && \
				!strncmp(e2_de->name, unit->name, e2_de->name_len))
				return  e2_de->inode;

			e2_de = (struct ext2_dir_entry_2 *)((char *)e2_de + e2_de->rec_len);
			len += e2_de->rec_len;
		}
	}

	GEN_DBG("\"%s\" not found!\n", unit->name);

	return 0;
}

static int ext2_open(struct file *fp, struct inode *inode)
{
	return 0;
}

static int ext2_close(struct file *fp)
{
	return 0;
}

static ssize_t ext2_read(struct file *fp, void *buff, size_t size, loff_t *off)
{
	ssize_t i, len;
	size_t blocks;
	size_t offset, real_size, block_size;
	struct dentry *de = fp->f_dentry;
	struct inode *in;
	struct super_block *sb;
	struct ext2_inode *e2_in;
	struct ext2_sb_info *e2_sbi;

	sb     = de->d_sb;
	in     = de->d_inode;
	e2_sbi = sb->s_fs_info;

	e2_in = ext2_get_inode(sb, in->i_ino);
	if (!e2_in)
		return -ENOENT;

	// FIXME!!!
	if (fp->f_pos == e2_in->i_size)
		return 0;

	block_size = 1 << (e2_sbi->e2_sb.s_log_block_size + 10);
	char tmp_buff[block_size];

	real_size = min(size, e2_in->i_size - fp->f_pos);

	blocks = (real_size + (block_size - 1)) / block_size;
	__le32 block_indexs[blocks];

	get_block_indexs(sb, e2_in, fp->f_pos / block_size, block_indexs, blocks);

	offset = fp->f_pos % block_size;
	len = block_size - offset;

	for(i = 0; i < blocks; i++) {
		if(i == blocks - 1)
			len = real_size % block_size == 0 ? block_size : (real_size % block_size);

		ext2_read_block(sb, tmp_buff, block_indexs[i], offset, len);

		memcpy(buff, tmp_buff, len);
		buff += len;
		fp->f_pos += len;
		offset = 0;
		len = block_size;
	}

	return real_size;
}

static ssize_t ext2_write(struct file *fp, const void *buff, size_t size, loff_t *off)
{
	return 0;
}

static int ext2_lookup(struct inode *parent, struct dentry *dentry, struct nameidata *nd)
{
	unsigned long ino;
	struct inode *inode;

	ino = ext2_inode_by_name(parent, &dentry->d_name);
	if (!ino) {
		// ...
		return -ENOENT;
	}

	inode = ext2_iget(parent->i_sb, ino);
	if (!inode) {
		// ...
		return -EIO; // FIXME!!!
	}

	// dentry->d_inode = inode;
	d_add(dentry, inode);

	return 0;
}

static int ext2_inode_read_block(struct inode *in, int blk_no, char *blk_buf)
{
	struct ext2_inode_info *e2_info = EXT2_I(in);
	int blk_index[in->i_size / in->i_sb->s_blocksize];

	get_block_indexs(in->i_sb, e2_info->i_e2in, 0, (__le32 *)blk_index, ARRAY_ELEM_NUM(blk_index));

	ext2_read_block(in->i_sb, blk_buf, blk_index[blk_no], 0, in->i_sb->s_blocksize);

	return 0;
}

static int ext2_readdir(struct file *fp, void *dirent, filldir_t filldir)
{
	int blk_num;
	int blk_off;
	char buff[fp->f_dentry->d_inode->i_sb->s_blocksize];
	struct inode *in;
	struct ext2_dir_entry_2 *e2_de;

	in = fp->f_dentry->d_inode;

	if (fp->f_pos == in->i_size)
		return 0;

	blk_num = fp->f_pos / in->i_sb->s_blocksize;
	blk_off = fp->f_pos % in->i_sb->s_blocksize;

	ext2_inode_read_block(in, blk_num, buff);

	e2_de = (struct ext2_dir_entry_2 *)(buff + blk_off);

	if (e2_de->rec_len + fp->f_pos > in->i_size)
		return -ENODATA;

	filldir(dirent, e2_de->name, e2_de->name_len, fp->f_pos /*FIXME*/,
		in->i_ino, e2_de->file_type);

	fp->f_pos += e2_de->rec_len;

	return e2_de->rec_len;
}

#if 0
static int set_dirent_by_blk(void *dirent, void *blk_buf, size_t blk_size)
{
	struct ext2_dir_entry_2 *de = (struct ext2_dir_entry_2 *)blk_buf;
	void *dst = dirent;

	while (de < blk_buf + blk_size) {
		if (de->name_len == 0)
			break;

		if (de->inode == 0)
			continue;

		memcpy(dst, de, de->rec_len);
		dst = dst + de->rec_len;
		de = (struct ext2_dir_entry_2 *)((void *)de + de->rec_len);
	}

	return dst - dirent;
}

static int ext2_readdir(struct file *fp, struct linux_dirent *dirent)
{
	struct dentry *de = fp->f_dentry;
	struct inode *ino = fp->f_dentry->d_inode;
	struct super_block *sb = fp->f_dentry->d_inode->i_sb;
	struct ext2_sb_info *sbi = (struct ext2_sb_info *)sb->s_fs_info;
	struct ext2_super_block *esb = &((struct ext2_sb_info *)sb->s_fs_info)->e2_sb;
	struct ext2_group_desc *gdt = ((struct ext2_sb_info *)sb->s_fs_info)->gdt;
	struct ext2_inode *eino;
	int blk_size = 1024 << esb->s_log_block_size;
	char buff[blk_size];
	unsigned long ino_num;
	unsigned long grp_off;
	unsigned long blk_off;
	unsigned long ino_off;
	size_t sz_in_blk;
	int i, j, k, l;
	int ids;
	int ret;
	int blk_no;
	char blk_buf[blk_size];
	int blk_index1[blk_size / sizeof(int)];
	int blk_index2[blk_size / sizeof(int)];
	int blk_index3[blk_size / sizeof(int)];

	ino_num = ino->i_ino;
	grp_off = ino_num / esb->s_inodes_per_group;
	ino_off = (ino_num % esb->s_inodes_per_group) * esb->s_inode_size;
	blk_off = gdt[grp_off].bg_inode_table + ino_off / blk_size;

	ext2_read_block(sb, buff, blk_off, 0, blk_size);
	eino = (struct ext2_inode *)(buff + ino_off);

	sz_in_blk = (eino->i_size + blk_size - 1) / blk_size;

	ids = blk_size / 4;
	for (i = 0; i < 12 && i < sz_in_blk; i++) {
		ext2_read_block(sb, blk_buf, eino->i_block[i], 0, blk_size);

	}

	if (sz_in_blk > 12) {
		ext2_read_block(sb, blk_index1, eino->i_block[13], 0, blk_size);

		for (l = 0; i < sz_in_blk && l < ids; i++, l++) {
			blk_no = blk_index1[l];
			ext2_read_block(sb, blk_buf, blk_no, 0, blk_size);
		}
	}

	if (sz_in_blk > 12 + ids) {
		ext2_read_block(sb, blk_index2, eino->i_block[14], 0, blk_size);

		for (j = 0; j < ids; j++) {
			ext2_read_block(sb, blk_index1, blk_index2[j], 0, blk_size);

			for (l = 0; i < sz_in_blk && l < ids; i++, l++) {
				blk_no = blk_index1[l];
				ext2_read_block(sb, blk_buf, blk_no, 0, blk_size);
			}

			if (i == sz_in_blk && i == 12 + ids + ids * ids)
				break;
		}
	}

	if (sz_in_blk > 12 + ids + ids * ids){
		ext2_read_block(sb, blk_index3, eino->i_block[15], 0, blk_size);

		for (k = 0; k < ids; k++) {
			ext2_read_block(sb, blk_index2, blk_index3[k], 0, blk_size);

			for (j = 0; j < ids; j++) {
				ext2_read_block(sb, blk_index1, blk_index2[j], 0, blk_size);

				for (l = 0; i < sz_in_blk && l < ids; i++, l++) {
					blk_no = blk_index1[l];
					ext2_read_block(sb, blk_buf, blk_no, 0, blk_size);
				}

				if (i == sz_in_blk && i == 12 + ids + ids * ids)
					break;
			}

			if (i == sz_in_blk)
				break;
		}
	}

error:

	return ret;
}
#endif

extern int ck_ext2_feature(uint32_t fc,uint32_t frc,uint32_t fi);

int ext2_check_fs_type(const char *bdev_name)
{
	struct ext2_super_block *e2_sb;
	struct bio *bio;
	char buff[EXT2_SUPER_BLK_SIZE];
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

	e2_sb = (struct ext2_super_block *)buff;

	if (0xEF53 != e2_sb->s_magic) {
		DPRINT("%s is not \"ext2\" fs!\n", bdev_name);
		return -EINVAL;
	}

	fc = e2_sb->s_feature_compat;
	fi = e2_sb->s_feature_incompat;
	frc = e2_sb->s_feature_ro_compat;

	ret = ck_ext2_feature(fc, frc, fi);

	extern void ext_sb_list(struct ext2_super_block * esb);
	if (ret == 0) {
		ext_sb_list(e2_sb);
	}

	return ret;
}

static struct file_system_type ext2_fs_type = {
	.name    = "ext2",
	.mount   = ext2_mount,
	.kill_sb = ext2_kill_sb,
	.check_fs_type = ext2_check_fs_type,
};

static int __init ext2_init(void)
{
	return register_filesystem(&ext2_fs_type);
}

module_init(ext2_init);
