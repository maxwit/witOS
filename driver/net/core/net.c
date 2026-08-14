#include <init.h>
#include <delay.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <malloc.h>
#include <net/net.h>
#include <uart/uart.h>

struct host_addr {
	struct eth_addr in_addr;
	struct list_head node;
};

struct pseudo_header {
	__u8  src_ip[IPV4_ADR_LEN];
	__u8  des_ip[IPV4_ADR_LEN];
	__u8  zero;
	__u8  prot;
	__u16 size;
};

static LIST_HEAD(g_host_list);
#ifdef CONFIG_DEBUG
static const char *g_arp_desc[] = {"N/A", "Request", "Reply"};
#endif

static int pseudo_calculate_checksum(struct sock_buff *skb, __u16 *checksum)
{
	struct pseudo_header *pse_hdr;
	struct socket *sock = skb->sock;

	pse_hdr = (struct pseudo_header *)skb->data - 1;

	memcpy(pse_hdr->src_ip, &sock->saddr[SA_SRC].sin_addr, IPV4_ADR_LEN);
	memcpy(pse_hdr->des_ip, &sock->saddr[SA_DST].sin_addr, IPV4_ADR_LEN);
	pse_hdr->zero = 0;

	switch (sock->type) {
	case SOCK_STREAM:
		pse_hdr->prot = PROT_TCP;
		break;

	case SOCK_DGRAM:
		pse_hdr->prot = PROT_UDP;
		break;

	default:
		return -EINVAL;
	}

	pse_hdr->size = htons(skb->size);

	if (skb->size & 1)
		skb->data[skb->size] = 0;

	*checksum = ~net_calc_checksum(pse_hdr, sizeof(*pse_hdr) + skb->size);

	return 0;
}

static int ether_send_packet(struct sock_buff *skb, const __u8 mac[], __u16 type);

struct socket *udp_search_socket(const struct udp_header *, const struct ip_header *);

struct socket *tcp_search_socket(const struct tcp_header *, const struct ip_header *);

struct socket *icmp_search_socket(const struct ping_packet *ping_pkt, const struct ip_header *ip_pkt);

static inline bool ip_is_bcast(struct net_device *ndev, __u32 ip)
{
	return ~(ndev->mask | ip) == 0;
}

static inline void mac_fill_bcast(__u8 mac[])
{
	memset(mac,(__u8)0xff, MAC_ADR_LEN);
}

#ifdef CONFIG_NET_DEBUG
static void ether_info(struct ether_header *eth_head)
{
	printf("\tEther frame type: 0x%04x\n", ntohs(eth_head->frame_type));

	printf("\tdest mac: %02x:%02x:%02x:%02x:%02x:%02x\n",
		eth_head->des_mac[0],
		eth_head->des_mac[1],
		eth_head->des_mac[2],
		eth_head->des_mac[3],
		eth_head->des_mac[4],
		eth_head->des_mac[5]);

	printf("\tsource mac: %02x:%02x:%02x:%02x:%02x:%02x\n",
		eth_head->src_mac[0],
		eth_head->src_mac[1],
		eth_head->src_mac[2],
		eth_head->src_mac[3],
		eth_head->src_mac[4],
		eth_head->src_mac[5]);
}

static void dump_sock_buff(const struct sock_buff *skb)
{
	int idx;
	__u8 *data;

	for (idx = 0, data = skb->head; idx < skb->size; idx++, data++)
		printf("%02x", *data);

	printf("\n");
}
#endif

//----------------- TCP Layer -----------------

void tcp_send_packet(struct sock_buff *skb, __u8 flags, struct tcp_option *opt)
{
	__u16 size = skb->size, opt_len;
	struct tcp_header *tcp_hdr;
	struct socket *sock = skb->sock;

	opt_len = opt ? opt->len : 0;

	skb->data -= TCP_HDR_LEN + opt_len;
	skb->size += TCP_HDR_LEN + opt_len;

	tcp_hdr = (struct tcp_header *)skb->data;
	//
	tcp_hdr->src_port = sock->saddr[SA_SRC].sin_port;
	tcp_hdr->dst_port = sock->saddr[SA_DST].sin_port;
	tcp_hdr->seq_num  = htonl(sock->seq_num);
	tcp_hdr->ack_num  = htonl(sock->ack_num);
	tcp_hdr->hdr_len  = (TCP_HDR_LEN + opt_len) >> 2;
#if 0
	tcp_hdr->reserve1 = 0;
	tcp_hdr->reserve2 = 0;
	tcp_hdr->flg_urg  = !!(flags & FLG_URG);
	tcp_hdr->flg_ack  = !!(flags & FLG_ACK);
	tcp_hdr->flg_psh  = !!(flags & FLG_PSH);
	tcp_hdr->flg_rst  = !!(flags & FLG_RST);
	tcp_hdr->flg_syn  = !!(flags & FLG_SYN);
	tcp_hdr->flg_fin  = !!(flags & FLG_FIN);
#else
	tcp_hdr->reserve  = 0;
	tcp_hdr->flags    = flags;
#endif
	// FIXME!!!
	if (flags & FLG_ACK)
		tcp_hdr->win_size = htons(457);
	else
		tcp_hdr->win_size = htons(14600);
	tcp_hdr->checksum = 0;
	tcp_hdr->urg_ptr  = 0;

	if (opt_len > 0)
		memcpy(tcp_hdr->options, opt, opt_len);

	pseudo_calculate_checksum(skb, &tcp_hdr->checksum);

	ip_send_packet(skb, PROT_TCP);

	if (flags & (FLG_SYN | FLG_FIN))
		sock->seq_num++;

	if (flags & FLG_PSH)
		sock->seq_num += size;
}

//----------------- UDP Layer -----------------
void udp_send_packet(struct sock_buff *skb)
{
	struct udp_header *udp_hdr;
	struct socket *sock = skb->sock;

	skb->data -= UDP_HDR_LEN;
	skb->size += UDP_HDR_LEN;

	udp_hdr = (struct udp_header *)skb->data;
	//
	udp_hdr->src_port = sock->saddr[SA_SRC].sin_port;
	udp_hdr->dst_port = sock->saddr[SA_DST].sin_port;
	udp_hdr->udp_len  = htons(skb->size);
	udp_hdr->checksum = 0;

	ip_send_packet(skb, PROT_UDP);
}

static int udp_layer_deliver(struct sock_buff *skb, const struct ip_header *ip_hdr)
{
	struct udp_header *udp_hdr;
	struct socket *sock;

	udp_hdr = (struct udp_header *)skb->data;

	skb->data += UDP_HDR_LEN;
	skb->size -= UDP_HDR_LEN;

	sock = udp_search_socket(udp_hdr, ip_hdr);
	if (NULL == sock) {
		skb_free(skb);
		return -ENOENT;
	}

	memcpy(&sock->saddr[SA_DST].sin_addr, ip_hdr->src_ip, IPV4_ADR_LEN);
	sock->saddr[SA_DST].sin_port = udp_hdr->src_port;

	list_add_tail(&skb->node, &sock->rx_qu);

	return 0;
}

static int tcp_send_ack(struct socket *sock)
{
	struct sock_buff *skb;

	skb = skb_alloc(ETH_HDR_LEN + IP_HDR_LEN + TCP_HDR_LEN, 0);
	if (NULL == skb)
		return -ENOMEM;

	skb->sock = sock;
	tcp_send_packet(skb, FLG_ACK, NULL);

	return 0;
}

static int tcp_layer_deliver(struct sock_buff *skb, const struct ip_header *ip_hdr)
{
	__u16 hdr_len;
	struct tcp_header *tcp_hdr;
	struct socket *sock;

	tcp_hdr = (struct tcp_header *)skb->data;

	hdr_len = tcp_hdr->hdr_len << 2;

	skb->data += hdr_len;
	skb->size -= hdr_len;

	DPRINT("%s(): src_port = 0x%x, dst_port = 0x%x, flags = 0x%08x\n",
		__func__, tcp_hdr->src_port, tcp_hdr->dst_port, *(&tcp_hdr->ack_num + 1));

	sock = tcp_search_socket(tcp_hdr, ip_hdr);
	if (NULL == sock) {
		skb_free(skb);
		return -ENOENT;
	}

	skb->sock = sock;

	switch (tcp_hdr->flags) {
	case FLG_SYN:
	case FLG_SYN | FLG_ACK:
		sock->ack_num = ntohl(tcp_hdr->seq_num) + 1;
		skb_free(skb);
		tcp_send_ack(sock);

		if (TCPS_SYN_SENT == sock->state)
			sock->state = TCPS_ESTABLISHED;
		else
			BUG();

		break;

	case FLG_FIN:
	case FLG_FIN | FLG_ACK:
	case FLG_FIN | FLG_ACK | FLG_PSH:
		if (skb->size > 0) {
			sock->ack_num = ntohl(tcp_hdr->seq_num) + skb->size;
			list_add_tail(&skb->node, &sock->rx_qu);
		} else {
			sock->ack_num = ntohl(tcp_hdr->seq_num) + 1;

			if (TCPS_FIN_WAIT1 == sock->state) {
				if (tcp_hdr->flags & FLG_ACK)
					sock->state = TCPS_TIME_WAIT;
				else
					sock->state = TCPS_CLOSING;
			} else if (TCPS_FIN_WAIT2 == sock->state)
				sock->state = TCPS_TIME_WAIT;
			else if (TCPS_ESTABLISHED == sock->state)
				sock->state = TCPS_CLOSE_WAIT;
			else
				BUG();

			skb_free(skb);
		}

		tcp_send_ack(sock);

		break;

	case FLG_ACK:
		if (skb->size == 0) {
			skb_free(skb);

			if (TCPS_FIN_WAIT1 == sock->state)
				sock->state = TCPS_FIN_WAIT2;
			else if (TCPS_CLOSING == sock->state)
				sock->state = TCPS_TIME_WAIT;
			else if (TCPS_LAST_ACK == sock->state)
				sock->state = TCPS_CLOSED;
#if 0
			else if (TCPS_TIME_WAIT == sock->state)
				sock->state = TCPS_CLOSED;
#endif
			break;
		}

	case FLG_ACK | FLG_PSH:
		sock->ack_num = ntohl(tcp_hdr->seq_num) + skb->size;
		list_add_tail(&skb->node, &sock->rx_qu);
		tcp_send_ack(sock);

		break;

	default:
		printf("%s() line %d\n", __func__, __LINE__);
		break;
	}

	return 0;
}

static int init_ping_packet(struct ping_packet *ping_pkt,
				const __u8 *buff, __u8 type, __u32 size, __u16 id, __u16 seq)
{
	char *data = NULL;
	__u32 hdr_len = sizeof(struct ping_packet);

	if (NULL == ping_pkt || NULL == buff)
	    return -EINVAL;

	ping_pkt->type   = type;
	ping_pkt->code   = 0;
	ping_pkt->chksum = 0;
	ping_pkt->id     = id;
	ping_pkt->seqno  = seq;

	data = (char *)ping_pkt + hdr_len;

	memcpy(data, buff, size - hdr_len);

	ping_pkt->chksum = ~net_calc_checksum(ping_pkt, size);

	return 0;
}

int ping_send(struct sock_buff *rx_skb, const struct ip_header *ip_hdr, __u8 type)
{
	__u8 ping_buff[PING_PACKET_LENGTH];
	struct socket sock;
	struct sock_buff *tx_skb;
	// struct ether_header *eth_head;
	struct ping_packet *ping_pkt, *rx_ping_head;

	// eth_head = (struct ether_header *)(rx_skb->data - IP_HDR_LEN - ETH_HDR_LEN);

	rx_ping_head = (struct ping_packet *)rx_skb->data;
	ping_pkt = (struct ping_packet *)ping_buff;

	tx_skb = skb_alloc(ETH_HDR_LEN + IP_HDR_LEN, PING_PACKET_LENGTH);
	if (NULL == tx_skb) {
	    printf("%s(): skb_alloc return null\n", __func__);
	    return -EBUSY;
	}

	// ?
	memcpy(&sock.saddr[SA_DST].sin_addr, ip_hdr->src_ip, IPV4_ADR_LEN);
	memcpy(&sock.saddr[SA_SRC].sin_addr, ip_hdr->des_ip, IPV4_ADR_LEN);

	tx_skb->sock = &sock;

	init_ping_packet(ping_pkt, (__u8 *)rx_ping_head + sizeof(struct ping_packet),
		type, PING_PACKET_LENGTH, rx_ping_head->id, rx_ping_head->seqno);

	memcpy(tx_skb->data, ping_pkt, PING_PACKET_LENGTH);

	ip_send_packet(tx_skb, PROT_ICMP);

	return 0;
}

static int icmp_deliver(struct sock_buff *skb, const struct ip_header *ip_hdr)
{
	struct ping_packet *ping_hdr;
	// struct ether_header *eth_head;
	struct socket *sock;

	ping_hdr = (struct ping_packet *)(skb->data + ((ip_hdr->ver_len & 0xf) << 2));
	// eth_head = (struct ether_header *)(skb->data- ETH_HDR_LEN);

	switch(ping_hdr->type) {
	case ICMP_TYPE_ECHO_REQUEST:
		skb->data += ((ip_hdr->ver_len & 0xf) << 2);
		skb->size -= ((ip_hdr->ver_len & 0xf) << 2);
	    ping_send(skb, ip_hdr, ICMP_TYPE_ECHO_REPLY);
	    break;

	case ICMP_TYPE_ECHO_REPLY:
		sock = icmp_search_socket(ping_hdr, ip_hdr);
		if (NULL == sock) {
			skb_free(skb);
			return -ENOENT;
		}
		list_add_tail(&skb->node, &sock->rx_qu);
	    break;

	case ICMP_TYPE_DEST_UNREACHABLE:
	    printf("From %d.%d.%d.%d: icmp_seq=%d Destination Host Unreachable\n",
	            ip_hdr->des_ip[0],
	            ip_hdr->des_ip[1],
	            ip_hdr->des_ip[2],
	            ip_hdr->des_ip[3],
	            ntohs(ping_hdr->seqno));

	    break;

	default:
	    break;
	}

	return 0;
}

int ip_layer_deliver(struct sock_buff *skb)
{
	struct ip_header *ip_hdr;
	__u8 ip_hdr_len;

	ip_hdr = (struct ip_header *)skb->data;
	ip_hdr_len = (ip_hdr->ver_len & 0xf) << 2;

	skb->data += ip_hdr_len;
	skb->size = ntohs(ip_hdr->total_len) - ip_hdr_len;

	if (ip_hdr_len < IP_HDR_LEN) {
		printf("Warning: ip_hdr head len not match\n");
		return -EINVAL;
	}

	switch(ip_hdr->up_prot) {
	case PROT_UDP:
		// printf("\tUDP received!\n");
		udp_layer_deliver(skb, ip_hdr);
		break;

	case PROT_TCP:
		// printf("\tTCP received!\n");
		tcp_layer_deliver(skb, ip_hdr);
		break;

	case PROT_ICMP:
		// printf("\n\tICMP received!\n");
		skb->data -= ip_hdr_len;
		skb->size += ip_hdr_len;
		icmp_deliver(skb, ip_hdr);
		break;

	case PROT_IGMP:
		// printf("\tIGMP received!\n");
		skb_free(skb);
		break;

	case PROT_OSPF:
		printf("\tOSPF received!\n");
		skb_free(skb);
		break;

	default:
		printf("\n\tUnknown IP Protocol!\n");
		skb_free(skb);
		return 0;
	}

	return ip_hdr->up_prot;
}

static inline int mac_fill_from_arp(__u8 mac[], __u32 nip)
{
	struct eth_addr *remote_addr;

	remote_addr = getaddr(nip);
	if (NULL == remote_addr) {
		remote_addr = gethostaddr(nip);
		if (NULL == remote_addr) {
			const __u8 *ip = (__u8 *)&nip;

			printf("%s(): host \"%d.%d.%d.%d\" not found!\n",
				__func__, ip[0], ip[1], ip[2], ip[3]);

			return -EIO;
		}
	}

	memcpy(mac, remote_addr->mac, MAC_ADR_LEN);
	return 0;
}

int ip_send_packet(struct sock_buff *skb, __u8 prot)
{
	int ret;
	__u32 nip;
	__u8 mac[MAC_ADR_LEN];
	struct ip_header *ip_hdr;
	struct socket *sock = skb->sock;
	static __u16 ip_id = 1;

	skb->data -= IP_HDR_LEN;
	skb->size += IP_HDR_LEN;

	ip_hdr = (struct ip_header *)skb->data;

	ip_hdr->ver_len   = 0x45;
	ip_hdr->tos       = 0; // FIXME
	ip_hdr->total_len = htons((__u16)skb->size);
	ip_hdr->id        = htons(ip_id); //
	ip_hdr->flag_frag = htons(0x4000);
	ip_hdr->ttl       = 64;
	ip_hdr->up_prot   = prot;
	ip_hdr->chksum    = 0;
	ip_id++;

	memcpy(ip_hdr->src_ip, &sock->saddr[SA_SRC].sin_addr, IPV4_ADR_LEN);
	memcpy(ip_hdr->des_ip, &sock->saddr[SA_DST].sin_addr, IPV4_ADR_LEN);

	ip_hdr->chksum = ~net_calc_checksum(ip_hdr, IP_HDR_LEN);

	//
	nip = sock->saddr[SA_DST].sin_addr.s_addr;
	if (ip_is_bcast(skb->ndev, ntohl(nip))) {
		mac_fill_bcast(mac);
	} else {
		ret = mac_fill_from_arp(mac, nip);
		if (ret < 0)
			return ret;
	}

	return ether_send_packet(skb, mac, ETH_TYPE_IP);
}

//------------------------ ARP Layer -------------------------
void arp_send_packet(const __u8 nip[], const __u8 *mac, __u16 op_code)
{
	struct sock_buff *skb;
	struct net_device *ndev;
	struct arp_packet *arp_pkt;

	skb = skb_alloc(ETH_HDR_LEN, ARP_PKT_LEN);
	if (NULL == skb) {
		printf("%s: fail to alloc skb!\n", __func__);
		return;
	}

	ndev = skb->ndev;

	arp_pkt = (struct arp_packet *)skb->data;

	arp_pkt->hard_type = htons(1);
	arp_pkt->prot_type = ETH_TYPE_IP;
	arp_pkt->hard_size = MAC_ADR_LEN;
	arp_pkt->prot_size = IPV4_ADR_LEN;
	arp_pkt->op_code   = op_code;

	memcpy(arp_pkt->src_ip, &ndev->ip, IPV4_ADR_LEN);
	memcpy(arp_pkt->des_ip, nip, IPV4_ADR_LEN);
	memcpy(arp_pkt->src_mac, ndev->mac_addr, MAC_ADR_LEN);

	if (NULL == mac)
		mac_fill_bcast(arp_pkt->des_mac);
	else
		memcpy(arp_pkt->des_mac, mac, MAC_ADR_LEN);

	ether_send_packet(skb, arp_pkt->des_mac, ETH_TYPE_ARP);
}

static int arp_recv_packet(struct sock_buff *skb)
{
	__u32 ip;
	struct arp_packet *arp_pkt;
	struct net_device *ndev = skb->ndev;

	arp_pkt = (struct arp_packet *)skb->data;

	if (arp_pkt->prot_type != ETH_TYPE_IP) {
		printf("\tProt Error!\n");
		return -ENOTSUPP;
	}

	memcpy(&ip, arp_pkt->src_ip, 4);

	DPRINT("\t%s ARP received from: %d.%d.%d.%d\n",
		g_arp_desc[ntohs(arp_pkt->op_code)],
		arp_pkt->src_ip[0], arp_pkt->src_ip[1], arp_pkt->src_ip[2], arp_pkt->src_ip[3]);

	switch (arp_pkt->op_code) {
	case ARP_OP_REP:
		if (getaddr(ip) == NULL) {
			struct host_addr *host;
			struct socket *sock;

			host = malloc(sizeof(struct host_addr));
			if (NULL == host)
				return -ENOMEM;

			memcpy(host->in_addr.ip, arp_pkt->src_ip, IPV4_ADR_LEN);
			memcpy(host->in_addr.mac, arp_pkt->src_mac, MAC_ADR_LEN);
#if 0
			printf("ARP %d.%d.%d.%d<-->%02x.%02x.%02x.%02x.%02x.%02x\n",
				   host->in_addr.ip[0],
				   host->in_addr.ip[1],
				   host->in_addr.ip[2],
				   host->in_addr.ip[3],
				   host->in_addr.mac[0],
				   host->in_addr.mac[1],
				   host->in_addr.mac[2],
				   host->in_addr.mac[3],
				   host->in_addr.mac[4],
				   host->in_addr.mac[5]
				   );
#endif

			list_add_tail(&host->node, &g_host_list);

			sock = arp_search_socket(arp_pkt);
			if (NULL == sock) {
				skb_free(skb);
				break;
			}

			list_add_tail(&skb->node, &sock->rx_qu);
		}

		break;

	case ARP_OP_REQ:
		if (0 == memcmp(arp_pkt->des_ip, &ndev->ip, IPV4_ADR_LEN))
			arp_send_packet(arp_pkt->src_ip, arp_pkt->src_mac, ARP_OP_REP);

		skb_free(skb);
		break;

	default:
		printf("\t%s(): op_code error!\n", __func__);
		skb_free(skb);
		break;
	}


	return 0;
}

//-----------------------------------------------
int netif_rx(struct sock_buff *skb)
{
	struct ether_header *eth_head;

	eth_head = (struct ether_header *)skb->data;

	skb->data += ETH_HDR_LEN;
	skb->size -= ETH_HDR_LEN;

	switch(eth_head->frame_type) {
	case ETH_TYPE_ARP:
		// printf("\tARP received.\n");
		arp_recv_packet(skb);
		break;

	case ETH_TYPE_RARP:
		DPRINT("\tRARP packet received.\n");
		skb_free(skb);
		break;

	case ETH_TYPE_IP:
		// printf("\tIP packet received.\n");
		ip_layer_deliver(skb);
		break;

	default:
		// DPRINT("\tframe type error (= 0x%04x)!\n", eth_head->frame_type);
		skb_free(skb);
		break;
	}

	return 0;
}

//------------------ Send Package to Hardware -----------------
int ether_send_packet(struct sock_buff *skb, const __u8 mac[], __u16 type)
{
	int ret;
	struct ether_header *eth_head;
	struct net_device *ndev = skb->ndev;

	skb->data -= ETH_HDR_LEN;
	skb->size += ETH_HDR_LEN;

	// for assert()?
	if (skb->data != skb->head) {
		printf("%s(): skb len error! (data = 0x%p, head = 0x%p)\n",
			__func__, skb->data, skb->head);
		return -EINVAL;
	}

	eth_head = (struct ether_header *)skb->data;

	memcpy(eth_head->des_mac, mac, MAC_ADR_LEN);
	memcpy(eth_head->src_mac, ndev->mac_addr, MAC_ADR_LEN);
	eth_head->frame_type = type;

	ret = ndev->send_packet(ndev, skb);
	// if ret < 0 ...

	skb_free(skb);

	return ret;
}

__u16 net_calc_checksum(const void *buff, __u32 size)
{
	int n;
	__u32 chksum;
	const __u16 *p;

	chksum = 0;

	for (n = size, p = buff; n >= 2; n -= 2, p++)
		chksum += *p;

	if (n == 1)
		chksum += *(__u8 *)p;

	chksum = (chksum & 0xffff) + (chksum >> 16);
	chksum = (chksum & 0xffff) + (chksum >> 16);

	return chksum & 0xffff;
}

struct eth_addr *getaddr(__u32 nip)
{
	struct list_head *iter;
	struct host_addr *host;
	__u32 __UNUSED__ psr;
	__u32 *dip;
	struct eth_addr *addr = NULL;

	lock_irq_psr(psr);

	list_for_each(iter, &g_host_list) {
		host = container_of(iter, struct host_addr, node);

		dip = (__u32 *)host->in_addr.ip;
		if (*dip == nip) {
			addr = &host->in_addr;
			break;
		}
	}

	unlock_irq_psr(psr);

	return addr;
}

int net_get_server_ip(__u32 *ip)
{
	int ret;
	char buff[CONF_VAL_LEN];

	ret = conf_get_attr("net.server", buff);
	if (ret < 0)
		return ret;

	ret = str_to_ip((__u8 *)ip, buff);

	return ret;
}

int net_set_server_ip(__u32 ip)
{
	char ip_str[IPV4_STR_LEN];
	const char *attr = "net.server";

	ip_to_str(ip_str, ip);

	if (conf_set_attr(attr, ip_str) < 0)
		conf_add_attr(attr, ip_str);

	return 0;
}

#if 0
bool check_ip_addr(const char *ip, int ver)
{
	int dotn = 1;
	__u8 num;

	while (*ip && dotn < ver) {
		if (*ip == '.') {
			num = 0;
			dotn++;
			ip++;
		}

		if (*ip < '0' || *ip > '9')
				break;

		num += num * 10 + *ip - '0';
		if (num > 255)
			break;

		ip++;
	}

	return false;
}

static int __init net_init(void)
{
	void __init socket_init(void);

	socket_init();
	return 0;
}

subsys_init(net_init);
#endif
