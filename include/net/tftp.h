#pragma once

#include <block.h>

#define TFTP_RRQ   CPU_TO_BE16(1)
#define TFTP_WRQ   CPU_TO_BE16(2)
#define TFTP_DAT   CPU_TO_BE16(3)
#define TFTP_ACK   CPU_TO_BE16(4)
#define TFTP_ERR   CPU_TO_BE16(5)

#define TFTP_HDR_LEN   4
#define TFTP_PKT_LEN   512
#define TFTP_BUF_LEN  (TFTP_PKT_LEN + TFTP_HDR_LEN)

#define TFTP_MODE_OCTET  "octet"

#undef  TFTP_DEBUG  // FIXME: depend on configuration
#define TFTP_VERBOSE

#define MAX_MODE_LEN 12
#define MAX_PATH_LEN 12

// to add: max_size for load_buff
struct tftp_opt {
	bool  verbose;
	char  file_name[FILE_NAME_SIZE];
	const char *dst;
	const char *src;
	char  mode[MAX_MODE_LEN];
	void *load_addr;
	size_t xmit_size;
	const char *type; // only for image
};

int tftp_download(struct tftp_opt *opt);
int tftp_upload(struct tftp_opt *opt);
