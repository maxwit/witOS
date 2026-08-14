#include <errno.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#define LEN 128
// FIXME
#define MAX_OPT 5

struct conf_opt {
	int opt;
	const char *attr, *val;
};

static int do_config(int option, const char *arg, const char *val)
{
	char buff[LEN];
	int ret;

	switch (option) {
	case 'a':
	case 's':
		ret = conf_set_attr(arg, val);
		if (ret < 0) {
			ret = conf_add_attr(arg, val);
			if (ret < 0)
				break;
		}
	case 'g':
		ret = conf_get_attr(arg, buff);
		if (!ret)
			printf("%s = %s\n", arg, buff);
		break;

	case 'd':
		ret = conf_del_attr(arg);
		break;

	case 'l':
		ret = conf_list_attr();
		break;

	default:
		printf("Invalid option '%c'\n", option);
		ret = -EINVAL;
		break;
	}

	return ret;
}


// space support?
int main(int argc, char *argv[])
{
	int i, j, opt, ret;
	struct conf_opt opt_list[MAX_OPT];

	i = 0;
	while ((opt = getopt(argc, argv, "a:g:d:s:lh")) != -1) {
		switch (opt) {
		case 'd':
		case 'g':
			opt_list[i].attr = optarg;
		case 'l':
			opt_list[i].opt = opt;
			break;

		case 'a':
		case 's':
			if (optind == argc || '-' == argv[optind][0]) {
				usage();
				return -EINVAL;
			}
			opt_list[i].opt  = opt;
			opt_list[i].attr = optarg;
			opt_list[i].val  = argv[optind];
			optind++;
			break;

		case 'h':
			usage();
			return 0;

		default:
			usage();
			return -EINVAL;
		}

		i++;
	}

	if (0 == i) {
		if (1 == argc) {
			opt_list[i].opt = 'l';
		} else if (3 == argc) {
			opt_list[i].opt  = 's';
			opt_list[i].attr = argv[1];
			opt_list[i].val  = argv[2];
		}

		i++;
	}

	for (j = 0; j < i; j++) {
		ret = do_config(opt_list[j].opt, opt_list[j].attr, opt_list[j].val);
		if (ret < 0)
			return -EINVAL;
	}

#if 0
	ret = conf_store();
	if (ret < 0)
		printf("Fail to save sysconf (ret = %d)!\n", ret);
#endif

	return ret;
}
