#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <image.h>
#include <mtd/mtd.h>

typedef struct {
	unsigned chunkId:20;
	unsigned serialNumber:2;
	unsigned byteCount:10;
	unsigned objectId:18;
	unsigned ecc:12;
	unsigned unusedStuff:2;
} YAFFS_TAGS;

typedef union {
	YAFFS_TAGS asTags;
	__u8 asBytes[8];
} YAFFS_TAGS_UNION;

//FIXME: tmp code
static struct nand_oob_layout g_yaffs_oob =
{
	.ecc_code_len = 6,
	.ecc_pos    = {8, 9, 10, 13, 14, 15},
	.free_region =
	{
		{.nOfOffset = 0, .nOfLen = 4},
		{.nOfOffset = 6, .nOfLen = 2},
		{.nOfOffset = 11, .nOfLen = 2}
	}
};

static void yaffs_calc_tags_ecc(YAFFS_TAGS * tags)
{
	unsigned char *b = ((YAFFS_TAGS_UNION *)tags)->asBytes;
	unsigned i, j;
	unsigned ecc = 0;
	unsigned bit = 0;

	tags->ecc = 0;

	for (i = 0; i < 8; i++) {
		for (j = 1; j & 0xff; j <<= 1) {
			bit++;
			if (b[i] & j)
				ecc ^= bit;
		}
	}

	tags->ecc = ecc;
}

static int yaffs_check_ecc_ontags(YAFFS_TAGS * tags)
{
	unsigned ecc = tags->ecc;

	yaffs_calc_tags_ecc(tags);

	ecc ^= tags->ecc;

	if (ecc && ecc <= 64) {
		unsigned char *b = ((YAFFS_TAGS_UNION *)tags)->asBytes;

		ecc--;
		b[ecc / 8] ^= (1 << (ecc & 7));
		yaffs_calc_tags_ecc(tags);

		return 1;
	} else if (ecc)
		return -1;

	return 0;
}

static bool check_yaffs1_image(const void *imagebuf)
{
	int ret;
	__u32 offset = 0, bytes = 0;
	__u8 free_oob_buff[YAFFS_OOB_SIZE], *oob_buf = free_oob_buff;
	struct oob_free_region *free;

	memset(free_oob_buff, 0, YAFFS_OOB_SIZE);

	for(free = g_yaffs_oob.free_region; free->nOfLen; free++) {
		bytes  = free->nOfLen;
		offset = free->nOfOffset;

		memcpy(oob_buf, imagebuf + offset, bytes);
		oob_buf += bytes;
	}

	ret = yaffs_check_ecc_ontags((YAFFS_TAGS *)free_oob_buff);
	if (ret < 0)
		return false;

	return true;
}

image_t image_type_detect(const void *data, size_t size)
{
	if (GTH_MAGIC == *(const __u32 *)(data + GTH_MAGIC_OFFSET)) {
		GEN_DBG("witrom image\n");
		return IMG_GTH;
	}

	if (GBH_MAGIC == *(const __u32 *)(data + GBH_MAGIC_OFFSET)) {
		GEN_DBG("witOS image\n");
		return IMG_GBH;
	}

	if (LINUX_MAGIC == *(const __u32 *)(data + LINUX_MAGIC_OFFSET)) {
		GEN_DBG("Linux kernel image\n");
		return IMG_LINUX;
	}

	if (JFFS2_MAGIC == *(const __u16 *)(data + JFFS2_MAGIC_OFFSET)) {
		GEN_DBG("JFFS2 image\n");
		return IMG_JFFS2;
	}

	if (UBIFS_MAGIC == *(const __u32 *)(data + UBIFS_MAGIC_OFFSET)) {
		GEN_DBG("UBIFS image\n");
		return IMG_UBIFS;
	}

	if (check_yaffs1_image(data + YAFFS_OOB_SIZE)) {
		GEN_DBG("YAFFS1 image\n");
		return IMG_YAFFS1;
	}

	// TODO: check other image types
	GEN_DBG("Unknown image type!\n");
	return IMG_UNKNOWN;
}
