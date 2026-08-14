#include <go.h>
#include <loader.h>
#include <stdio.h>
#include <string.h>
#include <fs.h>
#include <image.h>
#include <fcntl.h>
#include <mtd/mtd.h>
#include <uart/uart.h>
#include <uart/ymodem.h>

#define SOH    0x01
#define STX    0x02
#define EOT    0x04
#define ACK    0x06
#define NAK    0x15
#define CAN    0x18

#define MODEM_TIMEOUT  (UART_DELAY * 8)

// FIXME: reset fifo
static void uart_clear_buff()
{
	__u8  tmp;
	int ret;

	while (1) {
		ret = uart_recv_byte_timeout(&tmp, MODEM_TIMEOUT);

		if (ret < 0)
			break;
	}
}

// TODO:  support MS hyper-term
static inline void modem_end_rx()
{
	uart_send_byte(ACK);
	// uart_send_byte('C');
	// uart_send_byte(ACK);

	uart_send_byte(CAN);
	uart_send_byte(CAN);
	uart_send_byte(CAN);
	uart_send_byte(CAN);
	uart_send_byte(CAN);
	uart_send_byte(CAN);
}

int ymodem_load(struct loader_opt *opt)
{
	int ret;
	int size = 0, count;
	char blk = 0;
	__u8 stx, blk_num[2], crc[2];
	__u8 *curr_addr;
	int fd_bdev = 0;
	image_t img_type = IMG_MAX;

	if (!opt->load_addr) {
		__u8 data[1024];

		curr_addr = data;
	}
	else
	{
		curr_addr = opt->load_addr;
	}

	go_set_addr((__u32 *)curr_addr);

	opt->load_size = 0;
	if (opt->dst) {
		fd_bdev = open(opt->dst, O_WRONLY);
		if (fd_bdev < 0)
			return fd_bdev;
	}

	while (1) {
		uart_send_byte('C');

		ret = uart_recv_byte_timeout(&stx, 2000000);

		if (ret < 0)
			continue;

		if (SOH == stx) {
			// printf("SOH (%d)\n", stx);
			size = 128;
			break;
		} else if (STX == stx) {
			// printf("STX (%d)\n", stx);
			size = 1024;
			break;
		}
	}

	blk_num[0] = uart_recv_byte();
	blk_num[1] = uart_recv_byte();

	for (count = 0; count < size; count++) {
		ret = uart_recv_byte_timeout((__u8 *)opt->file_name + count, MODEM_TIMEOUT);

		if (ret == 0) {
			if ('\0' == opt->file_name[count])
				break;
		}
#ifdef CONFIG_DEBUG
		else {
			printf("line %d: timeout!\n", __LINE__);
		}
#endif
	}

#ifdef CONFIG_DEBUG
	printf("loading \'%s\'\n", opt->file_name);
#endif

	uart_clear_buff();
	uart_send_byte(ACK);

	// receiving data
	uart_send_byte('C');

	blk++;
	while (1) {
		while (1) {
			ret = uart_recv_byte_timeout(&stx, MODEM_TIMEOUT);

			if (ret < 0)
				continue;

			if (SOH == stx) {
				// printf("SOH (%d)\n", stx);
				size = 128;
				break;
			} else if (STX == stx) {
				// printf("STX (%d)\n", stx);
				size = 1024;
				break;
			} else if (EOT == stx) {
				// printf("Done!\n");
				goto L1;
			}
		}

		blk_num[0] = uart_recv_byte();
		blk_num[1] = uart_recv_byte();
		if ((blk_num[0] ^ blk_num[1]) != (__u8)0xFF ||
			blk_num[0] != (__u8)(blk & 0xFF)) {
			uart_clear_buff();
			uart_send_byte(NAK);

			printf("%d,%d,%d\n",blk_num[0], blk_num[1], blk);

#ifdef CONFIG_DEBUG
			//printf("block num (%d) error!\n", blk);
#endif
			continue;
		}

		for (count = 0; count < size; count++) {
			ret = uart_recv_byte_timeout(curr_addr + count, MODEM_TIMEOUT);

			if (ret < 0) {
				printf("line %d: timeout.\n");
				break;
			}
		}

		ret = uart_recv_byte_timeout(&crc[0], MODEM_TIMEOUT);
		ret = uart_recv_byte_timeout(&crc[1], MODEM_TIMEOUT);

		if (opt->dst) {
			if (img_type == IMG_MAX) {
				OOB_MODE oob_mode;
				img_type = image_type_detect(curr_addr, count);

				switch (img_type) {
				case IMG_YAFFS1:
					oob_mode = FLASH_OOB_RAW;
					break;

				case IMG_YAFFS2:
					oob_mode = MTD_OPS_AUTO_OOB;
					break;

				default:
					oob_mode = FLASH_OOB_PLACE;
					break;
				}

				ret = ioctl(fd_bdev, FLASH_IOCS_OOB_MODE, oob_mode);
				if (ret < 0)
					goto error;
			}

			ret = write(fd_bdev, curr_addr, count);
			if (ret < 0)
				goto error;
		}

		if (opt->load_addr)
			curr_addr += count;

		uart_send_byte(ACK);

		opt->load_size += count;
		blk++;
	}

L1:
	modem_end_rx();
	ret = opt->load_size;

error:
	if (opt->dst) {
		close(fd_bdev);
	}

	return ret;
}


