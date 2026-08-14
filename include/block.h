#pragma once

#include <types.h>
#include <list.h>
#include <device.h>

#define LABEL_NAME_SIZE  32

#define BDEV_NAME_FLASH  "mtdblock"
#define BDEV_NAME_MMC    "mmcblk"
#define BDEV_NAME_SCSI   "sd"
#define BDEV_NAME_ATA    "hd" // FIXME
#define BDEV_NAME_NBD    "nbd"

#define BDF_RDONLY       (1 << 0)

typedef unsigned long sector_t;

struct block_device;

struct part_attr {
	__u32 base;
	__u32 size;
	char  label[LABEL_NAME_SIZE];
	__u32 flags;
};

struct block_device {
	// struct device dev;
	char name[MAX_DEV_NAME]; // mmcblockXpY, mtdblockN, sdaN

	// use part_attr instead?
	__u32  flags;
	size_t base;
	size_t size;
	char   label[LABEL_NAME_SIZE];

	const struct file_operations *fops;

	struct list_head bdev_node;
};

int block_device_register(struct block_device *bdev);

enum {
	READ = 1,
	WRITE,
};

struct bio {
	void     *data;
	size_t   size;
	sector_t sect;
	struct block_device *bdev;
	unsigned long flags;
};

struct bio *bio_alloc();

void bio_free(struct bio *bio);

void submit_bio(int rw,struct bio * bio);

struct block_device *bdev_get(const char *name);
