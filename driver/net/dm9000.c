/*
 *  comment here
 */

#include <io.h>
#include <init.h>
#include <stdio.h>
#include <errno.h>
#include <net/net.h>
#include <net/mii.h>
#include <irq.h>
#include <delay.h>
#include <platform.h>
#include "dm9000.h"

// TODO: support 16-bit BW

#if 0
#define DM9K_INFO printf
#else
#define DM9K_INFO(args ...)
#endif

#define dm9000_readb(reg) \
	(writeb(dm9000->index_port, reg), readb(dm9000->data_port))

#define dm9000_writeb(reg, val) \
do { \
	writeb(dm9000->index_port, reg); \
	writeb(dm9000->data_port, val); \
} while (0)

struct dm9000_chip {
	void *index_port, *data_port;
	int irq;
};

static __u16 dm9000_mdio_read(struct net_device *ndev, __u8 addr, __u8 reg)
{
	__u16 val;
	struct dm9000_chip *dm9000 = ndev->chip;

	// dm9000_writeb(DM9000_EPAR, DM9000_PHY_INTER | reg);
	dm9000_writeb(DM9000_EPAR, (addr & 0x3) << 6 | reg);
	dm9000_writeb(DM9000_EPCR, DM9000_PHY_SELECT | DM9000_PHY_READ);

	while (dm9000_readb(DM9000_EPCR) & DM9000_PHY_BUSY);

	val = (dm9000_readb(DM9000_EPDRH) << 8) | dm9000_readb(DM9000_EPDRL);

	dm9000_writeb(DM9000_EPCR, 0x0);

	return val;
}

static void dm9000_mdio_write(struct net_device *ndev, __u8 addr, __u8 reg, __u16 val)
{
	struct dm9000_chip *dm9000 = ndev->chip;

	// dm9000_writeb(DM9000_EPAR, DM9000_PHY_INTER | reg);
	dm9000_writeb(DM9000_EPAR, (addr & 0x3) << 6 | reg);
	dm9000_writeb(DM9000_EPDRL, val & 0xff);
	dm9000_writeb(DM9000_EPDRH, (val >> 8) & 0xff);

	dm9000_writeb(DM9000_EPCR, DM9000_PHY_SELECT | DM9000_PHY_WRITE);

	while (dm9000_readb(DM9000_EPCR) & DM9000_PHY_BUSY);

	dm9000_writeb(DM9000_EPCR, 0x0);
}

static int dm9000_set_mac(struct net_device *ndev, const __u8 *mac)
{
	int i;
	struct dm9000_chip *dm9000 = ndev->chip;

	for (i = 0; i < MAC_ADR_LEN; i++)
		dm9000_writeb(DM9000_PAR + i, ndev->mac_addr[i]);

	return 0;
}

static int dm9000_reset(struct dm9000_chip *dm9000)
{
	int i;
	__u8 status;

	dm9000_writeb(DM9000_NCR, 1);
	while (dm9000_readb(DM9000_NCR) & 1);

	status = dm9000_readb(DM9000_ISR);
	dm9000_writeb(DM9000_ISR, status);

	dm9000_writeb(DM9000_TCR, 0);
	dm9000_writeb(DM9000_RCR, 0);
	// dm9000_writeb(DM9000_BPTR, 0x3f);
	// dm9000_writeb(DM9000_FCTR, 0x38);
	dm9000_writeb(DM9000_SMCR, 0);

	dm9000_writeb(DM9000_GPR, 0x00); //

	for (i = 0; i < 8; i++)
		dm9000_writeb(DM9000_MAR + i, 0xff);

	return 0;
}

static int dm9000_send_packet(struct net_device *ndev, struct sock_buff *skb)
{
	int i;
	const __u16 *tx_data;
	__u16 tx_size;
	__UNUSED__ __u32 flag;
	struct dm9000_chip *dm9000 = ndev->chip;

	lock_irq_psr(flag);

	writeb(VA(DM9000_INDEX_PORT), DM9000_MWCMD);

	tx_data = (const __u16 *)skb->data;
	tx_size = skb->size;

	for (i = 0; i < tx_size; i += sizeof(*tx_data)) {
		writew(VA(DM9000_DATA_PORT), *tx_data);
		tx_data++;
	}

	dm9000_writeb(DM9000_TXPLL, tx_size & 0xff);
	dm9000_writeb(DM9000_TXPLH, (tx_size >> 8) & 0xff);

	dm9000_writeb(DM9000_TCR, 1);

	while (dm9000_readb(DM9000_TCR) & 1);

	ndev->stat.tx_packets++;

	unlock_irq_psr(flag);

	return 0;
}

static int dm9000_recv_packet(struct net_device *ndev)
{
	int i;
	__u8 val;
	__u16 *rx_data;
	__u16 rx_stat, rx_size;
	struct sock_buff *skb;
	struct dm9000_chip *dm9000 = ndev->chip;

	while (1) {
		dm9000_readb(DM9000_MRCMDX);
		val = readb(VA(DM9000_DATA_PORT));

		if (val != 0x1) {
			if (0 == val)
				return 0;

			printf("\n%s(), line %d: wrong status = 0x%02x\n",
				__func__, __LINE__, val);

			dm9000_reset(dm9000);
			dm9000_writeb(DM9000_IMR, IMR_VAL);
			dm9000_writeb(DM9000_RCR, 1);

			return -EIO;
		}

		writeb(VA(DM9000_INDEX_PORT), DM9000_MRCMD);
		rx_stat = readw(VA(DM9000_DATA_PORT));
		rx_size = readw(VA(DM9000_DATA_PORT));

		// TODO: check the size
		DM9K_INFO("stat = 0x%04x, size = 0x%04x\n", rx_stat, rx_size);

		if ((rx_stat & 0xbf00) || (skb = skb_alloc(0, (rx_size + 1) & ~1)) == NULL) {
			for (i = 0; i < rx_size; i += 2)
				readw(VA(DM9000_DATA_PORT));

			printf("\n%s(), line %d error: status = 0x%04x, size = %d\n",
					__func__, __LINE__, rx_stat, rx_size);
		} else {
			rx_data  = (__u16 *)skb->data;  // or head;

			for (i = 0; i < rx_size; i += 2) {
				*rx_data = readw(VA(DM9000_DATA_PORT));
				DM9K_INFO("%04x", *rx_data);
				rx_data++;
			}

			skb->size -= 4;

			netif_rx(skb);

			ndev->stat.rx_packets++;
		}
	}

	return 0;
}

static int dm9000_link_change(struct net_device *ndev)
{
	__u8 link;
	__u16 stat;
	struct mii_phy *phy;
	struct dm9000_chip *dm9000 = ndev->chip;

	if (list_empty(&ndev->phy_list))
		return -EIO;
	phy = container_of(ndev->phy_list.next, struct mii_phy, phy_node);

	link = dm9000_readb(DM9000_NSR);
	if (link & (1 << 6)) {
		int to;

		ndev->link.connected = true;

		for (to = 0; to < 100; to++) {
			stat = ndev->mdio_read(ndev, phy->addr, MII_REG_BMS);
			if (stat & 0x20)
				break;

			udelay(10);
		}

		stat = ndev->mdio_read(ndev, phy->addr, MII_REG_STAT);
		switch (stat >> 12) {
		case 1:
			ndev->link.speed = ETHER_SPEED_10M_HD;
			break;

		case 2:
			ndev->link.speed = ETHER_SPEED_10M_FD;
			break;

		case 4:
			ndev->link.speed = ETHER_SPEED_100M_HD;
			break;

		case 8:
			ndev->link.speed = ETHER_SPEED_100M_FD;
			break;

		default:
			ndev->link.speed = ETHER_SPEED_UNKNOWN;
			break;
		}
	} else {
		ndev->link.connected = false;
	}

	ndev_link_change(ndev);

	return 0;
}

static int dm9000_isr(__u32 irq, void *dev)
{
	__u8 irq_stat, rx_stat;
	struct net_device *ndev = dev;
	struct dm9000_chip *dm9000;

	dm9000 = ndev->chip;

	irq_stat = dm9000_readb(DM9000_ISR);
	if (0 == irq_stat)
		return 0;

	dm9000_writeb(DM9000_ISR, irq_stat);

	DM9K_INFO("%s() line %d: irq_stat = 0x%08x\n",
		__func__, __LINE__, irq_stat);

	if (irq_stat & 0x1) {
		rx_stat = dm9000_readb(DM9000_RSR);
		if (rx_stat & 0xBF)
			printf("%s(): RX Status = 0x%02x\n", __func__, rx_stat);

		dm9000_recv_packet(ndev);
	}

	if (irq_stat & 0x20)
		dm9000_link_change(ndev);

	return 0;
}

#ifndef CONFIG_IRQ_SUPPORT
static int dm9000_poll(struct net_device *ndev)
{
	return dm9000_isr(CONFIG_DM9000_IRQ, ndev);
}
#endif

static int __init dm9000_probe(struct platform_device *pdev)
{
	int ret;
	__u16 ven_id, dev_id;
	__u8 rev;
	int irq;
	const char *chip_name;
	struct resource *index_res, *data_res;
	struct net_device *ndev;
	struct dm9000_chip *dm9000;

	index_res = platform_get_mem(pdev, 0);
	// if ...
	data_res = platform_get_mem(pdev, 1);
	// if ...
	irq = platform_get_irq(pdev, 0);

	ndev = ndev_new(sizeof(*dm9000));
	if (NULL == ndev)
		return -ENOMEM;

	dm9000 = ndev->chip;
	dm9000->index_port = VA(index_res->start);
	dm9000->data_port = VA(data_res->start);
	dm9000->irq = irq;

	ven_id = (dm9000_readb(DM9000_VIDH) << 8) | dm9000_readb(DM9000_VIDL);
	dev_id = (dm9000_readb(DM9000_PIDH) << 8) | dm9000_readb(DM9000_PIDL);

	if (ven_id != VENDOR_ID_DAVICOM) {
		printf("No DM9000X found! (ID = %04x:%04x)\n", ven_id, dev_id);
		ret = -ENODEV;
		goto error_id;
	}

	rev = dm9000_readb(DM9000_REV);
	// fixme: "DM9000B"
	switch (rev) {
	case 0x18:
	case 0x19:
		chip_name = "DM9000A";
		break;

	case 0x1A:
		chip_name = "DM9000C";
		break;

	default:
		chip_name = "DM9000E";
		break;
	}

	printf("%s: Vendor ID = 0x%04x, Product ID = 0x%04x, Rev = 0x%02x.\n",
		chip_name, ven_id, dev_id, rev);

	dm9000_reset(dm9000);

	ndev->chip_name    = chip_name;
	ndev->send_packet  = dm9000_send_packet;
	ndev->set_mac_addr = dm9000_set_mac;
	// MII
	ndev->phy_mask   = 2;
	ndev->mdio_read  = dm9000_mdio_read;
	ndev->mdio_write = dm9000_mdio_write;

#ifdef CONFIG_IRQ_SUPPORT
	ret = irq_request(irq, dm9000_isr, ndev);
#else
	ndev->ndev_poll = dm9000_poll;
#endif

	ret = ndev_register(ndev);
	if (ret < 0)
		goto fail_reg;

	// TODO:  try to manually update when RX/TX done
	dm9000_reset(dm9000);
	dm9000_writeb(DM9000_IMR, IMR_VAL);
	dm9000_writeb(DM9000_RCR, 1);

	return 0;

fail_reg:
#ifdef CONFIG_IRQ_SUPPORT
	// unregister_irq
#endif
error_id:
	ndev_del(ndev);

	return ret;
}

static struct platform_driver dm9000_driver = {
	.drv = {
		.name = "dm9000",
	},
	.probe = dm9000_probe,
};

static int __init dm9000_init(void)
{
	return platform_driver_register(&dm9000_driver);
}

module_init(dm9000_init);
