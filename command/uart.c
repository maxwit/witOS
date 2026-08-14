#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <loader.h>
#include <uart/ymodem.h>
#include <uart/kermit.h>
#include <uart/uart.h>
#include <mtd/mtd.h>

static int uart_load(int argc, char *argv[])
{
	int size, ret;
	struct loader_opt ldr_opt;
	char *pro = NULL;
	int opt;

	size = 0;

	memset(&ldr_opt, 0x0, sizeof(ldr_opt));

	ldr_opt.dst = get_current_dir_name();

	while ((opt = getopt(argc, argv, "m::p:f:i:vh")) != -1) {
		switch (opt) {
		case 'm':
			if (optarg != NULL) {
				ret = str_to_val(optarg, (unsigned long *)&ldr_opt.load_addr);
				if (ret < 0) {
					printf("Input a invalied address!\n");

					return -EINVAL;
				}
			}

			ldr_opt.dst = NULL;

			break;

		case 'v':
		case 'i':
		case 'f':

			// FIXME

			break;

		case 'h':
			usage();

			return 0;

		case 'p':
			pro = optarg;

			break;

		default:
			usage();

			return -EINVAL;
		}
	}

	if (0 == strcmp(pro, "k")) {
		printf("load kermit....:");
		size = kermit_load(&ldr_opt);
	} else if (0 == strcmp(pro, "y")) {
		printf("load ymode....:");
		size = ymodem_load(&ldr_opt);
	} else {
		usage();
		return -EINVAL;
	}

	if (ldr_opt.dst && size > 0) {
		char conf_attr[CONF_ATTR_LEN], conf_val[CONF_VAL_LEN];

		snprintf(conf_attr, CONF_ATTR_LEN, "bdev.%s.image.name", ldr_opt.dst);
		if (conf_set_attr(conf_attr, ldr_opt.file_name) < 0) {
			conf_add_attr(conf_attr, ldr_opt.file_name);
		}

		// set file size
		snprintf(conf_attr, CONF_ATTR_LEN, "bdev.%s.image.size", ldr_opt.dst);
		val_to_dec_str(conf_val, size);
		if (conf_set_attr(conf_attr, conf_val) < 0) {
			conf_add_attr(conf_attr, conf_val);
		}

	}

	return size;
}

static int uart_setup(int argc ,char *argv[])
{
	int opt;
	int flag = 0;
	int ret = 0;

	while ((opt = getopt(argc, argv, "hi:")) != -1) {
		switch (opt) {
		case 'i':
			// FIXME
			flag = 1;

			break;

		default:
			ret = -EINVAL;

		case 'h':
			usage();

			return ret;
		}
	}

	if (flag == 1) {
		// FIXME
	} else {
		usage();
	}

	return 0;
}

static int uart_send(int argc ,char *argv[])
{
	int opt;
	int het;
	int ret = 0;
	struct loader_opt ldr_opt;

	memset(&ldr_opt, 0x0, sizeof(ldr_opt));
	while ((opt = getopt(argc, argv, "m::p:f:i:vh")) != -1) {
		switch (opt) {
		case 'm':
			if (optarg != NULL) {
				het = str_to_val(optarg, (unsigned long *)&ldr_opt.load_addr);
				if (het < 0) {
					printf("Input a invalied address!\n");

					return -EINVAL;
				}
			}

			break;

		case 'p':
		case 'v':
		case 'i':
		case 'f':

			// FIXME

			break;

		default:
			ret = -EINVAL;

		case 'h':
			usage();
			return ret;
		}
	}

	// FIXME

	return 0;
}

static int uart_test(int argc ,char *argv[])
{
	int opt;
	int het;
	int ret = 0;
	struct loader_opt ldr_opt;

	memset(&ldr_opt, 0x0, sizeof(ldr_opt));
	while ((opt = getopt(argc, argv, "m::p:f:i:vh")) != -1) {
		switch (opt) {
		case 'm':
			if (optarg != NULL) {
				het = str_to_val(optarg, (unsigned long *)&ldr_opt.load_addr);
				if (het < 0) {
					printf("Input a invalied address!\n");

					return -EINVAL;
				}
			}

			break;

		case 'p':
		case 'v':
		case 'i':
		case 'f':

			// FIXME

			break;

		default:
			ret = -EINVAL;

		case 'h':
			usage();
			return ret;
		}
	}

	// FIXME

	return 0;
}

int main(int argc, char *argv[])
{
	int i;
	int size;
	int ret = 0;

	struct command cmd[] = {
		{
			.name = "load",
			.main = uart_load
		},
		{
			.name = "setup",
			.main = uart_setup
		},
		{
			.name = "send",
			.main = uart_send
		},
		{
			.name = "test",
			.main = uart_test
		}
	};

	if (argc >= 2) {
		for (i = 0; i < ARRAY_ELEM_NUM(cmd); i++) {
			if (0 == strcmp(argv[1], cmd[i].name)) {
				size = cmd[i].main(argc - 1, argv +1);

				return size;
			}
		}
		ret = -EINVAL;
	}

	usage();
	return ret;
}
