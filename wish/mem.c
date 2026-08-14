#include <task.h>

static struct option mem_dump_option[] = {
	{
		.opt = "-d <o|O|d|D|x|X>",
		.desc = "display format, default is hex.",
	},
	{
		.opt = "-a <address>",
		.desc = "start memory address.",
	},
	{
		.opt = "-l <len>",
		.desc = "data size to dump. default 100 byte",
	},
};

static struct option mem_set_option[] = {
	{
		.opt = "-a <address>",
		.desc = "write the value list to memory begin with <address>",
	},
	{
		.opt = "-v <value>",
		.desc = "set value in byte default 0.",
	},
	{
		.opt = "-l <size>",
		.desc = "move/copy size, in byte|K|M|G, default is byte."
	},
	{
		.opt = "-w <1|2|4>",
		.desc = "operation unit, default is 1-byte.",
	},
};

static struct option mem_write_option[] = {
	{
		.opt = "-a <address> <val1.....valn>",
		.desc = "write the value list to memory begin with <address>.",
	},
	{
		.opt = "-w <1|2|4|8>",
		.desc = "operation unit, default is 1-byte.",
	},
};

static struct option mem_copy_move_option[] = {
	{
		.opt = "-s <src>",
		.desc = "source address",
	},
	{
		.opt = "-d <dst>",
		.desc = "destination address",
	},
	{
		.opt = "-l <size>",
		.desc = "move/copy size, in byte|K|M|G, default is byte.",
	},
};

static struct option mem_free_option[] = {
	{
		.opt = "-d",
		.desc = "show used/free infomation in 0d defalut 0x",
	},
};

static const struct help_info mem_subcmd_list[] = {
	{
		.name = "dump",
		.desc = "display memory in hex and/or ascii modes",
		.level = 1,
		.count = ARRAY_ELEM_NUM(mem_dump_option),
		.u = {
			.optv = mem_dump_option,
		},
	},
	{
		.name = "set",
		.desc = "memory set",
		.level = 1,
		.count = ARRAY_ELEM_NUM(mem_set_option),
		.u = {
			.optv = mem_set_option,
		},
	},
	{
		.name = "write",
		.desc = "memory write",
		.level = 1,
		.count = ARRAY_ELEM_NUM(mem_write_option),
		.u = {
			.optv = mem_write_option,
		},
	},
	{
		.name = "copy",
		.desc = "memory copy",
		.level = 1,
		.count = ARRAY_ELEM_NUM(mem_copy_move_option),
		.u = {
			.optv = mem_copy_move_option,
		},
	},
	{
		.name = "move",
		.desc = "memory move",
		.level = 1,
		.count = ARRAY_ELEM_NUM(mem_copy_move_option),
		.u = {
			.optv = mem_copy_move_option,
		},
	},
	{
		.name = "free",
		.desc = "show used/free infomation",
		.level = 1,
		.count = ARRAY_ELEM_NUM(mem_free_option),
		.u = {
			.optv = mem_free_option,
		},
	},
};


REGISTER_HELP_L2(mem, "mem unility", mem_subcmd_list);
