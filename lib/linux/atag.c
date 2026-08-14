#include <stdio.h>
#include <string.h>
#include "linux.h"

#define CONFIG_RAM_BANK_NUM  1 // FIXME!

struct tag *begin_setup_atag (void *tag_base)
{
	struct tag *cur_tag;

	cur_tag = (struct tag *)tag_base;

	cur_tag->hdr.tag = ATAG_CORE;
	cur_tag->hdr.size = tag_size(tag_core);

	cur_tag->u.core.flags    = 0;
	cur_tag->u.core.pagesize = 0;
	cur_tag->u.core.rootdev  = 0;

	return cur_tag;
}

struct tag *setup_cmdline_atag(struct tag *cur_tag, char *cmd_line)
{
	cur_tag = tag_next(cur_tag);

	while (*cmd_line && ' ' == *cmd_line)
		cmd_line++;

	cur_tag->hdr.tag = ATAG_CMDLINE;
	cur_tag->hdr.size = (sizeof(struct tag_header) + strlen(cmd_line) + 3) >> 2;

	strcpy(cur_tag->u.cmdline.cmdline, cmd_line);

	return cur_tag;
}

// FIXME
struct tag *setup_mem_atag (struct tag *cur_tag)
{
	int i;

	for (i = 0; i < CONFIG_RAM_BANK_NUM; i++) {
		cur_tag = tag_next(cur_tag);

		cur_tag->hdr.tag  = ATAG_MEM;
		cur_tag->hdr.size = tag_size(tag_mem32);

		cur_tag->u.mem.start = SDRAM_BASE;
		cur_tag->u.mem.size  = SDRAM_SIZE;
	}

	return cur_tag;
}

#if 0
struct tag *setup_ramdisk_atag(struct tag *cur_tag, struct image_cache *rd_cache)
{
	cur_tag = tag_next(cur_tag);

	cur_tag->hdr.tag = ATAG_RAMDISK;
	cur_tag->hdr.size = tag_size(TagRamDisk);
	cur_tag->stRamDisk.flags = 3; // FIXME!!
	cur_tag->stRamDisk.nStart = (__u32)rd_cache->cache_base;
	cur_tag->stRamDisk.size  = rd_cache->cache_size;

	return cur_tag;
}
#endif

struct tag *setup_initrd_atag(struct tag *cur_tag, void *image_buff, __u32 image_size)
{
	cur_tag = tag_next(cur_tag);

	cur_tag->hdr.tag  = ATAG_INITRD2;
	cur_tag->hdr.size = tag_size(tag_initrd);
	cur_tag->u.initrd.start = (__u32)image_buff;
	cur_tag->u.initrd.size  = image_size;

	return cur_tag;
}

void end_setup_atag (struct tag *cur_tag)
{
	cur_tag = tag_next(cur_tag);

	cur_tag->hdr.tag = ATAG_NONE;
	cur_tag->hdr.size = 0;
}
