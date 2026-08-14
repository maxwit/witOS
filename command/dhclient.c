#include <fcntl.h>
#include <errno.h>
#include <delay.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <random.h> // FIXME: to be removed
#include <fs.h>
#include <net/net.h>

// DHCP opcode
#define CLIENT_REQUEST	1
#define SERVER_ACK		2

#define ETHER_TYPE 1

// DHCP option type code
#define DHCP_MESSAGE		53
#define DHCP_MESSAGE_LEN	1
#define DHCP_REQUEST_IP		50
#define DHCP_REQUEST_IP_LEN 4

// DHCP message type value
#define DHCPDISCOVER	0x1
#define DHCPOFFER		0x2
#define DHCPREQUEST		0x3
#define DHCPDECLINE		0x4
#define DHCPACK			0x5
#define DHCPNAK			0x6
#define DHCPRELEASE		0x7
#define DHCPINFOM		0x8

struct dhcp_packet {
	char	op;
	char	htype;
	char	hlen;
	char	hops;
	__u32	xid;
	short	sec;
	short	flags;
	__u32	ciaddr;
	__u32	yiaddr;
	__u32	siaddr;
	__u32	giaddr;
	char	chaddr[16];
	char	sname[64];
	char	file[128];
	// actually, the options field's size is 64 bytes(include the magic cookie)
	char	magic_cookie[4];
	char	option[60];
}__PACKED__;

static void init_dhcp_packet(struct dhcp_packet *packet, __u32 xid, __u8 mac_addr[])
{
	memset(packet, 0x0, sizeof(*packet));

	packet->op		= CLIENT_REQUEST;
	packet->htype	= ETHER_TYPE;
	packet->hlen	= MAC_ADR_LEN;
	packet->hops	= 0x00;
	packet->xid     = xid;
	packet->sec     = 0x00;
	packet->flags   = 0x00;
	packet->yiaddr  = 0x00;
	memcpy(packet->chaddr, mac_addr, MAC_ADR_LEN);
	// magic cookie
	packet->magic_cookie[0] = 0x63;
	packet->magic_cookie[1] = 0x82;
	packet->magic_cookie[2] = 0x53;
	packet->magic_cookie[3] = 0x63;
}

static int send_dhcp_discover(int sockfd, struct dhcp_packet *packet, struct sockaddr_in *remote_addr)
{
	packet->option[0] = DHCP_MESSAGE;
	packet->option[1] = DHCP_MESSAGE_LEN;
	packet->option[2] = DHCPDISCOVER;
	packet->option[3] = 0xff;

	return sendto(sockfd, packet, sizeof(*packet), 0,
				(const struct sockaddr *)remote_addr, sizeof(struct sockaddr));
}

static int send_dhcp_request(int sockfd, struct dhcp_packet *packet, struct sockaddr_in *remote_addr)
{
	__u32 request_ip;

	request_ip = packet->yiaddr;

	packet->option[0] = DHCP_MESSAGE;
	packet->option[1] = DHCP_MESSAGE_LEN;
	packet->option[2] = DHCPREQUEST;
	packet->option[3] = DHCP_REQUEST_IP;
	packet->option[4] = DHCP_REQUEST_IP_LEN;
	// request ip
	packet->option[5] = request_ip & 0xff;
	packet->option[6] = request_ip >> 8 & 0xff;
	packet->option[7] = request_ip >> 16 & 0xff;
	packet->option[8] = request_ip >> 24;

	packet->option[9] = 0xff;

	return sendto(sockfd, packet, sizeof(*packet), 0,
				(const struct sockaddr *)remote_addr, sizeof(*remote_addr));
}

static int send_dhcp_declient(int sockfd, struct dhcp_packet *packet, struct sockaddr_in *remote_addr)
{
	__u32 declient_ip;

	declient_ip = packet->yiaddr;

	packet->option[0] = DHCP_MESSAGE;
	packet->option[1] = DHCP_MESSAGE_LEN;
	packet->option[2] = DHCPDECLINE;
	packet->option[3] = DHCP_REQUEST_IP;
	packet->option[4] = DHCP_REQUEST_IP_LEN;
	// request ip
	packet->option[5] = declient_ip & 0xff;
	packet->option[6] = declient_ip >> 8 & 0xff;
	packet->option[7] = declient_ip >> 16 & 0xff;
	packet->option[8] = declient_ip >> 24;

	packet->option[9] = 0xff;

	return sendto(sockfd, packet, sizeof(*packet), 0,
				(const struct sockaddr *)remote_addr, sizeof(*remote_addr));
}

int qu_is_empty(int fd);

static int check_ip_is_user(__u32 ip)
{
	int ret, i;
	int sockfd;
	struct sockaddr_in remote_addr, local_addr;
	socklen_t addrlen = sizeof(struct sockaddr_in);

	sockfd = socket(AF_INET, SOCK_RAW, PROT_ETH);
	if (sockfd < 0) {
		printf("socket() failed!\n");
		return sockfd;
	}

	local_addr.sin_addr.s_addr = 0;
	remote_addr.sin_addr.s_addr = ip;

	ret = bind(sockfd, (struct sockaddr *) &local_addr, sizeof(local_addr));
	if (ret < 0) {
		printf("connect() failed!\n");
		goto L1;
	}

	ret = sendto(sockfd, NULL, 0, 0, (struct sockaddr *)&remote_addr, addrlen);
	if (ret < 0) {
		printf("sendto arp error\n");
		goto L1;
	}

	for (i = 0; i < 3; i++) {
		if (qu_is_empty(sockfd)) {
			if (i >= 3)
				break;

			udelay(10000000);
			continue;
		}

		ret = 1;
		goto L1;
	}

	ret = 0;

L1:
	close(sockfd);

	return ret;
}

int main(int argc, char *argv[])
{
	int opt;
	int ret, i, j;
	int sockfd;
	__u32 xid;
	char nic_name[NET_NAME_LEN];
	char ip_str[IPV4_STR_LEN];
	__u8 mac_addr[MAC_ADR_LEN];
	bool sync_svr = false, nic = false;
	socklen_t addrlen;
	struct net_device *ndev = NULL;
	struct list_head *ndev_list, *iter;
	struct sockaddr_in remote_addr, local_addr;
	struct dhcp_packet packet;

	while ((opt = getopt(argc, argv, "x:sh")) != -1) {
		switch (opt) {
		case 's':
			sync_svr = true;
			break;

		case 'x':
			nic = true;
			strncpy(nic_name, optarg, NET_NAME_LEN);
			break;

		default:
			usage();
			return -EINVAL;
		}
	}

	if (nic) {
		ndev_list= ndev_get_list();

		list_for_each(iter, ndev_list) {
			ndev = container_of(iter, struct net_device, ndev_node);

			if (strncpy(ndev->ifx_name, nic_name, NET_NAME_LEN) == 0)
				break;
		}

		if (NULL == ndev) {
			printf("device \'%s\' not found\n", nic_name);
			return -ENODEV;
		}
	} else {
		ndev = ndev_get_first();

		if (NULL == ndev) {
			printf("No NIC available!\n");
			return -ENODEV;
		}
	}

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		printf("socket() failed!\n");
		return sockfd;
	}

	local_addr.sin_addr.s_addr = 0;
	local_addr.sin_port = htons(68);

	memset(&remote_addr.sin_addr, 0xff, IPV4_ADR_LEN);
	remote_addr.sin_port = htons(67);

	ret = bind(sockfd, (const struct sockaddr *) &local_addr, sizeof(local_addr));
	if (ret < 0) {
		printf("connect() failed!\n");
		goto error;
	}

	ret = ndev_ioctl(ndev, NIOC_GET_MAC, mac_addr);

	srandom(__LINE__);
	xid = random();

	init_dhcp_packet(&packet, xid, mac_addr);

	ret = send_dhcp_discover(sockfd, &packet, &remote_addr);
	if (ret < 0) {
		printf("send dhcp discover packet failed!\n");
		goto error;
	}

	addrlen = sizeof(remote_addr);

	for (i = 0, j = 0; i < 3; i++) {
#if 0
		if (qu_is_empty(sockfd)) {
			if (i >= 3)
				break;

			udelay(10000000);

			continue;
		}
#endif

		ret = recvfrom(sockfd, &packet, sizeof(packet), 0,
				(struct sockaddr *)&remote_addr, &addrlen);
		if (ret <= 0) {
			printf("recv dhcp offer packet failed!");
			goto error;
		}

		if (packet.xid == xid) {
			if (0 == j) {
				if (packet.option[2] != DHCPOFFER) {
					if (i < 3)
						continue;

					printf("Can't recv offer packet!\n");
					ret = -ENONET;
					goto error;
				}

				remote_addr.sin_addr.s_addr = 0xFFFFFFFF;

				i = -1;
				j++;

				ret = send_dhcp_request(sockfd, &packet, &remote_addr);
				if (ret < 0) {
					printf("send dhcp request packet failed!");
					goto error;
				}
			} else {
				if (packet.option[2] != DHCPACK) {
					if (i < 3)
						continue;

					printf("Can't recv ACK packet!\n");
					ret = -ENONET;
					goto error;
				}

				break;
			}
		}
	}

	if (i >= 3) {
		printf("Time out\n");
		goto error;
	}

	ret = check_ip_is_user(packet.yiaddr);
	if (ret != 0) {
		ip_to_str(ip_str, packet.yiaddr);
		printf("ip: %s is used\n", ip_str);

		send_dhcp_declient(sockfd, &packet, &remote_addr);

		goto error;
	}

	ndev_ioctl(ndev, NIOC_SET_IP, (void *)packet.yiaddr);

	if (sync_svr) {
		net_set_server_ip(packet.siaddr);
		// conf_store();
	}

	ip_to_str(ip_str, remote_addr.sin_addr.s_addr);
	printf("server ip: %s\n", ip_str);
	ip_to_str(ip_str, packet.yiaddr);
	printf("local  ip: %s\n", ip_str);

	sk_close(sockfd);

	return 0;

error:
	sk_close(sockfd);

	return ret;
}
