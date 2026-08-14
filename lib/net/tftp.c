#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <malloc.h>
#include <assert.h>
#include <delay.h>
#include <image.h>
#include <fcntl.h>
#include <block.h>
#include <unistd.h>
#include <net/net.h>
#include <net/tftp.h>
#include <net/socket.h>
#include <fs.h>
#include <mtd/mtd.h>

#define TFTP_DEBUG

struct tftp_packet {
	__u16 op_code;
	union {
		__u16 block;
		__u16 error;
	};
	__u8 data[0];
} __PACKED__;

static inline int tftp_make_req(__u8 *buff, __u16 req, const char *file_name, const char *mode)
{
	int len;

	*(__u16 *)buff = req; // TFTP_RRQ or TFTP_WRQ
	len = 2;

	strcpy((char *)buff + len, file_name);
	len += strlen(file_name);

	buff[len] = '\0';
	len += 1;

	strcpy((char *)buff + len, mode);
	len += strlen(mode);

	buff[len] = '\0';
	len += 1;

	return len;
}

static inline int tftp_send_ack(const int fd, const __u16 blk,
	struct sockaddr_in *remote_addr)
{
	int ret;
	struct tftp_packet tftp_pkt;

	tftp_pkt.op_code = TFTP_ACK;
	tftp_pkt.block = htons(blk);
	ret = sendto(fd, &tftp_pkt, TFTP_HDR_LEN, 0,
			(struct sockaddr *)remote_addr, sizeof(*remote_addr));

	return ret;
}

// FIXME
int tftp_download(struct tftp_opt *opt)
{
	int ret;
	int sockfd, fd = -1; // FIXME!!!
	__u16 blk_num;
	__u8 *buff_ptr;
	socklen_t addrlen;
	__u8 buf[TFTP_BUF_LEN];
	size_t  pkt_len, load_len;
	struct tftp_packet *tftp_pkt = (struct tftp_packet *)buf;
	struct sockaddr_in local_addr, remote_addr;
	image_t img_type = IMG_MAX;

	// printf(" \"%s\": %s => %s\n", opt->file_name, server_ip, local_ip);
	printf("loading file \"%s\" from %s\n", opt->file_name, opt->src);

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd <= 0) {
		printf("%s(): error @ line %d!\n", __func__, __LINE__);
		return -EIO;
	}

	memset(&local_addr, 0, sizeof(local_addr));
	local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	ret = bind(sockfd, (struct sockaddr *)&local_addr, sizeof(struct sockaddr));
	if (ret < 0)
		goto L1;

	pkt_len = tftp_make_req((__u8 *)tftp_pkt, TFTP_RRQ, opt->file_name,
				opt->mode[0] ? opt->mode : TFTP_MODE_OCTET);

	memset(&remote_addr, 0, sizeof(remote_addr));
	str_to_ip((__u8 *)&remote_addr.sin_addr.s_addr, opt->src); // bigendian
	remote_addr.sin_port = htons(STD_PORT_TFTP);

	ret = sendto(sockfd, tftp_pkt, pkt_len, 0,
		(struct sockaddr *)&remote_addr, sizeof(remote_addr));
	if (ret < 0)
		goto L1;

	buff_ptr = opt->load_addr;
	load_len = 0;
	blk_num  = 1;

	if (opt->dst) {
		fd = open(opt->dst, O_WRONLY);
		if (fd < 0) {
			printf("fail to open \"%s\"!\n", opt->dst);
			goto L1;
		}

		if (opt->type) {
			OOB_MODE oob_mode;

			if (!strcmp(opt->type, "jffs2")) {
				img_type = IMG_JFFS2;
			} else if (!strcmp(opt->type, "yffs2")) {
				img_type = IMG_YAFFS2;
			} else if (!strcmp(opt->type, "yffs1")) {
				img_type = IMG_YAFFS1;
			} else {
				img_type = IMG_UNKNOWN;
			}

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

			ret = ioctl(fd, FLASH_IOCS_OOB_MODE, oob_mode);
			if (ret < 0)
				goto L2;
		}
	}

	do {
		pkt_len = recvfrom(sockfd, tftp_pkt, TFTP_BUF_LEN, 0,
						(struct sockaddr *)&remote_addr, &addrlen);
		if(0 == pkt_len)
			goto L2;

		pkt_len -= TFTP_HDR_LEN;

		switch (tftp_pkt->op_code) {
		case TFTP_DAT:
			if (ntohs(tftp_pkt->block) == blk_num) {
				tftp_send_ack(sockfd, blk_num, &remote_addr);
				blk_num++;

				load_len += pkt_len;

#ifdef TFTP_VERBOSE
				if ((load_len & 0x3fff) == 0 || TFTP_PKT_LEN != pkt_len) {
					char tmp[32];

					val_to_hr_str(load_len, tmp);
					printf("\r %d(%s) loaded  ", load_len, tmp);
				}
#endif

				if (NULL != buff_ptr) {
					memcpy(buff_ptr, tftp_pkt->data, pkt_len);
					buff_ptr += pkt_len;
				}

				if (opt->dst) {
					if (img_type == IMG_MAX) {
						OOB_MODE oob_mode;

						img_type = image_type_detect(tftp_pkt->data, pkt_len);

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

						ret = ioctl(fd, FLASH_IOCS_OOB_MODE, oob_mode);
						if (ret < 0)
							goto L2;
					}

					ret = write(fd, tftp_pkt->data, pkt_len);
					if (ret < 0)
						goto L2;
				}
			} else {
#ifdef TFTP_DEBUG
				printf("\t%s(): LOST Packet = 0x%x (0x%x).\r",
					__func__, blk_num, ntohs(tftp_pkt->block));
#endif
				tftp_send_ack(sockfd, blk_num - 1, &remote_addr);
			}

			break;

		case TFTP_ERR:
			printf("\n%s(): %s (Error num = %d)\n",
				__func__, tftp_pkt->data, ntohs(tftp_pkt->error));

			ret = -EIO;
			goto L2;

		default:
			printf("\n%s(): Unsupported opcode 0x%02x! (block = %d)\n",
				__func__, ntohs(tftp_pkt->op_code), blk_num);

			ret = -EIO;
			goto L2;
		}
	} while (TFTP_PKT_LEN == pkt_len);

	opt->xmit_size = load_len;
L2:
#ifdef TFTP_VERBOSE
	printf("\n");
#endif
	if (opt->dst)
		close(fd);
L1:
	sk_close(sockfd);
	return ret;
}

int tftp_upload(struct tftp_opt *opt)
{
	int ret;
	int sockfd, fd;
	__u16 blk_num;
	socklen_t addrlen;
	__u8 buf[TFTP_BUF_LEN];
	size_t dat_len, send_len;
	struct tftp_packet *tftp_pkt = (struct tftp_packet *)buf;
	struct sockaddr_in local_addr, remote_addr;

	if (!opt->src)
		return -EINVAL;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd <= 0) {
		printf("%s(): error @ line %d!\n", __func__, __LINE__);
		return -EIO;
	}

	fd = open(opt->src, O_RDONLY);
	if (fd < 0) {
		printf("fail to open \"%s\"!\n", opt->src);
		ret = fd;
		goto L1;
	}

	memset(&local_addr, 0, sizeof(local_addr));
	local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	ret = bind(sockfd, (struct sockaddr *)&local_addr, sizeof(struct sockaddr));
	if (ret < 0)
		goto L1;

	printf("putting file \"%s\" to %s\n", opt->file_name, opt->dst);

	dat_len = tftp_make_req((__u8 *)tftp_pkt, TFTP_WRQ, opt->file_name,
				opt->mode[0] ? opt->mode : TFTP_MODE_OCTET);

	memset(&remote_addr, 0, sizeof(remote_addr));
	str_to_ip((__u8 *)&remote_addr.sin_addr.s_addr, opt->dst); // bigendian
	remote_addr.sin_port = htons(STD_PORT_TFTP);

	ret = sendto(sockfd, tftp_pkt, dat_len, 0,
		(struct sockaddr *)&remote_addr, sizeof(remote_addr));
	if (ret < 0)
		goto L1;

	send_len = 0;
	blk_num  = 0;

	while (1) {
		ret = recvfrom(sockfd, tftp_pkt, TFTP_BUF_LEN, 0,
						(struct sockaddr *)&remote_addr, &addrlen);
		if(ret <= 0)
			goto L2;

		switch (tftp_pkt->op_code) {
		case TFTP_ACK:
			if (ntohs(tftp_pkt->block) == blk_num) {
				ret = read(fd, tftp_pkt->data, TFTP_PKT_LEN);
				if (ret <= 0)
					goto L2;
				blk_num++;

				tftp_pkt->op_code = TFTP_DAT;
				tftp_pkt->block= htons(blk_num);

				dat_len = ret;

				ret = sendto(sockfd, tftp_pkt, dat_len + TFTP_HDR_LEN, 0,
					(struct sockaddr *)&remote_addr, sizeof(remote_addr));
				// if (ret != dat_len + TFTP_HDR_LEN)

				send_len += dat_len;

#ifdef TFTP_VERBOSE
				if ((send_len & 0x3fff) == 0 || TFTP_PKT_LEN != dat_len) {
					char tmp[32];

					val_to_hr_str(send_len, tmp);
					printf("\r %d(%s) sended  ", send_len, tmp);
				}
#endif
			} else {
#ifdef TFTP_DEBUG
				printf("\t%s(): LOST Packet = 0x%x (0x%x).\r",
					__func__, blk_num, ntohs(tftp_pkt->block));
#endif
				// TODO:  Resend
			}

			break;

		case TFTP_ERR:
			printf("\n%s(): %s (Error num = %d)\n",
				__func__, tftp_pkt->data, ntohs(tftp_pkt->error));

			ret = -EIO;
			goto L2;

		default:
			printf("\n%s(): Unsupported opcode 0x%02x! (CurBlkNum = %d)\n",
				__func__, ntohs(tftp_pkt->op_code), blk_num);

			ret = -EIO;
			goto L2;
		}
	}

	opt->xmit_size = send_len;
L2:
#ifdef TFTP_VERBOSE
	printf("\n");
#endif

	close(fd);

L1:
	sk_close(sockfd);
	return ret;
}
