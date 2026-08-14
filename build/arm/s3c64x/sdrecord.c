#ifndef __x86_64__
#define _FILE_OFFSET_BITS 64
#endif

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

#define BUF_LEN 512
#define SZ_KB(n) ((n) << 10)
#define SZ_GB(n) ((n) << 30)
#define MAX_SIZE SZ_KB(8)

int main(int argc, char *argv[])
{
	int i, ret;
	off_t size, resv_kb;
	ssize_t len;
	unsigned char buff[BUF_LEN];
	int fd_mmc, fd_img;
	const char *fn_mmc, *fn_img;

	if (argc != 3) {
		printf("usage: xxx\n");
		return -EINVAL;
	}

	fn_mmc = argv[1];
	fn_img = argv[2];

	fd_mmc = open(fn_mmc, O_RDWR);
	if (fd_mmc < 0) {
		printf("fail to open %s!\n", fn_mmc);
		return fd_mmc;
	}

	fd_img = open(fn_img, O_RDONLY);
	if (fd_img < 0) {
		printf("fail to open %s!\n", fn_img);
		ret = -EIO;
		goto L1;
	}

	size = lseek(fd_mmc, 0, SEEK_END);

	if (size < (2UL << 30))
		resv_kb = 1;
	else
		resv_kb = 513;

#ifndef __x86_64__
	printf("%s: size = %lld Bytes(%lld MB), reserved = %lld KB\n",
		fn_mmc, size, size >> 20, resv_kb);
#else
	printf("%s: size = %ld Bytes(%ld MB), reserved = %ld KB\n",
		fn_mmc, size, size >> 20, resv_kb);
#endif

	size = lseek(fd_mmc, -(MAX_SIZE + SZ_KB(resv_kb)), SEEK_END);

	for (i = 0; i < MAX_SIZE;) {
		len = read(fd_img, buff, BUF_LEN);
		if (len < 0)
			goto L2;

		ret = write(fd_mmc, buff, len);
		if (ret < 0)
			goto L2;

		i += BUF_LEN;

		if (len < BUF_LEN)
			break;
		// FIXME
	}

	fsync(fd_mmc);

#ifndef __x86_64__
	printf("%d bytes written!\n", i - BUF_LEN + len);
#else
	printf("%lu bytes written!\n", i - BUF_LEN + len);
#endif

L2:
	close(fd_img);
L1:
	close(fd_mmc);

	return ret;
}
