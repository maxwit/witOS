#include <errno.h>
#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <mtd/mtd.h>

static int flash_str_to_val(char * str, __u32 * val, char *unit)
{
	int len;
	char *p = str;

	len = strlen(str);

	if (len > 5 && 0 == strncmp(p + len - 5, "block", 5)) {
		*unit = 'b'; // block
		p[len - 5] = '\0';
	} else if (len > 4 && 0 == strncmp(p + len - 4, "page", 4)) {
		*unit = 'p'; // page
		p[len - 4] = '\0';
	} else if (len > 1 && strchr("kKmMgG", str[len - 1])) {
		return hr_str_to_val(str, (unsigned long *)val);
	}

	return str_to_val(str, (unsigned long *)val);
}

static int info(int argc, char *argv[])
{
	int ret, fd;
	struct part_attr part;
	const char *dev;

	dev = get_current_dir_name();
	fd = open(dev, O_RDONLY);

	ret = ioctl(fd, 0 /*FIXME*/, &part);
	if (ret < 0) {
		printf("fail to get parition info! (ret = %d)\n", ret);
		// return ret;
	}

	close(fd);

	printf(
		"label      %s\n"
		"flash:     %s\n"
		"device:    %s\n"
		"base:      0x%08X\n"
		"size:      0x%08X\n"
		"pagesize:  0x%08X\n"
		"blocksize: 0x%08x\n",
		part.label,
		"??",
		"??", // part.name,
		part.base,
		part.size,
		0, // write_size,
		0 // erase_size
		);

	return 0;
}

static int read_write(int argc, char *argv[])
{
	int ch, ret = 0, flag = 0;
	__u32 start = 0, size = 1024;
	char start_unit = 0, size_unit = 0;
	void *buff = NULL;
	int fd;
	struct flash_info flash_val;
	char bdev_name[BLOCK_DEV_NAME_LEN];

	while ((ch = getopt(argc, argv, "a:l:m:h")) != -1) {
		switch (ch) {
		case 'a':
			if (flag || flash_str_to_val(optarg, &start, &start_unit) < 0) {
				printf("Invalid argument: \"%s\"\n", optarg);
				usage();

				return -EINVAL;
			}

			flag++;

			break;

		case 'l':
			if (flash_str_to_val(optarg, &size, &size_unit) < 0) {
				printf("Invalid argument: \"%s\"\n", optarg);
				return -EINVAL;
			}

			flag++;

			break;

		case 'm':
			if (str_to_val(optarg, (unsigned long *)&buff) < 0) {
				printf("Invalid argument: \"%s\"\n", optarg);
				usage();
				return -EINVAL;
			}

			flag++;

			break;

		default:
			ret = -EINVAL;
		case 'h':
			usage();
			return ret;
		}
	}

	// must set -a flash_addr -l size -m mem_addr
	if (flag != 3) {
		printf("Please set the option -a addr -l size -m address>\n");
		usage();
		return -EINVAL;
	}

	// FIXME
	getcwd(bdev_name, sizeof(bdev_name));
	fd = open(bdev_name, O_RDWR);
	if (fd < 0) {
		goto L1;
	}

	ret = ioctl(fd, FLASH_IOCG_INFO, &flash_val);
	if (ret < 0) {
		goto L2;
	}

	// -a xxxblock or -a xxxpage
	if (start_unit == 'b') {
		start *= flash_val.block_size;
	} else if (start_unit == 'p') {
		start *= flash_val.page_size;
	}

	// -l xxxblock or -l xxxpage
	if (size_unit == 'b') {
		size *= flash_val.block_size;
	} else if (size_unit == 'p') {
		size *= flash_val.page_size;
	}

	if (start + size >= flash_val.bdev_size) {
		printf("Address 0x%08x overflow!\n", start + size);
		ret = -EINVAL;
		goto L1;
	}

	lseek(fd, start, SEEK_SET);
	if (0 == strcmp(argv[0], "read")) {
		ret = read(fd, buff, size);
		if (ret < 0) {
			printf("please check argument!\n");
			usage();

			ret = -EINVAL;
			goto L2;
		}
		printf("Read 0x%08x bytes data to mem 0x%08x from flash 0x%08x\n", size, (__u32)buff, start);
	} else {
		ret = write(fd, buff, size);
		if (ret < 0) {
			printf("please check argument!\n");
			usage();

			ret = -EINVAL;
			goto L2;
		}
		printf("write 0x%08x bytes data to flash 0x%08x from mem 0x%08x\n", size, start, (__u32)buff);
	}

L2:
	close(fd);
L1:
	return ret;
}

#if 0
/* part erase */
struct flash_parterase_param {
	FLASH_HOOK_PARAM hParam;
	struct process_bar *pBar;
};

static int flash_parterase_process(struct mtd_info *mtd, FLASH_HOOK_PARAM *pParam)
{
	struct flash_parterase_param *pErInfo;

	pErInfo = container_of(pParam, struct flash_parterase_param, hParam);

	progress_bar_set_val(pErInfo->pBar, pParam->nBlockIndex);

	return 0;
}
#endif
/* scan bb */
// TODO: add process bar
static int scanbb(int argc, char *argv[])
{
	int ret;
	__u32 part_num = -1;
	int ch;
	int fd;
	char bdev_name[BLOCK_DEV_NAME_LEN];
	struct flash_info flash_val;

	while ((ch = getopt(argc, argv, "p:")) != -1) {
		switch (ch) {
		case 'p':
			if (str_to_val(optarg, (unsigned long *)&part_num) < 0) {
				printf("Invalid argument: \"%s\"\n", optarg);
				return -EINVAL;
			}

			break;

		default:
			usage();
			return -EINVAL;
		}
	}

	if (part_num != -1) {
		snprintf(bdev_name, sizeof(bdev_name), "mtdblock%d", part_num);
	} else {
		getcwd(bdev_name, sizeof(bdev_name));
	}

	fd = open(bdev_name, O_RDONLY);
	if (fd < 0) {
		ret = fd;
		goto L1;
	}

	ret = ioctl(fd, FLASH_IOCG_INFO, &flash_val);
	if (ret < 0) {
		goto L2;
	}

	if (MTD_NANDFLASH != flash_val.type) {
		goto L2;
	}

	ret = ioctl(fd, FLASH_IOC_SCANBB);
	if (ret < 0) {
		goto L2;
	}

L2:
	close(fd);
L1:
	return ret;
}

static int dump(int argc, char *argv[])
{
	__u8 *p, *buff;
	int ch;
	int ret = 0;
	int flag  = 0;
	__u32 start = 0;
	int size = 0;
	char start_unit = 0;
	char size_unit = 0;
	int fd;
	struct flash_info flash_val;
	char bdev_name[BLOCK_DEV_NAME_LEN];

	while ((ch = getopt(argc, argv, "p:a:l:h")) != -1) {
		switch (ch) {
		case 'a':
			if (flag || (flash_str_to_val(optarg, &start, &start_unit) < 0)) {
				printf("Invalid argument: \"%s\"\n", optarg);
				usage();
				return -EINVAL;
			}

			flag = 1;

			break;

		case 'l':
			if (flag == 2 || flash_str_to_val(optarg, (__u32 *)&size, &size_unit) < 0) {
				printf("Invalid argument: \"%s\"\n", optarg);
				usage();
				return -EINVAL;
			}

			break;


		default:
			ret = -EINVAL;
		case 'h':
			usage();
			return ret;
		}
	}

	getcwd(bdev_name, sizeof(bdev_name));
	fd = open(bdev_name, O_RDONLY);
	if (fd < 0) {
		ret = fd;
		goto L1;
	}

	ret = ioctl(fd, FLASH_IOCG_INFO, &flash_val);
	if (ret < 0) {
		goto L2;
	}

	// -a xxxblock or -a xxxpage
	if (start_unit == 'b') {
		start *= flash_val.block_size;
	} else if (start_unit == 'p') {
		start *= flash_val.page_size;
	}

	// -l xxxblock or -l xxxpage
	if (size_unit == 'b') {
		size *= flash_val.block_size;
	} else if (size_unit == 'p') {
		size *= flash_val.page_size;
	}

	if (size == 0)
		size = flash_val.page_size + flash_val.oob_size;

	ALIGN_UP(size, flash_val.page_size + flash_val.oob_size);

	if (start + size >= flash_val.bdev_size) {
		printf("Address 0x%08x overflow!\n", start);
		ret = -EINVAL;
		goto L2;
	}

	buff = (__u8 *)malloc(size);
	if (NULL == buff) {
		ret = -ENOMEM;
		goto L3;
	}

	start &= ~(flash_val.page_size - 1);

	ret = ioctl(fd, FLASH_IOCS_OOB_MODE, (void *)FLASH_OOB_RAW);
	if (ret < 0) {
		goto L3;
	}

	ret = lseek(fd, start, SEEK_SET);
	if (ret < 0) {
		goto L3;
	}

	ret = read(fd, buff, size);
	if (ret < 0) {
		printf("%s(): line %d execute read_raw() error!\n"
			"error = %d\n", __func__, __LINE__, ret);

		goto L3;
	}

	DPRINT("%s 0x%08x (flash 0x%08x) ==> RAM 0x%08x, "
		"Expected length 0x%08x, Real length 0x%08x\n",
		flash_val.name, start, flash_val.bdev_base + start, buff, size, ret);

	p = buff;

	while (size > 0) {
		int i;

		printf("Page @ 0x%08x:\n", start);

		i = flash_val.page_size >> 4;

		while (i--) {
			printf( "\t%02x %02x %02x %02x %02x %02x %02x %02x"
				"  %02x %02x %02x %02x %02x %02x %02x %02x\n",
				p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7],
				p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);

			p += 16;
		}

		printf("OOB @ 0x%08x + 0x%08x:\n", start, flash_val.page_size);

		i = flash_val.oob_size >> 4;

		while (i--) {
			printf( "\t%02x %02x %02x %02x %02x %02x %02x %02x"
				"  %02x %02x %02x %02x %02x %02x %02x %02x\n",
				p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7],
				p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);

			p += 16;
		}

		start += flash_val.page_size + flash_val.oob_size;
		size  -= flash_val.page_size + flash_val.oob_size;

		puts("\n");
	}

L3:
	free(buff);
L2:
	close(fd);
L1:
	return ret;
}


// FIXME: bad logic!
static int erase(int argc, char *argv[])
{
	int ch;
	int arg_flag = 0;
	int ret = 0;
	__u32 start = 0;
	__u32 size = 0;
	__u32 dev_num = 0;
	char start_unit = 0;
	char size_unit = 0;
	__u32 erase_flags = EDF_NORMAL;
	int fd;
	struct flash_info flash_val;
	struct erase_info eopt;
	char bdev_name[BLOCK_DEV_NAME_LEN];

	while ((ch = getopt(argc, argv, "a:l:p::c:f")) != -1) {
		switch(ch) {
		case 'a':
			if (arg_flag == 2 || flash_str_to_val(optarg, &start, &start_unit) < 0) {
				printf("Invalid argument: \"%s\"\n", optarg);
				usage();
				return -EINVAL;
			}

			arg_flag = 1;

			break;

		case 'l':
			if (arg_flag == 2 || flash_str_to_val(optarg, &size, &size_unit) < 0) {
				printf("Invalid argument: \"%s\"\n", optarg);
				usage();
				return -EINVAL;
			}

			break;

		case 'p':
			if (arg_flag == 1 || (optarg && str_to_val(optarg, (unsigned long *)&dev_num) < 0)) {
				printf("Invalid argument: \"%s\"\n", optarg);
				usage();
				return -EINVAL;
			}

			arg_flag = 2;

			break;

		case 'c':
			erase_flags |= EDF_JFFS2;
			break;

		case 'f':
			erase_flags |= EDF_ALLOWBB;
			break;

		default:
			ret = -EINVAL;
		case 'h':
			usage();
			return ret;
		}
	}

	getcwd(bdev_name, sizeof(bdev_name));

	fd = open(bdev_name, O_WRONLY);
	if (fd < 0) {
		ret = fd;
		goto L1;
	}

	ret = ioctl(fd, FLASH_IOCG_INFO, &flash_val);
	if (ret < 0) {
		goto L2;
	}

	// if option is "-p" erase the whole partiton or default current partition with no option
	if (0 == size) {
		size = flash_val.bdev_size;
	} else {
		// -a xxxblock or -a xxxpage
		if (start_unit == 'b') {
			start *= flash_val.block_size;
		} else if (start_unit == 'p') {
			start *= flash_val.page_size;
		}

		// -l xxxblock or -l xxxpage
		if (size_unit == 'b') {
			size *= flash_val.block_size;
		} else if (size_unit == 'p') {
			size *= flash_val.page_size;
		}
	}

	if (start + size > flash_val.bdev_size) {
		printf("Out of chip size!\n");
		return -EINVAL;
	}

	//aligned:
	ALIGN_UP(start, flash_val.page_size);
	ALIGN_UP(size, flash_val.block_size);

	printf("[0x%08x : 0x%08x]\n", start, size);

	memset(&eopt, 0, sizeof(eopt));
	eopt.len = size;
	eopt.addr = start;
	eopt.flags = erase_flags;

	ret = ioctl(fd, FLASH_IOC_ERASE, &eopt);
	if (ret < 0) {
		goto L2;
	}

L2:
	close(fd);

L1:
	return ret;
}

int main(int argc, char *argv[])
{
	int ret = 0, i;

	struct command cmd[] = {
		{
			.name = "info",
			.main = info
		}, {
			.name = "dump",
			.main = dump
		}, {
			.name = "erase",
			.main = erase
		}, {
			.name = "read",
			.main = read_write
		}, {
			.name = "write",
			.main = read_write
		}, {
			.name = "scanbb",
			.main = scanbb
		},
	};

	if (1 == argc) {
		usage();
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_ELEM_NUM(cmd); i++) {
		if (0 == strcmp(argv[1], cmd[i].name)) {
			ret = cmd[i].main(argc - 1, argv + 1);
			break;
		}
	}

	return ret;
}
