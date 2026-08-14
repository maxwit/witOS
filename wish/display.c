#include <task.h>

static const struct option display_opt[] = {
	{
		.opt  = "-l [<all>]",
		.desc = "list video mode (current video mode by default)"
	},
	{
		.opt  = "-s <model> | <N>",
		.desc = "set current video to the built-in model (by name <model> or NO. <N>)."
	},
};

REGISTER_HELP_L1(display, "a useful debug utility for display controller.", display_opt);
