#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <net/net.h>
#include <net/tftp.h>
#include <mtd/mtd.h>

#define PORT_LEN 6
#define IS_ALPHBIT(c) (((c) >= 'a' && (c) <= 'z') || \
			((c) >= 'A' && (c) <= 'Z'))

struct sub_cmd_info {
	const char *name;
	int (*cmd)(int, char *[]);
};

static int tftp_get_file(int argc, char **argv);
static int tftp_put_file(int argc, char **argv);

#if 0
static int port_atoi(const char *str)
{
	const char *iter;
	int interger = 0;
	int temp;

	for (iter = str; *iter >= '0' && *iter <= '9'; iter++) {
		temp = *iter - '0';
		interger = interger * 10 + temp;
	}

	return interger;
}
#endif

/*
url:
1. tftp://ip/fn
2. ip/fn
3. ip
4. fn

 */
static int parse_url(const char *url, char * const ip, char * const fn)
{
	const char *end, *start;

	end = strstr(url, "://");
	if (end != NULL) {
		if (strcmp(url, "tftp://"))
			return -EINVAL;

		end += 3;
	} else {
		end = url;
	}

	start = end;

	end = strchr(start, '/');
	if (end == NULL) {
		*fn = '\0';
		strcpy(ip, start);
	} else {
		strncpy(ip, start, end - start);
		ip[end - start] = '\0';

		end++;
		strcpy(fn, end);
	}

	return 0;
}

int main(int argc, char *argv[])
{
	int i;
	int ret;
	struct sub_cmd_info cmd_info[] = {
		{
			.name = "get",
			.cmd   = tftp_get_file,
		},
		{
			.name = "put",
			.cmd   = tftp_put_file,
		},
	};

	if (argc >= 2) {
		for (i = 0; i < ARRAY_ELEM_NUM(cmd_info); i++) {
			if (0 == strcmp(argv[1], cmd_info[i].name)) {
				ret = cmd_info[i].cmd(argc - 1, argv + 1);
				return ret;
			}
		}
	}

	ret = tftp_get_file(argc, argv);

	return ret;
}
static int tftp_get_file(int argc, char **argv)
{
	int ret, ch;
	bool mem_only = false;
	struct tftp_opt dlopt;
	char ip[IPV4_STR_LEN];
	const char *url = NULL;
	char conf_attr[CONF_ATTR_LEN], conf_val[CONF_VAL_LEN];
	const char *src = NULL;

	memset(&dlopt, 0x0, sizeof(dlopt));
	net_get_server_ip((__u32 *)&dlopt.src);

	while ((ch = getopt(argc, argv, "a:m:r:l:t:v:")) != -1) {
		switch(ch) {
		case 'a':
			ret = str_to_val(optarg, (unsigned long *)&dlopt.load_addr);
			if (ret < 0) {
				usage();
				return ret;
			}

			mem_only = true;
			break;

		case 'm':
			if (strcmp(optarg, "octet") == 0 || strcmp(optarg, "netacsii") == 0) {
				strncpy(dlopt.mode, optarg, MAX_MODE_LEN);
				dlopt.mode[MAX_MODE_LEN - 1] = '\0';
			} else {
				printf("Invalid mode: %s\n", optarg);
				return -EINVAL;
			}
			break;

		case 'l':
			if (IS_ALPHBIT(optarg[0]) && \
			('\0' == optarg[1] || (':' == optarg[1] && '\0' == optarg[2]))) {
				dlopt.dst = optarg;
			} else {
				usage();
				return -EINVAL;
			};
			break;
		case 'r':
			url = optarg;
			break;

		case 't':
			dlopt.type = optarg;
			break;

		case 'v':
			dlopt.verbose = true;
			break;


		case 'h':
		default:
			usage();
			return -EINVAL;
		}
	}

	if (optind < argc) {
		if (optind + 1 == argc) {
			url = argv[optind];
		} else {
			usage();
			return -EINVAL;
		}
	}

	if (url) {
		ret = parse_url(url, ip, dlopt.file_name);
		if (ret < 0) {
			printf("Invalid URL: \"%s\"\n", url);
			return -ret;
		}

		if (ip[0] != '\0')
			src = ip;
	}

	if (!src) {
		if (conf_get_attr("net.server", ip) < 0) {
			printf("Please set the server address!\n");
			return -EINVAL;
		}

		src = ip;
	}

	if (!dlopt.file_name[0]) {
		dlopt.dst = get_current_dir_name();
		snprintf(conf_attr, CONF_ATTR_LEN, "bdev.%s.image.name", dlopt.dst);
		if (conf_get_attr(conf_attr, conf_val) < 0) {
			printf("Please sepcify the filename!\n");
			return -EINVAL;
		}

		strncpy(dlopt.file_name, conf_val, sizeof(dlopt.file_name));
	}

	if (mem_only == false) {
		if (!dlopt.dst) {
			dlopt.dst = get_current_dir_name();
		}
	}

	dlopt.src = src;

	ret = tftp_download(&dlopt);
	if (ret < 0) {
		printf("fail to download \"%s\" (ret = %d)!\n", dlopt.file_name, ret);
		return ret;
	}

	// FIXME: check file type
	if (dlopt.dst) {
		// set file name
		snprintf(conf_attr, CONF_ATTR_LEN, "bdev.%s.image.name", dlopt.dst);
		if (conf_set_attr(conf_attr, dlopt.file_name) < 0) {
			conf_add_attr(conf_attr, dlopt.file_name);
		}

		// set file size
		snprintf(conf_attr, CONF_ATTR_LEN, "bdev.%s.image.size", dlopt.dst);
		val_to_dec_str(conf_val, dlopt.xmit_size);
		if (conf_set_attr(conf_attr, conf_val) < 0) {
			conf_add_attr(conf_attr, conf_val);
		}
	}

	// if (false == mem_only) conf_store();

	return ret;
}

static int tftp_put_file(int argc, char **argv)
{
	int ret, ch;
	struct tftp_opt opt;
	char ip[IPV4_STR_LEN];
#if 0
	struct block_device *cur_bdev;
#endif

	memset(&opt, 0x0, sizeof(opt));

	// FIXME
	while ((ch = getopt(argc, argv, "a:m:r:l:t:v:")) != -1) {
		switch(ch) {
		case 'a':
			ret = str_to_val(optarg, (unsigned long *)&opt.load_addr);

			if (ret < 0) {
				usage();
				return ret;
			}
			break;

		case 'm':
			if (strcmp(optarg, "octet") == 0 || strcmp(optarg, "netacsii") == 0) {
				strncpy(opt.mode, optarg, MAX_MODE_LEN);
				opt.mode[MAX_MODE_LEN - 1] = '\0';
			} else {
				printf("Invalid mode: %s\n", optarg);
				return -EINVAL;
			}
			break;

		case 'l':
			if (IS_ALPHBIT(optarg[0]) && \
			('\0' == optarg[1] || (':' == optarg[1] && '\0' == optarg[2]))) {
				opt.src = optarg;
			} else {
				usage();
				return -EINVAL;
			};
			break;

		case 'r':
			ret = parse_url(optarg, ip, opt.file_name);
			if (ret < 0) {
				usage();
				return -ret;
			}
			str_to_ip((__u8 *)&opt.dst, ip);
			break;

		case 't':
			opt.type = optarg;
			break;

		case 'v':
			opt.verbose = true;
			break;


		case 'h':
		default:
			usage();
			return -EINVAL;
		}
	}

	if (!opt.dst)
		net_get_server_ip((__u32 *)&opt.dst);

	if (optind < argc) {
		if (optind + 1 == argc && !opt.file_name[0]) {
			strncpy(opt.file_name, argv[optind], FILE_NAME_SIZE);
			opt.file_name[FILE_NAME_SIZE - 1] = '\0';
		} else {
			usage();
			return -EINVAL;
		}
	}

	ret = tftp_upload(&opt);

	return ret;
}
