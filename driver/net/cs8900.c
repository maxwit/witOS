#include <irq.h>
#include <net/net.h>
#include "cs8900.h"

static inline __u16 cs8900_inw(__u16 reg)
{
	writew(VA(CS8900_IOBASE + 0x0A), reg);
	return readw(VA(CS8900_IOBASE + 0x0C));
}

static inline void cs8900_outw(__u16 reg, __u16 val)
{
	writew(VA(CS8900_IOBASE + 0x0A), reg);
	writew(VA(CS8900_IOBASE + 0x0C), val);
}

static int cs8900_reset(void)
{
	int i;
	__u16 val;

	val = cs8900_inw(PP_SelfCTL);
	val |= 0x40;
	cs8900_outw(PP_SelfCTL, val);

	for (i = 0; i < 100; i++) {
		if(!(cs8900_inw(PP_SelfCTL) & 0x40))
			return 0;

		udelay(1000);
	}

	return -ETIMEDOUT;
}

static int cs89x0_send_packet(struct net_device *ndev, struct sock_buff *skb)
{
	int i;
	__u16 isq_stat;
	const __u16 *buff;
	__UNUSED__ __u32 psr;

	lock_irq_psr(psr);

	writew(VA(CS8900_IOBASE + CS_TxCMD), 0x00);
	writew(VA(CS8900_IOBASE + CS_TxLen), skb->size);

	while (1) {
		isq_stat = cs8900_inw(PP_BusST);
		if (isq_stat & Rdy4TxNow)
			break;

		printf("BusST = 0x%04x\n", isq_stat);
	}

	buff = (const __u16 *)skb->data;

	for (i = 0; i < skb->size; i += 2) {
		writew(VA(CS8900_IOBASE + CS_DATA0), *buff);
		buff++;
	}

	ndev->stat.tx_packets++;

	unlock_irq_psr(psr);

	return 0;
}

static int cs89x0_set_mac(struct net_device *ndev, const __u8 mac[])
{
	int i;

	for (i = 0; i < 6; i += 2)
		cs8900_outw(0x158 + i, *(__u16 *)(mac + i));

	return 0;
}

static int cs89x0_isr(__u32 irq, void *dev)
{
	int i;
	__u16 isq_stat;
	__u16 rx_stat, rx_size;
	__u16 *buff;
	struct sock_buff *skb;
	// struct net_device *ndev = dev;

	while ((isq_stat = readw(VA(CS8900_IOBASE + CS_ISQ)))) {
		int regn = isq_stat & 0x3F;

		if (regn == 4) {
			rx_stat = readw(VA(CS8900_IOBASE + 0x00));
			rx_size = readw(VA(CS8900_IOBASE + 0x00));

			DPRINT("isq_stat = 0x%04x, size = 0x%04x (%d)\n",
				rx_stat, rx_size, rx_size);

			skb = skb_alloc(0, rx_size);
			// if NULL
			buff = (__u16 *)skb->data;

			for (i = 0; i < rx_size; i += 2) {
				*buff = readw(VA(CS8900_IOBASE + 0x00));
				buff++;
			}

			netif_rx(skb);
		}
	}

	return 0;
}

#ifndef CONFIG_IRQ_SUPPORT
static int cs89x0_poll(struct net_device *ndev)
{
	int i;
	__u16 rx_event;
	__u16 rx_stat, rx_size;
	__u16 *buff;
	struct sock_buff *skb;

	rx_event = cs8900_inw(0x0124);
	if (!(rx_event & 0x100))
		return 0;

	// printf("rx event = 0x%04x\n", rx_event);

	rx_stat = readw(VA(CS8900_IOBASE + 0x00));
	rx_size = readw(VA(CS8900_IOBASE + 0x00));

	DPRINT("rx_event = 0x%04x, size = 0x%04x (%d)\n",
		rx_stat, rx_size, rx_size);

	skb = skb_alloc(0, rx_size);
	// if NULL
	buff = (__u16 *)skb->data;

	for (i = 0; i < rx_size; i += 2) {
		*buff = readw(VA(CS8900_IOBASE + 0x00));
		buff++;
	}

	netif_rx(skb);

	ndev->stat.rx_packets++;

	return 0;
}
#endif

static int __init cs89x0_init(void)
{
	int ret;
	__u16 ven_id, dev_id, val, line_st;
	struct net_device *ndev;

	ven_id = cs8900_inw(0x00);
	dev_id = cs8900_inw(0x02);

	printf("CS8900: vendor = 0x%04x, device = 0x%04x\n",
		ven_id, dev_id);

	ret = cs8900_reset();
	if (ret < 0) {
		printf("CS8900 reset failed!\n");
		return ret;
	}

	ndev = ndev_new(0);
	// if NULL

	ndev->chip_name = "CS8900A";
	ndev->phy_mask = 0;
	ndev->send_packet = cs89x0_send_packet;
	ndev->set_mac_addr = cs89x0_set_mac;
#ifndef CONFIG_IRQ_SUPPORT
	ndev->ndev_poll   = cs89x0_poll;
#endif

	irq_request(CONFIG_CS8900_IRQ, cs89x0_isr, ndev);

	ret = ndev_register(ndev);

	cs8900_outw(PP_RxCTL, 0x3FC5);

	cs8900_outw(PP_LineCTL, 0xD3);

	val = cs8900_inw(PP_BusCTL);
	val |= 1 << 15;
	cs8900_outw(PP_BusCTL, val);

	val = cs8900_inw(PP_RxCFG);
	val |= 0x7100;
	cs8900_outw(PP_RxCFG, val);

	cs8900_outw(PP_INTN, 0x0);

	while (1) {
		line_st = cs8900_inw(PP_LineST);
		printf("line line_st = 0x%04x\n", line_st);
		if (line_st & 0x80)
			break;
	}

	return ret;
}

module_init(cs89x0_init);

