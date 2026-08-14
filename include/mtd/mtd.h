#pragma once

#include <types.h>
#include <list.h>
#include <block.h>

#define MTD_CHAR_MAJOR 90
#define MTD_BLOCK_MAJOR 31

// #define MTD_DEV_NAME_LEN    32

#define MAX_FLASH_PARTS	    16
// #define DEF_VOL_ID          1  // FIXME: default point to witOS bottom half
#define BOOT_FLASH_ID       0 // FIXME

#define FLASH_ABSENT		0
#define FLASH_RAM			1
#define FLASH_ROM			2
#define MTD_NORFLASH		3
#define MTD_NANDFLASH		4
#define MTD_DATAFLASH		6
#define MTD_UBIVOLUME		7
#define FLASH_FLASH_PART   (1 << 6) // FIXME

#define MTD_WRITEABLE		0x400	/* Device is writeable */
#define MTD_BIT_WRITEABLE	0x800	/* Single bits can be flipped */
#define MTD_NO_ERASE		0x1000	/* No erase necessary */
#define MTD_POWERUP_LOCK	0x2000	/* Always locked after reset */

enum {
	FLASH_IOCG_INFO,
	FLASH_IOCS_OOB_MODE,
	FLASH_IOCG_OOB_MODE,
	FLASH_IOCS_CALLBACK,
	FLASH_IOCG_SIZE,
	FLASH_IOC_SCANBB,
	FLASH_IOC_ERASE,
};

#define BLOCK_DEV_NAME_LEN  32
#define MTD_ID_NAME_LEN     32

#define IS_FS_PART(type) (IMG_BEGIN <= (type) && (type) <= IMG_END)

struct mtd_info;

struct oob_free_region {
	__u32 nOfOffset;
	__u32 nOfLen;
};

#define FLASH_MAX_OOBFREE_ENTRIES	8

struct nand_oob_layout {
	__u32 ecc_code_len;
	__u32 ecc_pos[64];

	__u32 free_oob_sum;
	struct oob_free_region free_region[FLASH_MAX_OOBFREE_ENTRIES];
};

struct ecc_stats {
	__u32 ecc_correct_count;
	__u32 ecc_failed_count;
	__u32 badblocks;
	__u32 bbtblocks;
};

typedef struct {
	__u32 page_index;
	__u32 block_index;
} FLASH_HOOK_PARAM;

typedef int (*FLASH_HOOK_FUNC)(struct mtd_info *, FLASH_HOOK_PARAM *);

typedef struct {
	FLASH_HOOK_PARAM *args;
	FLASH_HOOK_FUNC func;
} FLASH_CALLBACK;

#define MAX_FLASH_DEVICES  32

#define FLASH_ERASE_PENDING 0x01
#define FLASH_ERASING		0x02
#define FLASH_ERASE_SUSPEND	0x04
#define FLASH_ERASE_DONE    0x08
#define FLASH_ERASE_FAILED  0x10

#define MAX_ECC_STEPS  16

// Flash erase flags
#define EDF_NORMAL     0
#define EDF_JFFS2      (1 << 0)
#define EDF_ALLOWBB    (1 << 8)

struct erase_info {
	struct mtd_info *mtd;
	uint64_t addr;
	uint64_t len;
	uint64_t fail_addr;
	u_long time;
	u_long retries;
	unsigned dev;
	unsigned cell;
	void (*callback) (struct erase_info *self);
	u_long priv;
	u_char state;
	struct erase_info *next;
	__u32 flags; // FIXME
};

#if 0
struct erase_info {
	__u32 estart;
	__u32 esize;
	__u32 flags;
	__u32 fail_addr; // fail_at, faddr
	__u8  estate;
};
#endif

typedef enum {
	FLASH_OOB_PLACE,
	MTD_OPS_AUTO_OOB,
	FLASH_OOB_RAW,
} OOB_MODE;

struct mtd_oob_ops {
	unsigned int	mode;
	size_t		len;
	size_t		retlen;
	size_t		ooblen;
	size_t		oobretlen;
	uint32_t	ooboffs;
	uint8_t		*datbuf;
	uint8_t		*oobbuf;
};

struct image_info {
	char  image_name[FILE_NAME_SIZE];
	__u32 image_size;
};

struct flash_info {
	int type;
	size_t block_size;
	size_t page_size;
	size_t oob_size;
	OOB_MODE oob_mode;
	const char *name;
	size_t bdev_base;
	size_t bdev_size;
	const char *bdev_label;
};

#define MTD_MAX_OOBFREE_ENTRIES_LARGE	32
#define MTD_MAX_ECCPOS_ENTRIES_LARGE	448

struct nand_oobfree {
	__u32 offset;
	__u32 length;
};

struct nand_ecclayout {
	__u32 eccbytes;
	__u32 eccpos[MTD_MAX_ECCPOS_ENTRIES_LARGE];
	__u32 oobavail;
	struct nand_oobfree oobfree[MTD_MAX_OOBFREE_ENTRIES_LARGE];
};

struct mtd_info {
	uint32_t flags;

	struct block_device bdev;

	union {
		struct list_head master_node;
		struct list_head slave_node;
	};

	union {
		struct list_head slave_list;
		struct mtd_info *master;
	};

	int   type;
	char  name[MTD_ID_NAME_LEN]; // FIXME: no need for slaves

	size_t write_size;
	size_t erase_size;
	size_t chip_size; // FIXME

	__u32 write_shift;
	__u32 erase_shift;
	__u32 chip_shift;

	__u32 oob_size;

	struct ecc_stats eccstat;

	int bad_allow;

	FLASH_HOOK_PARAM *callback_args;
	FLASH_HOOK_FUNC   callback_func;

	int (*read)(struct mtd_info *, __u32, __u32, size_t *, __u8 *);
	int (*write)(struct mtd_info *, __u32, __u32 , __u32 *, const __u8 *);
	int (*erase)(struct mtd_info *, struct erase_info *);

	int (*read_oob)(struct mtd_info *, __u32, struct mtd_oob_ops *);
	int (*write_oob)(struct mtd_info *,__u32, struct mtd_oob_ops *);

	int (*block_isbad)(struct mtd_info *, __u32);
	int (*block_markbad)(struct mtd_info *, __u32);
	int (*scan_bad_block)(struct mtd_info *); // FIXME: to be removed

	OOB_MODE oob_mode;
	//
	struct nand_ecclayout *ecclayout;
};

static __u32 inline flash_write_is_align(struct mtd_info *mtd, __u32 size)
{
	return (size + mtd->write_size - 1) & ~(mtd->write_size - 1);
}

static __u32 inline flash_erase_is_align(struct mtd_info *mtd, __u32 size)
{
	return (size + mtd->erase_size - 1) & ~(mtd->erase_size - 1);
}

int flash_register(struct mtd_info *mtd);

int flash_unregister(struct mtd_info *mtd);

typedef enum {
	NAND_ECC_NONE,
	NAND_ECC_SW,
	NAND_ECC_HW,
//	NAND_ECC_YAFFS,
	NAND_ECC_YAFFS2,
} ECC_MODE;

int flash_set_ecc_mode(struct mtd_info *mtd, ECC_MODE newMode, ECC_MODE *pOldMode);

int flash_fops_init(struct block_device *bdev);

struct mtd_info *get_mtd_device(void *nil, unsigned int num);
