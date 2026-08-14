#pragma once

#include <types.h>
#include <list.h>
#include <net/ndev.h>
#include <net/skb.h>
#include <net/socket.h>

#define PROT_ICMP 1
#define PROT_IGMP 2
#define PROT_TCP  6
#define PROT_UDP  17
#define PROT_OSPF 89
#define PROT_ETH	10 //FIXME

#define ETH_TYPE_IP   CPU_TO_BE16(0x0800)
#define ETH_TYPE_ARP  CPU_TO_BE16(0x0806)
#define ETH_TYPE_RARP CPU_TO_BE16(0x0835)

#define IPV4_STR_LEN    16
#define IPV4_ADR_LEN    4
#define MAC_STR_LEN	    18

#define ETH_HDR_LEN     14
#define ARP_PKT_LEN     28
#define IP_HDR_LEN      20

#define UDP_HDR_LEN     8
#define MAX_ETH_LEN     (1500 + ETH_HDR_LEN)
#define MIN_ETH_LEN     46

#define TCP_HDR_LEN     20

// ping
#define PING_PACKET_LENGTH      64
#define PING_MAX_TIMES          3
#define PING_DEFUALT_PID        1

// std ports:
#define STD_PORT_TFTP   69

#define MKIP(a, b, c, d) ((d << 24) | (c << 16) | (b << 8) | a)

struct socket;
struct sockaddr;

//
struct ether_header {
	__u8 des_mac[MAC_ADR_LEN];
	__u8 src_mac[MAC_ADR_LEN];
	__u16 frame_type;
};

//
#define ARP_OP_REQ  CPU_TO_BE16(1)
#define ARP_OP_REP  CPU_TO_BE16(2)

struct arp_packet {
	__u16  hard_type;
	__u16  prot_type;
	__u8   hard_size;
	__u8   prot_size;
	__u16  op_code;
	__u8   src_mac[MAC_ADR_LEN];
	__u8   src_ip[IPV4_ADR_LEN];
	__u8   des_mac[MAC_ADR_LEN];
	__u8   des_ip[IPV4_ADR_LEN];
} __PACKED__;

//
struct ip_header {
	__u8   ver_len;
	__u8   tos;
	__u16  total_len;
	__u16  id;
	__u16  flag_frag;
	__u8   ttl;
	__u8   up_prot;
	__u16  chksum;
	__u8   src_ip[IPV4_ADR_LEN];
	__u8   des_ip[IPV4_ADR_LEN];
};

//
#define ICMP_TYPE_ECHO_REPLY         0
#define ICMP_TYPE_DEST_UNREACHABLE   3
#define ICMP_TYPE_ECHO_REQUEST       8

//
struct icmp_packet {
	__u8  type;
	__u8  code;
	__u16 chksum;
};

//
struct ping_packet {
	__u8  type;
	__u8  code;
	__u16 chksum;
	__u16 id;
	__u16 seqno;
};

//
struct udp_header {
	__u16 src_port;
	__u16 dst_port;
	__u16 udp_len;
	__u16 checksum;
};

#define FLG_FIN  (1 << 0)
#define FLG_SYN  (1 << 1)
#define FLG_RST  (1 << 2)
#define FLG_PSH  (1 << 3)
#define FLG_ACK  (1 << 4)
#define FLG_URG  (1 << 5)

struct tcp_header {
	__u16 src_port;
	__u16 dst_port;
	__u32 seq_num;
	__u32 ack_num;
#if 0
	__u16 reserve1:4;
	__u16 hdr_len:4;
	__u16 flg_fin:1;
	__u16 flg_syn:1;
	__u16 flg_rst:1;
	__u16 flg_psh:1;
	__u16 flg_ack:1;
	__u16 flg_urg:1;
	__u16 reserve2:2;
#else
#ifdef __LITTLE_ENDIAN
	__u8 reserve: 4;
	__u8 hdr_len: 4;
#else
	__u8 hdr_len: 4;
	__u8 reserve: 4;
#endif
	__u8 flags;
#endif
	__u16 win_size;
	__u16 checksum;
	__u16 urg_ptr;
	__u8  options[0];
};

#define MAX_TCP_OPT_LEN 64

struct tcp_option {
	__u8 opt[MAX_TCP_OPT_LEN];
	__u16 len;
};

struct eth_addr {
	__u8  ip[4];
	__u8  mac[6];
};

struct eth_addr *getaddr(__u32 nip);
struct eth_addr *gethostaddr(const __u32 nip);

void arp_send_packet(const __u8 nip[], const __u8 *mac, __u16 op_code);
int ip_send_packet(struct sock_buff *skb, __u8 proto);
void udp_send_packet(struct sock_buff *skb);
void tcp_send_packet(struct sock_buff *skb, __u8 flags, struct tcp_option *opt);

int ip_layer_deliver(struct sock_buff *skb);

__u16 net_calc_checksum(const void *buff, __u32 size);

int net_get_server_ip(__u32 *ip);

int net_set_server_ip(__u32 ip);

struct socket *arp_search_socket(const struct arp_packet *arp_pkt);
