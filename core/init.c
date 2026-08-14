#include <stdio.h>
#include <init.h>
#include <errno.h>
#include <delay.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h> // for mount(), FIXME
// FIXME: to be removed!
#include <syscalls.h>
#include <shell.h>
#include <uart/uart.h>
#include <font/font.h>
#include <fs.h> // FIXME: to be removed

static const char banner[] = "\n\n" // CLRSCREEN
	"\t+---------------------------------+\n"
	"\t|    Welcome to MaxWit witOS!    |\n"
	"\t|      http://www.maxwit.com      |\n"
	"\t|      "__DATE__", "__TIME__"      |\n"
	"\t+---------------------------------+\n"
	"\nwitOS running "
#ifdef CONFIG_IRQ_SUPPORT
	"with IRQ enabled!\n"
#else
	"in polling mode!\n"
#endif
	;

static int __init system_init(void)
{
	int count;
	init_func_t *init_call;
	extern init_func_t init_call_begin[], init_call_end[];

	count = 1;
	init_call = init_call_begin;

	while (init_call < init_call_end) {
		int ret;
		const char *func_name;

		printf("%d. [0x%08x]", count, *init_call);
		func_name = get_func_name(*init_call);
		if (func_name)
			printf(" %s()", func_name);
		putchar('\n');

		ret = (*init_call)();
		if (ret < 0)
			puts("Failed!");
		else
			puts("OK!");

		init_call++;
		count++;

		printf("\n");
	}

	printf("(witOS initialization finished.)\n");

	return 0;
}

static void __init auto_boot(void)
{
	int time_out = 3;
	char *argv[] = {"boot"};

	while (1) {
		int index;

		printf("\rAutoboot after %d seconds. "
			"Press any key to interrupt.", time_out);

		if (0 == time_out)
			break;

		for (index = 0; index < 10; index++) {
			mdelay(100);

			if (uart_rxbuf_count()) {
				puts("\n");
				return;
			}
		}

		time_out--;
	}

	puts("\n");

	exec(ARRAY_ELEM_NUM(argv), argv);
}

int __init mount_root(const char *dev_name, const char *type,
	unsigned long flags);

static int __init populate_rootfs()
{
	int i, ret;
	const char *device;
	unsigned long flags;
	// FIXME
	const char *fstab[][3] = {{"none", "/dev", "devfs"},
						// {"mmcblk0p1", "/boot", "vfat"},
						{"mmcblk0p2", "/data", "ext4"}
						};

	ret = mount_root(NULL, "ramfs", MS_NODEV);
	if (ret < 0) {
		printf("Fetal error: fail to mount rootfs! (error = %d)\n", ret);
		return ret;
	}

	for (i = 0; i < ARRAY_ELEM_NUM(fstab); i++) {
		ret = sys_mkdir(fstab[i][1], 0755);
		if (ret < 0) {
			printf("fail to create %s! error = %d\n", fstab[i][1], ret);
			goto L1;
		}

		flags = 0;
		device = fstab[i][0];
		if (!strcmp(fstab[i][0], "none")) {
			device = NULL;
			flags |= MS_NODEV;
		}

		ret = sys_mount(device, fstab[i][1], fstab[i][2], flags);
		if (ret < 0) {
			printf("Warning: \"mount %s %s %s\" failed (error = %d)\n",
				fstab[i][0], fstab[i][1], fstab[i][2], ret);
		}
	}

	return 0;

L1:
	return ret;
}

int main(void)
{
	int ret;

	ret = conf_check();
	if (ret < 0) {
		printf("Warning: fail to initialize system configuration!\n");
		return ret;
	}

	ret = system_init();
	if (ret < 0)
		return ret;

	ret = populate_rootfs();
	if (ret < 0)
		return ret;

	// TODO: show more information of system
	printf("%s\n", banner);

	auto_boot();

	while (1) {
		printf("\nEnter witOS Shell.\n");
		shell();
	}

	return -EINVAL;
}
