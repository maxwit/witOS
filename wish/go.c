#include <task.h>

static struct option go_option[] = {
	{
		.opt = "[<address>]",
		.desc = "the default address form uart -m [<address>]"
	},
};

REGISTER_HELP_L1(go, "jump to the address without condition.", go_option);
