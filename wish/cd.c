#include <task.h>

static const struct option cd_opt[] = {
	{
		.opt = "[<vol>]",
		.desc = "change CWD to the volume<vol> (HOME by default)."
	},
};

REGISTER_HELP_L1(cd, "Change CWD.", cd_opt);
