#pragma once

#include <types.h>
#include <list.h>

#define NET_NAME_LEN  16
#define MAC_ADR_LEN   6

struct sock_buff;

struct ndev_stat {
	__u32 rx_packets;
	__u32 tx_packets;
	__u32 tx_errors;
	__u32 rx_errors;
};

enum ether_speed {
	ETHER_SPEED_UNKNOWN,
	ETHER_SPEED_10M_HD,
	ETHER_SPEED_10M_FD,
	ETHER_SPEED_100M_HD,
	ETHER_SPEED_100M_FD,
	ETHER_SPEED_1000M_HD,
	ETHER_SPEED_1000M_FD,
};

struct link_status {
	bool connected;
	enum ether_speed speed;
};

struct net_device {
	const char *chip_name;
	char ifx_name[NET_NAME_LEN];
	struct list_head ndev_node;

	//
	__u32 ip;
	__u32 mask;
	__u8  mac_addr[MAC_ADR_LEN];
	void *chip;

	struct ndev_stat stat;
	struct link_status link;

	int (*set_mac_addr)(struct net_device *ndev, const __u8 mac[]);
	// transmit & receive
	int (*send_packet)(struct net_device *ndev, struct sock_buff *skb);
#ifndef CONFIG_IRQ_SUPPORT
	int (*ndev_poll)(struct net_device *ndev);
#endif

	// PHY
	__u32 phy_mask;
	struct list_head phy_list;
	__u16 (*mdio_read)(struct net_device *ndev, __u8 addr, __u8 reg);
	void (*mdio_write)(struct net_device *ndev, __u8 addr, __u8 reg, __u16 val);
};

struct net_device *ndev_new(size_t chip_size);

void ndev_del(struct net_device *ndev);

int ndev_register(struct net_device *ndev);

int netif_rx(struct sock_buff *skb);

#ifndef CONFIG_IRQ_SUPPORT
int ndev_poll();
#else
static inline int ndev_poll()
{
	return 0;
}
#endif

void ndev_link_change(struct net_device *ndev);

// fix the following 2 APIs
struct list_head *ndev_get_list(void);
struct net_device *ndev_get_first();

#define NIOC_GET_IP     1
#define NIOC_SET_IP     2
#define NIOC_GET_MASK   3
#define NIOC_SET_MASK   4
#define NIOC_GET_MAC    5
#define NIOC_SET_MAC    6
#define NIOC_GET_LINK   7
#define NIOC_GET_STAT   8

int ndev_ioctl(struct net_device *ndev, int cmd, void *arg);
