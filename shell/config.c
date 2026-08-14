#include <task.h>

static const struct option config_opt[] = {
	{
		.opt  = "-a <attr> <value>",
		.desc = "adds a new attribute with value"
	},
	{
		.opt  = "-d <attr>",
		.desc = "removes the attribute"
	},
	{
		.opt  = "-s <attr> <value>",
		.desc = "set the <attr> with new <value>"
	},
	{
		.opt  = "-g <attr>",
		.desc = "get value of <attr>"
	},
	{
		.opt  = "-l",
		.desc = "list all information. (default)"
	},
};

REGISTER_HELP_L1(config, "witOS system configuration utility", config_opt);
