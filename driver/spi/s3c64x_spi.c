#include <io.h>
#include <spi.h>
#include <init.h>
#include <delay.h>
#include <stdio.h>

#define USE_PKG_CNT
#define STEP_LEN 1

static void s3c6410_spi_reset(struct spi_master *master)
{
	int i;
	__u32 val;

	writel(VA(PACKET_CNT_REG), 0);

	val = 1 << 5;
	writel(VA(CH_CFG), val);

	for (i = 0; i < 10; i++) {
		val = readl(VA(SPI_STATUS));
		printf("%s() line %d: status = 0x%08x (rx = 0x%x, tx = 0x%x)\n",
			__func__, __LINE__, val, (val >> 13) & 0x7F, (val >> 6) & 0x7F);

		if (!(val & (0x3FFF << 6)))
			break;

		udelay(10);
	}

	val = readl(VA(CH_CFG));
	val &= ~(1 << 5);
	writel(VA(CH_CFG), val);

#if 0
	val |= 3 << 2;
#else
	val &= ~(3 << 2);
#endif
	writel(VA(CH_CFG), val);

	val = 0x3ff << 19;
	writel(VA(MODE_CFG), val);
}

static void nor_chip_select(void)
{
#if 0
	__u32 val;

	val = readl(VA(CS_REG));
	val &= ~0x1;
	writel(VA(CS_REG), val);
#else
	writel(VA(CS_REG), 0);
#endif
}

static void nor_chip_unselect(void)
{
#if 0
	__u32 val;

	val = readl(VA(CS_REG));
	val |= 0x1;
	writel(VA(CS_REG), val);
#else
	writel(VA(CS_REG), 1);
#endif

}

static int s3c6410_spi_transfer(struct spi_slave *slave)
{
	enum TX_TYPE{TX, RX};
	struct spi_trans_msg *trans;
	__u32 i, j, val;
	__u8 *tx_buf = NULL;
	__u32 len;
	__u8 *rx_buf = NULL;
	struct list_head *msg_qu = &slave->msg_qu;

	nor_chip_select();

	for (msg_qu = msg_qu->next; msg_qu != msg_qu->next; msg_qu = msg_qu->next) {
		trans  = container_of(msg_qu, struct spi_trans_msg, msg_node);
		tx_buf = trans->tx_buf;
		rx_buf = trans->rx_buf;
		len    = trans->len;

		printf("tx_buf:%p, rx_buf:%p, %d\n", tx_buf, rx_buf, len);
		if (NULL != tx_buf) {
			// write
		#ifdef USE_PKG_CNT
			// really work?
			writel(VA(PACKET_CNT_REG), 1 << 16 | len);
		#endif
			for (i = 0; i < len; i++)
				writeb(VA(SPI_TX_DATA), tx_buf[i]);

			val = readl(VA(CH_CFG));
			val |= 0x1;
			writel(VA(CH_CFG), val);

			// FIXME
			for (i = 0; i < 10; i++) {
				val = readl(VA(SPI_STATUS));
				printf("%s() line %d: status = 0x%08x (rx = 0x%x, tx = 0x%x)\n",
					__func__, __LINE__, val, (val >> 13) & 0x7F, (val >> 6) & 0x7F);

				//if (((val >> 13) & 0x7F) >= n_tx)
				if (val & (1 << 21))
					break;

				udelay(10);
			}
		}

		if (NULL != rx_buf) {
			// read
			for (i = 0; i < len; i += STEP_LEN) {
				__u32 irx;

		#ifdef USE_PKG_CNT
				writel(VA(PACKET_CNT_REG), 1 << 16 | STEP_LEN);
		#else
				for (j = 0; j < STEP_LEN; j++)
					writeb(VA(SPI_TX_DATA), 0);
		#endif

				val = readl(VA(CH_CFG));
				val &= ~3;
		#ifdef USE_PKG_CNT
				val |= 0x2;
		#else
				val |= 0x3;
		#endif
				writel(VA(CH_CFG), val);

				// FIXME
				for (j = 0; j < 10; j++) {
					val = readl(VA(SPI_STATUS));
					irx = (val >> 13) & 0x7F;
					printf("%s() line %d: status = 0x%08x (irx = 0x%x, tx = 0x%x)\n",
						__func__, __LINE__, val, (val >> 13) & 0x7F, (val >> 6) & 0x7F);

					if (irx >= STEP_LEN) {
						// if irx > STEP_LEN ...
						break;
					}

					udelay(10);
				}

				for (j = 0; j < irx; j++, rx_buf++) {
					*rx_buf = readb(VA(SPI_RX_DATA));
					printf(" %02x", *rx_buf);
				}
				printf("\n");
				s3c6410_spi_reset(NULL); // FIXME
			}
		}

		s3c6410_spi_reset(NULL); // FIXME
		list_del(msg_qu);
	}

	nor_chip_unselect();

	return 0;
}

static int __init s3c6410_spi_init(void)
{
	// __u32 val;
	struct spi_master *master;

	writel(VA(CLK_CFG), 0x1 << 8 | 0x1);

	// probe the spi master:
	// get master object from board code
	// master = ...

	master = get_spi_master("s3c6410_spi0");

	master->transfer = s3c6410_spi_transfer;

	s3c6410_spi_reset(master);

	spi_master_register(master);

	return 0;
}

postsubs_init(s3c6410_spi_init);
