#include <list.h>
#include <stdio.h>
#include <string.h>
#include <block.h>
#include <assert.h>
#include <malloc.h>
#include <fs.h>
#include <drive.h> // FIXME
#include <fs/devfs.h>

static LIST_HEAD(g_bdev_list);

struct block_device *bdev_get(const char *name)
{
	struct list_head *iter;

	list_for_each(iter, &g_bdev_list) {
		struct block_device *bdev;

		bdev = container_of(iter, struct block_device, bdev_node);
		if (!strcmp(bdev->name, name))
			return bdev;
	}

	return NULL;
}

int block_device_register(struct block_device *bdev)
{
	list_add_tail(&bdev->bdev_node, &g_bdev_list);

	printf("    0x%08x - 0x%08x %s (%s)\n",
		bdev->base, bdev->base + bdev->size, bdev->name,
		bdev->label[0] ? bdev->label : "N/A");

	// device_enqueue(bdev);

	return 0;
}

struct bio *bio_alloc(/* reserved */)
{
	struct bio *bio;

	bio = zalloc(sizeof(*bio));

	return bio;
}

void bio_free(struct bio *bio)
{
	free(bio);
}

void submit_bio(int rw, struct bio *bio)
{
	int ret;
	size_t len;
	sector_t sect = bio->sect;
	struct block_device *bdev = bio->bdev;
	struct disk_drive *drive;

	assert(bdev);

	drive = container_of(bdev, struct disk_drive, bdev);

	for (len = 0; len < bio->size; len += 512) {
		if (READ == rw)
			ret = drive->get_block(drive, sect << 9, bio->data + len);
		else
			ret = drive->put_block(drive, sect << 9, bio->data + len);

		if (ret < 0)
			bio->flags = 1; // FIXME

		sect++;
	}
}

int devfs_bdev_open(struct file *fp, struct inode *inode)
{
	return 0;
}
