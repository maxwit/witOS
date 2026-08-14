#include <task.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

int help(int argc, char *argv[])
{
	int ret;
	const struct command *exe;
	extern const struct command g_exe_begin[], g_exe_end[];

	switch (argc) {
	case 1:
		printf("\nwitOS commands:\n");

		for (exe = g_exe_begin; exe < g_exe_end; exe++)
			printf("  %-8s\n", exe->name);

		ret = 0;
		break;

	case 2:
		for (exe = g_exe_begin; exe < g_exe_end; exe++) {
			if (!strcmp(exe->name, argv[1])) {
				ret = 0;
				break;
			}
		}

		if (g_exe_end == exe) {
			printf("help: %s: No such command\n", argv[1]);
			ret = -ENOENT;
		}

		break;

	default:
		printf("error: too many arguments\n");

		ret = -EINVAL;
		break;
	}

	return ret;
}
