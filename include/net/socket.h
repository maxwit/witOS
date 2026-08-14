#pragma once

#include <types.h>
#include <list.h>

// FIXME
#define	PF_INET		2
#define	AF_INET		PF_INET
#define INADDR_ANY  0

enum __socket_type {
	SOCK_STREAM    = 1,
	SOCK_DGRAM     = 2,
	SOCK_RAW       = 3,
	SOCK_RDM       = 4,
	SOCK_SEQPACKET = 5,
	SOCK_DCCP      = 6,
	SOCK_PACKET    = 10,
	SOCK_CLOEXEC   = 02000000,
	SOCK_NONBLOCK  = 04000
};

typedef __u32 socklen_t;
typedef unsigned short sa_family_t;

#define __SOCKADDR_COMMON(sa_prefix) \
	sa_family_t sa_prefix##family

struct sockaddr {
	__SOCKADDR_COMMON(sa_);
	char sa_data[14];
};

struct in_addr {
	__u32 s_addr;
};

struct sockaddr_in {
	__SOCKADDR_COMMON(sin_);
	unsigned short sin_port;
	struct in_addr sin_addr;

	unsigned char sin_zero[sizeof(struct sockaddr) -
				sizeof(unsigned short) -
				sizeof(unsigned short) -
				sizeof(struct in_addr)];
};

enum {
	SA_SRC,
	SA_DST
};

// FIXME
enum tcp_state {
	TCPS_CLOSED,
	TCPS_LISTEN,
	TCPS_SYN_SENT,
	TCPS_SYN_RCVD,
	TCPS_ESTABLISHED,
	TCPS_CLOSE_WAIT,
	TCPS_FIN_WAIT1,
	TCPS_CLOSING,
	TCPS_LAST_ACK,
	TCPS_FIN_WAIT2,
	TCPS_TIME_WAIT,
};

struct socket {
	int type;
	int protocol;
	int obstruct_flags;
	struct list_head tx_qu, rx_qu;
	struct sockaddr_in saddr[2]; // FIXME: sockaddr instead
	enum tcp_state state;
	// int tmp;
	__u32 seq_num, ack_num;
};

static inline __u16 htons(__u16 val)
{
	return CPU_TO_BE16(val);
}

static inline __u32 htonl(__u32 val)
{
	return CPU_TO_BE32(val);
}

static inline __u16 ntohs(__u16 val)
{
	return BE16_TO_CPU(val);
}

static inline __u32 ntohl(__u32 val)
{
	return BE32_TO_CPU(val);
}

int socket(int domain, int type, int protocol);
int bind(int fd, const struct sockaddr *addr, socklen_t len);
int connect(int fd, const struct sockaddr *addr, socklen_t len);
ssize_t send(int fd, const void *buf, size_t n, int flag);
ssize_t recv(int fd, void *buf, size_t n, int flag);
ssize_t sendto(int fd, const void *buf, __u32 n, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
ssize_t recvfrom(int fd, void *buf, __u32 n, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
int sk_close(int fd);

#define SKIOCS_FLAGS 1

int socket_ioctl(int fd, int cmd, int flags);
