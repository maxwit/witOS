#include <errno.h>
#include <task.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

static inline void list_option(const struct help_info *help)
{
	int i;
	const struct option *opt = help->u.optv;

	for (i = 0; i < help->count; i++)
		printf("  %s\n   %s\n", opt[i].opt, opt[i].desc);
	printf("  -h\n   this help\n");
}

int usage(void)
{
	int i;
	int argc;
	char **argv;
	// const struct option *opt;
	const struct task *current;
	const struct help_info *help, *subcmd;

	current = get_current_task();
	assert(current);

	argc = current->argc;
	argv = current->argv;
	help = current->help;
	if (!help) {
		printf("%s: help infomation not available.\n"
			"Please refer to witOS user manual.\n", argv[0]);
		return -ENOENT;
	}

	if (1 == help->level) {
		// opt = help->u.optv;

		printf("Usage: %s [options]\n", argv[0]);
		if (help->desc)
			printf("%s\n\n", help->desc);

		printf("option list:\n");
		list_option(help);
	} else if (2 == help->level) {
		subcmd = help->u.cmdv;

		if (argc >= 2) {
			for (i = 0; i < help->count; i++) {
				if (!strcmp(subcmd[i].name, argv[1])) {
					// opt = subcmd[i].u.optv;

					printf("specific %s options:\n", subcmd[i].name);
					list_option(&subcmd[i]);

					return 0;
				}
			}
		}

		printf("Usage: %s <command> [<args>]\n", argv[0]);
		if (help->desc)
			printf("%s\n\n", help->desc);

		printf("command list:\n");
		for (i = 0; i < help->count; i++) {
			assert(subcmd[i].desc);
			printf("  %-12s%s\n", subcmd[i].name, subcmd[i].desc);
		}

		printf("See '%s <command> -h' for more information on a specific command.\n", argv[0]);
	} else {
		BUG();
	}

	return 0;
}
