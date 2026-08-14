#include <init.h>
#include <stdio.h>
#include <delay.h>
#include <string.h>
#include <errno.h>
#include <mmc/mmc.h>
#include <mmc/mmc_ops.h>

#define BLKNR 1
#define MMC_BLK_SIZE 512
#define MMC_HOST_NUM 5

static int mmc_card_count = 0; // FIXME

static struct mmc_host *g_mmc_host[MMC_HOST_NUM];

int mmc_erase_blk(struct mmc_host *host, int start)
{
	int ret = 0;
	ret = host->send_cmd(host, 32, start, R1);
	if (ret < 0)
		goto out;

	ret = host->send_cmd(host, 33, start + MMC_BLK_SIZE, R1);
	if (ret < 0)
		goto out;

	ret = host->send_cmd(host, MMC_ERASE, 0, R1b);
	if (ret < 0)
		goto out;

	while (1) {
		ret = host->send_cmd(host, MMC_SEND_STATUS,  host->card.rca << 16, R1);
		if (host->resp[0] & 0x100)
			break;
	}

	return 0;
out:
	return ret;
}

int mmc_read_blk(struct mmc_host *host, __u8 *buf, int start)
{
	int ret = 0;

	if (host->card.raw_csd[3] & 3 << 29)
		start = start >> 9;

	ret = host->send_cmd(host, MMC_READ_SINGLE_BLOCK, start, R1);
	if (ret < 0)
		return ret;

	ret = host->read_data(host, buf);
	if (ret < 0)
		return ret;

	udelay(1000);


	return 0;
}

int mmc_write_blk(struct mmc_host *host, const __u8 *buf, int start)
{
	int ret = 0;

	if (host->card.raw_csd[3] & 3 << 29)
		start = start >> 9;

	ret = host->send_cmd(host, MMC_WRITE_BLOCK, start, R1);
	if (ret < 0)
		return ret;

	ret = host->write_data(host, buf);
	if (ret < 0)
		return ret;

	udelay(1000);

	return 0;
}

int mmc_decode_cid(struct mmc_host *host)
{
	char *name = host->card.card_name;
	struct mmc_card *card = &host->card;

	name[0] = (char)(card->raw_cid[3]) & 0xff;
	name[1] = card->raw_cid[2] >> 24;
	name[2] = (card->raw_cid[2] >> 16) & 0xff;
	name[3] = (card->raw_cid[2] >> 8) & 0xff;
	name[4] = card->raw_cid[2] & 0xff;
	name[5] = '\0';

	if (!name[0])
		return -1;

	return 0;
}

static int mmc_get_block(struct disk_drive *drive, int start, void *buff)
{
	struct mmc_card *card = container_of(drive, struct mmc_card, drive);

	return mmc_read_blk(card->host, buff, start);
}

static int mmc_put_block(struct disk_drive *drive, int start, const void *buff)
{
	struct mmc_card *card = container_of(drive, struct mmc_card, drive);

	return mmc_write_blk(card->host, buff, start);
}

static int mmc_card_register(struct mmc_card *card)
{
	struct disk_drive *drive = &card->drive;

	sprintf(drive->bdev.name, BDEV_NAME_MMC "%d", mmc_card_count);
	mmc_card_count++;

	// TODO: fix size
	drive->bdev.base = 0;
	drive->bdev.size = 0;
	drive->sect_size = MMC_BLK_SIZE;

	drive->get_block = mmc_get_block;
	drive->put_block = mmc_put_block;

	return disk_drive_register(&card->drive);
}

int mmc_sd_detect_card(struct mmc_host *host)
{
	int ret = 0;
	__u32 val;
	struct mmc_card *card;

	if (!host)
		return -ENODEV;

	card = &host->card;
	memset(card, 0, sizeof(*card));
	card->host = host;

	if (host->set_lclk)
		host->set_lclk();

	/*cmd0*/
	ret = mmc_go_idle(host);
	if (ret)
		goto out;

	/*cmd8*/
	ret = mmc_send_if_cond(host, 0x1AA);

	val = (ret == 0) ? 0x40ff8000 : 0x00ff8000;

	/*acmd41*/
	ret = mmc_send_app_op_cond(host, val, NULL);
	if (ret)
		goto out;

	/*cmd2*/
	ret = mmc_all_send_cid(host, card->raw_cid);
	if (ret)
		goto out;
	ret = mmc_decode_cid(host);
	if (ret)
		return ret;

	/*cmd3*/
	ret = mmc_set_relative_addr(host);
	if (ret)
		goto out;

	/*cmd9*/
	ret = mmc_send_csd(host, card->raw_csd);
	if (ret)
		goto out;

	ret = mmc_select_card(host);
	if (ret)
		goto out;

	ret = mmc_switch_width(host);
	if (ret)
		goto out;

	if (host->set_hclk)
		host->set_hclk();
	else
		printf("Host not support to switch high frequency!\n");

	//mmc_app_set_bus_width(host,SD_BUS_WIDTH_4);

	//ret = mmc_set_block_len(host, MMC_BLK_SIZE);

	ret = mmc_card_register(card);

	return 0;
out:
	return ret;
}

struct mmc_host * mmc_get_host(int id)
{
	if (id >= MMC_HOST_NUM || id < 0)
		return NULL;

	return g_mmc_host[id];
}

int mmc_register(struct mmc_host *host)
{
	int ret, i;

	if (!host || !host->send_cmd)
		return -EINVAL;

	for (i = 0; i < MMC_HOST_NUM; i++) {
		if (NULL == g_mmc_host[i])
			break;
	}

	if (MMC_HOST_NUM == i) {
		ret = -EBUSY;
		goto L1;
	}

	g_mmc_host[i] = host;

	ret = mmc_sd_detect_card(host);
	if (ret < 0) {
		printf("No SD card found!\n");
		goto L1;
	}

	printf("SD Card found, name = %s\n", host->card.card_name);

L1:
	return ret;
}

static int __init mmc_init(void)
{
	return 0;
}

subsys_init(mmc_init);
