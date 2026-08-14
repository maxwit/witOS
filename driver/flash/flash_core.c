#include <io.h>
#include <init.h>
#include <delay.h>
#include <errno.h>
#include <string.h>
#include <malloc.h>
#include <mtd/mtd.h>

static LIST_HEAD(g_master_list);
static int g_flash_count = 0;

/*
 * witOS flash partition definition
 * original from Linux kernel (driver/mtd/cmdlinepart.c)
 *
 * mtdparts  := <mtddef>[;<mtddef]
 * <mtddef>  := <mtd-id>:<partdef>[,<partdef>]
 *              where <mtd-id> is the name from the "cat /proc/mtd" command
 * <partdef> := <size>[@offset][<name>][ro][lk]
 * <mtd-id>  := unique name used in mapping driver/device (mtd->name)
 * <size>    := standard linux memsize or "-" to denote all remaining space
 * <name>    := '(' NAME ')'
 *
 * Examples:  1 NOR Flash with 2 partitions, 1 NAND with one
 * edb7312-nor:256k(ARMboot)ro,-(root);edb7312-nand:-(home)
 *
 *
 * FIXME:
 * 1. to fix cross/overlapped parts
 * 2. as an API and called from flash core
 * 3. to support <partdef> := <size>[@offset][(<label>[, <image_name>, <image_size>])]
 * 4. handle exception
 *
 */

static int __init flash_parse_part(struct mtd_info *host,
						struct part_attr *part,	const char *part_def)
{
	int i, ret = -EINVAL, index = 0;
	__u32 curr_base = 0;
	char buff[128];
	const char *p;

	p = strchr(part_def, ':');
	if (!p)
		goto error;

	p++;

	while (*p && *p != ';') {
		if (curr_base >= host->chip_size)
			goto error;

		while (' ' == *p) p++;

		// part size
		if (*p == '-') {
			part->size = host->chip_size - curr_base;
			p++;
		} else {
			for (i = 0; *p; i++, p++) {
				if (*p == '@' || *p == '(' || *p == 'r' || *p == ',')
					break;
				buff[i] = *p;
			}
			buff[i] = '\0';

			ret = hr_str_to_val(buff, (unsigned long *)&part->size);
			if (ret < 0)
				goto error;

			ALIGN_UP(part->size, host->erase_size);
		}

		// part base
		if (*p == '@') {
			for (i = 0, p++; *p; i++, p++) {
				if (*p == '(' || *p == 'r' || *p == ',')
					break;
				buff[i] = *p;
			}
			buff[i] = '\0';

			ret = hr_str_to_val(buff, (unsigned long *)&part->base);
			if (ret < 0)
				goto error;

			ALIGN_UP(part->base, host->erase_size);

			curr_base = part->base;
		} else {
			part->base = curr_base;
		}

		// part label and image
		i = 0;
		if (*p == '(') {
			for (p++; *p && *p != ')'; i++, p++)
				part->label[i] = *p;

			p++;
		}

		if (*p == 'r') {
			p++;
			if (*p != 'o') {
				return -EINVAL;
			}

			part->flags |= BDF_RDONLY;
			p++;
		}

		if (',' == *p)
			p++;

		part->label[i] = '\0';

		curr_base += part->size;

		index++;
		part++;
	}

	return index;

error:
	printf("%s(): invalid part definition \"%s\"\n", __func__, part_def);
	return ret;
}

static size_t remove_blank_space(const char *src, char *dst, size_t size)
{
	char *p = dst;

	while (*src) {
		if (*src != ' ') {
			*p = *src;
			p++;
		}

		src++;
	}

	*p = '\0';

	return p - dst;
}

static int __init flash_scan_part(struct mtd_info *host,
						struct part_attr part[])
{
	int ret;
	char part_def[CONF_VAL_LEN];
	char conf_val[CONF_VAL_LEN];
	const char *match_parts;

	ret = conf_get_attr("flash.parts", conf_val);
	if (ret < 0) {
		return ret;
	}

	match_parts = strstr(conf_val, host->name);
	if (!match_parts) {
		// TODO: add hint here
		return -ENOENT;
	}

	remove_blank_space(match_parts, part_def, sizeof(part_def));

	return flash_parse_part(host, part, part_def);
}

static int part_read(struct mtd_info *slave,
				__u32 from, __u32 len, size_t *retlen, __u8 *buff)
{
	struct mtd_info *master = slave->master;

	return master->read(master, slave->bdev.base + from, len, retlen, buff);
}

static int part_write(struct mtd_info *slave,
				__u32 to, __u32 len, __u32 *retlen, const __u8 *buff)
{
	struct mtd_info *master = slave->master;

	return master->write(master, slave->bdev.base + to, len, retlen, buff);
}

static int part_erase(struct mtd_info *slave, struct erase_info *opt)
{
	struct mtd_info *master = slave->master;

	opt->addr += slave->bdev.base;
	return master->erase(master, opt);
}

static int part_read_oob(struct mtd_info *slave,
				__u32 from, struct mtd_oob_ops *ops)
{
	struct mtd_info *master = slave->master;

	return master->read_oob(master, slave->bdev.base + from, ops);
}

static int part_write_oob(struct mtd_info *slave,
				__u32 to,	struct mtd_oob_ops *opt)
{
	struct mtd_info *master = slave->master;

	return master->write_oob(master, slave->bdev.base + to, opt);
}

static int part_block_is_bad(struct mtd_info *slave, __u32 off)
{
	struct mtd_info *master = slave->master;

	return master->block_isbad(master, slave->bdev.base + off);
}

static int part_block_mark_bad(struct mtd_info *slave, __u32 off)
{
	struct mtd_info *master = slave->master;

	return master->block_markbad(master, slave->bdev.base + off);
}

int flash_register(struct mtd_info *mtd)
{
	int i, n, ret;
	struct mtd_info *slave;
	struct part_attr part_tab[MAX_FLASH_PARTS];

	printf("registering flash device \"%s\":\n", mtd->name);
	list_add_tail(&mtd->master_node, &g_master_list);

	memset(part_tab, 0, sizeof(part_tab));
	n = flash_scan_part(mtd, part_tab);

	if (n <= 0) {
		snprintf(mtd->bdev.name, MAX_DEV_NAME,
			BDEV_NAME_FLASH "%c", 'A' + g_flash_count);

		strncpy(mtd->bdev.label, "mtd", sizeof(mtd->bdev.label));

		flash_fops_init(&mtd->bdev); // FIXME: not here!
		ret = block_device_register(&mtd->bdev);
		// if ret < 0 ...
	} else {
		INIT_LIST_HEAD(&mtd->slave_list);

		for (i = 0; i < n; i++) {
			slave = zalloc(sizeof(*slave));
			if (NULL == slave)
				return -ENOMEM;

			snprintf(slave->bdev.name, LABEL_NAME_SIZE, BDEV_NAME_FLASH "%d", i + 1);

			slave->bdev.flags = part_tab[i].flags;
			slave->bdev.base  = part_tab[i].base;
			slave->bdev.size  = part_tab[i].size;
			strncpy(slave->bdev.label, part_tab[i].label,
				sizeof(slave->bdev.label));

			slave->write_size  = mtd->write_size;
			slave->erase_size  = mtd->erase_size;
			slave->chip_size   = mtd->chip_size;
			slave->write_shift = mtd->write_shift;
			slave->erase_shift = mtd->erase_shift;
			slave->chip_shift  = mtd->chip_shift;
			slave->type        = mtd->type;
			slave->oob_size    = mtd->oob_size;
			slave->oob_mode    = mtd->oob_mode;
			slave->master      = mtd;

			slave->read  = part_read;
			slave->write = part_write;
			slave->erase = part_erase;
			slave->read_oob  = part_read_oob;
			slave->write_oob = part_write_oob;
			slave->block_isbad   = part_block_is_bad;
			slave->block_markbad = part_block_mark_bad;
			slave->scan_bad_block = mtd->scan_bad_block; // FIXME

			list_add_tail(&slave->slave_node, &mtd->slave_list);

			flash_fops_init(&slave->bdev);
			ret = block_device_register(&slave->bdev);
			// if ret < 0 ...
		}
	}

	g_flash_count++;

	return ret;
}

int flash_unregister (struct mtd_info *mtd)
{
	// TODO: check master or not

	return 0;
}

struct mtd_info *get_mtd_device(void *nil, unsigned int num)
{
	unsigned int i = 1;
	struct list_head *iter;
	struct mtd_info *master, *mtd;

	list_for_each_entry(master, &g_master_list, master_node)
		list_for_each(iter, &master->slave_list) {
			if (i == num) {
				mtd = container_of(iter, struct mtd_info, slave_node);
				return mtd;
			}
			i++;
		}

	return NULL;
}

static int __init flash_init(void)
{
	return 0;
}

subsys_init(flash_init);
