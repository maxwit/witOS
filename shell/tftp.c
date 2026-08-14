#include <task.h>

static struct option tftp_get_option[] = {
	{
		.opt = "-r <URL>",
		.desc = " remote URL, i.e. \"10.0.0.2:69/witOS.bin\".",
	},
	{
		.opt = "-l <path>",
		.desc = " local path.",
	},
	{
		.opt = "-m <mode>",
		.desc = " transfer mode: text or binary (default is binary).",
	},
	{
		.opt = "-t <type>",
		.desc = " image type for image file burning (default auto detect)",
	},
	{
		.opt = "-v",
		.desc = " verbose mode.",
	},
	{
		.opt = "-a <address>",
		.desc = " only load to the memory <address>, without writing to stoarge. (affect GO).",
	},

};

static struct option tftp_put_option[] = {
	{
		.opt = "-r <URL>",
		.desc = " remote URL, i.e. \"10.0.0.2:69/witOS.bin\".",
	},
	{
		.opt = "-l <path>",
		.desc = " local path.",
	},
	{
		.opt = "-m <mode>",
		.desc = " transfer mode: text or binary (default is binary).",
	},
	{
		.opt = "-t <type>",
		.desc = " image type for image file burning (default auto detect)",
	},
	{
		.opt = "-v",
		.desc = " verbose mode.",
	},
};

static const struct help_info tftp_subcmd_list[] = {
	{
		.name  = "get",
		.desc  = "download file",
		.level = 1,
		.count = ARRAY_ELEM_NUM(tftp_get_option),
		.u = {
			.optv = tftp_get_option,
		},
	},
	{
		.name  = "put",
		.desc  = "upload file",
		.level = 1,
		.count = ARRAY_ELEM_NUM(tftp_put_option),
		.u = {
			.optv = tftp_put_option,
		},
	},
};

REGISTER_HELP_L2(tftp, "Trivial File Transfer Protocol client.", tftp_subcmd_list);
