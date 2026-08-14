#include <task.h>

static struct option flash_dump_option[] = {
	{
		.opt	= "-d <d|D|o|O|x|X>",
		.desc = "display format.",
	},
	{
		.opt  = "-t <test>",
		.desc = "display format.",
	},
};

static struct option flash_rdwr_option[] = {
	{
		.opt  = "-f <file>",
		.desc = "image file name to read or write.",
	},
};

static struct option flash_erase_option[] = {
	{
		.opt  = "-c <size>",
		.desc = "cleanmark size for JFFS2.",
	},
	{
		.opt  = "-d <ddd>",
		.desc = "cleanmark size for JFFS2.",
	},
	{
		.opt  = "-e <eee>",
		.desc = "cleanmark size for JFFS2.",
	},
};

static const struct help_info flash_subcmd_list[] = {
	{
		.name  = "dump",
		.desc  = "dump data",
		.level = 1,
		.count = ARRAY_ELEM_NUM(flash_dump_option),
		.u = {
			.optv = flash_dump_option,
		},
	},
	{
		.name  = "read",
		.desc  = "read data",
		.level = 1,
		.count = ARRAY_ELEM_NUM(flash_rdwr_option),
		.u = {
			.optv = flash_rdwr_option,
		},
	},
	{
		.name  = "write",
		.desc  = "write data",
		.level = 1,
		.count = ARRAY_ELEM_NUM(flash_rdwr_option),
		.u = {
			.optv = flash_rdwr_option,
		},
	},
	{
		.name  = "erase",
		.desc  = "erase data",
		.level = 1,
		.count = ARRAY_ELEM_NUM(flash_erase_option),
		.u = {
			.optv = flash_erase_option,
		},
	},
};

REGISTER_HELP_L2(flash, "flash utility", flash_subcmd_list);
