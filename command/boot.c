#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <linux.h>
#include <board.h>
#include <fs.h>

#if 0
#include <net/net.h>
#include <net/tftp.h>
#include <uart/uart.h>
#include <mtd/mtd.h>
#include <image.h>

#define KERNEL_MAX_SIZE   (CONFIG_HEAP_SIZE / 4)

// FIXME: for debug stage while no flash driver available
static int build_command_line(char *cmd_line, size_t max_len)
{
	int ret;
	char *str = cmd_line;
	char local_ip[IPV4_STR_LEN], server_ip[IPV4_STR_LEN], net_mask[IPV4_STR_LEN];
	__u32 client_ip, client_mask; // FIXME
	char config[CONF_VAL_LEN];
	const char *console;
	char attr[CONF_ATTR_LEN];

	// FIXME: to be removed
	memset(cmd_line, 0, max_len);

	// set root
	ret = conf_get_attr("linux.root", config);
	if (ret < 0) {
		// TODO: search the root device
		strcpy(config, "mtdblock4");
	}

	if (!strncmp(config, "/dev/", 5))
		strcpy(config, config + 5);

	str += sprintf(str, "root=/dev/%s", config);

	if (!strncmp(config, "nfs", 3)) {
		ret = conf_get_attr("linux.nfsroot", config);
		if (ret < 0) {
			conf_get_attr("net.server", config);
			// add image path
			strcat(config, ":/maxwit/image/rootfs");
		} else {
			// TODO: parse nfsroot config here
		}

		str += sprintf(str, " nfsroot=%s", config);
	} else {
		if (!strncmp(config, "mtdblock", 8)) {
			int fd;
			ssize_t ret;
			image_t type;
			char image_buff[KB(8)], attr[CONF_ATTR_LEN];
			const char *fstype;

			fd = open(config, O_RDONLY);
			if (fd < 0) {
				printf("fail to open block device \"%s\"!\n", config);
				return -ENODEV;
			}

			ret = read(fd, image_buff, sizeof(image_buff));
			assert(ret >= 0);

			close(fd);

			snprintf(attr, sizeof(attr), "%s.fstype", config);
			if (conf_get_attr(attr, config) >= 0) {
				fstype = config;
			} else {
				// FIXME: check it always
				type = image_type_detect(image_buff, sizeof(image_buff));
				switch (type) {
				case IMG_YAFFS1:
					fstype = "yaffs";
					break;

				case IMG_YAFFS2:
					fstype = "yafs2";
					break;

				case IMG_JFFS2:
					fstype = "jffs2";
					break;

				case IMG_UBIFS:
					fstype = "ubifs";
					break;

				default:
					fstype = NULL;
				}
			}

			if (fstype)
				str += sprintf(str, " rootfstype=%s", fstype);
		} else if (!strncmp(config, "mmcblk", 6)) {
			str += sprintf(str, " rootwait");
		}
	}

	// TODO: merge partitions for booting Android
	if (conf_get_attr("flash.parts", config) >= 0)
		str += sprintf(str, " mtdparts=%s", config);

	// set IP
	ret = conf_get_attr("net.server", server_ip);
	// if ret < 0 ...

	const char *ifx = "eth0";

	sprintf(attr, "net.%s.method", ifx);
	ret = conf_get_attr(attr, config);

	if (ret < 0 || !strcmp(config, "dhcp")) {
		str += sprintf(str, " ip=dhcp");
	} else {
		struct net_device *ndev;
		struct list_head *list_head, *iter;

		list_head = ndev_get_list();
		list_for_each(iter, list_head) {
			ndev = container_of(iter, struct net_device, ndev_node);
			if (!strcmp(ndev->ifx_name, ifx))
				break;
		}

		if (iter != list_head) {
			if (ndev_ioctl(ndev, NIOC_GET_IP, &client_ip) == 0)
				ip_to_str(local_ip, client_ip);

			if (ndev_ioctl(ndev, NIOC_GET_MASK, &client_mask) == 0)
				ip_to_str(net_mask, client_mask);
		} else {
			GEN_DBG("Not supportted now!\n");
			// TODO: read from sysconfig
		}

		str += sprintf(str, " ip=%s:%s:%s:%s:maxwit.googlecode.com:eth0:off",
				local_ip, server_ip, server_ip, net_mask);
	}

	// set console
	if (conf_get_attr("linux.console", config) < 0)
		console = "ttyS";
	else
		console = config;

	str += sprintf(str, " console=%s%d\0", console, CONFIG_UART_INDEX);

	ret = str - cmd_line;

	return ret;
}

static ssize_t load_image(void *dst, const char *src)
{
	int ret;
	const char *image = "zImage"; // FIXME!

	if (!strncmp(src, "tftp://", 7)) {
		struct tftp_opt dlopt;

		memset(&dlopt, 0x0, sizeof(dlopt));

		net_get_server_ip((__u32 *)&dlopt.src);
		strcpy(dlopt.file_name, image);
		dlopt.load_addr = dst;

		ret = tftp_download(&dlopt);
		if (ret < 0) {
			printf("fail to download %s!\n", image);
			return ret;
		}
	} else {
#if 0
		if (!strncmp(src, "mtdblock", 8)) {
			struct mtd_info *mtd;

			flash = flash_open(src);
			if (!flash) {
				// ...
				return -ENODEV;
			}

			ret = flash_read(flash, dst, 0, KERNEL_MAX_SIZE /* FIXME */);
			if (ret < 0) {
				GEN_DBG("fail to load kernel image from %s!\n", src);
				return ret;
			}

			flash_close(flash);
		} else {
			printf("file \"%s\" NOT supported now:(\n", src);
			return -EINVAL;
		}
#endif
		int fd;

		fd = open(src, O_RDONLY);
		if (fd < 0) {
			printf("fail to open \"%s\" (ret = %d)!\n", src, fd);
			return fd;
		}

		ret = read(fd, dst, KERNEL_MAX_SIZE /* FIXME */);
		if (ret < 0) {
			GEN_DBG("fail to load kernel image from %s!\n", src);
			return ret;
		}

		close(fd);
	}

	return ret;
}
#endif

static int show_atags(const struct tag *tag)
{
	int i = 0;

	while (ATAG_NONE != tag->hdr.tag) {
		printf("[ATAG %d] ", i);

		switch (tag->hdr.tag) {
		case ATAG_CORE:
			printf("ATAG Begin\n");
			break;

		case ATAG_NONE:
			printf("ATAG End\n");
			break;

		case ATAG_CMDLINE:
			printf("Command Line\n%s\n", tag->u.cmdline.cmdline);
			break;

		case ATAG_MEM:
			printf("Memory\n(0x%08x, 0x%08x)\n",
				tag->u.mem.start, tag->u.mem.size);
			break;

		case ATAG_INITRD2:
			printf("Initrd\n");
			if (tag->u.initrd.size)
				printf("(0x%08x, 0x%08x)\n",
					tag->u.initrd.start, tag->u.initrd.size);
			else
				printf("%s (to be loaded)\n", (char *)tag->u.initrd.start);
			break;

		default:
			printf("Invalid ATAG 0x%08x @ 0x%08x!\n", tag->hdr.tag, tag);
			return -EINVAL;
		}

		tag = tag_next(tag);
		i++;
	}

	return 0;
}

static inline int prepare()
{
	irq_disable();

	// TODO: add code here

	return 0;
}

#if 0
int main(int argc, char *argv[])
{
	int ret = -EIO, opt;
	enum {BA_SLIENT, BA_STOP, BA_VERBOSE} verbose = BA_SLIENT;
	char config[CONF_VAL_LEN], cmd_line[512];
	void *initrd;
	LINUX_KERNEL_ENTRY linux_kernel;
	const struct board_desc *board;
	struct tag *tag;

	while ((opt = getopt(argc, argv, "v::h")) != -1) {
		switch (opt) {
		case 'v':
			if (optarg)
				verbose = BA_VERBOSE;
			else
				verbose = BA_STOP;
			break;

		default:
			printf("Invalid option \'%c\'\n", opt);
		case 'h':
			usage();
			return 0;
		}
	}

	board = board_get_active();
	if (!board) {
		printf("not active board found!\n");
		return -ENODEV;
	}

	if (verbose != BA_SLIENT)
		printf("Board: name = \"%s\", ID = %d (0x%x)\n",
			board->name, board->mach_id, board->mach_id);

	tag = begin_setup_atag(VA(ATAG_BASE));

	// parse command line
	ret = conf_get_attr("linux.cmdline", config);
	if (ret < 0 || !strcmp(config, "auto"))
		build_command_line(cmd_line, sizeof(cmd_line));
	else
		strcpy(cmd_line, config); // FIXME: strncpy()

	tag = setup_cmdline_atag(tag, cmd_line);

	// setup mem tag
	if (strstr(cmd_line, "mem=") == NULL)
		tag = setup_mem_atag(tag);

	// load initrd
	ret = conf_get_attr("linux.initrd", config);
	if (ret >= 0) {
		if (BA_STOP == verbose) {
			tag = setup_initrd_atag(tag, config, 0);
		} else {
			initrd = malloc(MB(8)); // FIXME
			if (!initrd) {
				ret = -ENOMEM;
				goto error;
			}

			ret = load_image(initrd, config);
			if (ret <= 0) {
				// BA_STOP == verbose args;
				return ret;
			}

			// TODO: check the initrd.img
			tag = setup_initrd_atag(tag, initrd, ret);
		}
	}

	// tag end
	end_setup_atag(tag);

	if (verbose != BA_SLIENT) {
		show_atags(VA(ATAG_BASE));
		if (BA_STOP == verbose)
			return 0;
	}

	// load kernel image
	linux_kernel = malloc(KERNEL_MAX_SIZE);
	if (!linux_kernel) {
		ret = -ENOMEM;
		goto error;
	}

	ret = conf_get_attr("linux.kernel", config);
	if (ret < 0) {
		// set to default
		strcpy(config, "mtdblock3"); // FIXME
	}

	printf("Loading Linux kernel from %s to %p ...\n", config, linux_kernel);
	ret = load_image(linux_kernel, config);
	if (ret < 0) {
		printf("failed to load kernel!\n");
		goto error;
	}

	if (image_type_detect(linux_kernel, KERNEL_MAX_SIZE) != IMG_LINUX)
		printf("Warning: invalid linux kernel image!\n");
	printf("Done! 0x%08x bytes loaded.\n", ret);

	prepare();

	linux_kernel(0, board->mach_id, ATAG_BASE);

error:
	GEN_DBG("boot failed! error = %d\n", ret);
	return ret;
}
#else
int main(int argc, char *argv[])
{
	int img_fd;
	int ret, opt;
	struct stat st;
	ssize_t size, len = 0;
	const char *img_fn = "/data/boot/zImage"; // FIXME
	LINUX_KERNEL_ENTRY linux_kernel;
	const struct board_desc *board;
	struct tag *tag, *const tag_base = VA(ATAG_BASE);
	char cmd_line[1024], *p = cmd_line;
	const char *nfs = NULL;

	while ((opt = getopt(argc, argv, "n::h")) != -1) {
		switch (opt) {
		case 'n': // -n [[<ip>][:<path>]]
			if (optarg)
				nfs = optarg;
			else
				nfs = "192.168.0.106"; // FIXME;
			break;

		default:
			printf("Invalid option \'%c\'\n", opt);
		case 'h':
			usage();
			return 0;
		}
	}

	linux_kernel = (LINUX_KERNEL_ENTRY)(SDRAM_BASE + 0x8000);

	board = board_get_active();
	if (!board) {
		printf("not active board found!\n");
		return -ENODEV;
	}

	printf("%s: board = \"%s\" (id = %d)\n",
		argv[0], board->name, board->mach_id);

	// setup atags
	tag = begin_setup_atag(tag_base);

	p += sprintf(p, "console=ttyO%d", CONFIG_UART_INDEX);

	if (nfs) {
		p += sprintf(p, " root=/dev/nfs rw"
					" nfsroot=%s:/maxwit/image/rootfs", nfs);

		if (1) // FIXME
			p += sprintf(p, " ip=dhcp");
		else
			p += sprintf(p, " ip=192.168.0.6:%s:%s:255.255.255.0"
					":maxwit.com:eth0:off", nfs, nfs);
	} else {
		p += sprintf(p, " root=/dev/mmcblk0p2 rootwait");
	}

	tag = setup_cmdline_atag(tag, cmd_line);

	tag = setup_mem_atag(tag);

	end_setup_atag(tag);

	show_atags(tag_base);

	// load the kernel now!
	img_fd = open(img_fn, O_RDONLY);
	if (img_fd < 0) {
		printf("fail to open %s\n", img_fn);
		return img_fd;
	}

	ret = fstat(img_fd, &st);
	if (ret < 0) {
		printf("%s line %d\n", __FILE__, __LINE__);
		close(img_fd);
		return ret;
	}

	// ...
	while (len < st.st_size) {
		size = read(img_fd, (void *)linux_kernel + len, KB(8));
		if (size <= 0) {
			if (size < 0) {
				printf("%s line %d\n", __FILE__, __LINE__);
				close(img_fd);
				return size;
			}

			break;
		}

		len += size;
		printf("loading %s ... %d of %d bytes\r", img_fn, len, st.st_size);
	}

	if (len > 0)
		printf("\n");

	close(img_fd);

	// boot!
	prepare();
	linux_kernel(0, board->mach_id, (unsigned long)tag_base);

	return -ENOEXEC;
}
#endif
