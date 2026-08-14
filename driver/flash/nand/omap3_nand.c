#include <io.h>
#include <init.h>
#include <delay.h>
#include <errno.h>
#include <malloc.h>
#include <mtd/mtd.h>
#include <mtd/nand.h>

static int omap3_nand_ready(struct nand_chip *nand)
{
	return readl(VA(GPMC_BASE + GPMC_STATUS)) & (1 << 8);
}

static void omap3_nand_enable_hwecc(struct nand_chip *nand, int mode)
{
	__u32 val;

	writel(VA(0x6E000000 + 0x1F8), 0x101);

	val = readl(VA(0x6E000000 + 0x1F4));
	val = 1 << 7 | 0x1;
	writel(VA(0x6E000000 + 0x1F4), val);
}

#define ECC_P1_128_E(val)    ((val)  & 0x000000FF)      /* Bit 0 to 7 */
#define ECC_P512_2048_E(val) (((val) & 0x00000F00)>>8)  /* Bit 8 to 11 */
#define ECC_P1_128_O(val)    (((val) & 0x00FF0000)>>16) /* Bit 16 to Bit 23 */
#define ECC_P512_2048_O(val) (((val) & 0x0F000000)>>24) /* Bit 24 to Bit 27 */

static int omap3_nand_calc_hwecc(struct nand_chip *nand, const __u8 *data, __u8 *ecc)
{
	__u32 val;

	val = readl(VA(0x6E000000 + 0x200));

	ecc[0] = ECC_P1_128_E(val);
	ecc[1] = ECC_P1_128_O(val);
	ecc[2] = ECC_P512_2048_E(val) | ECC_P512_2048_O(val) << 4;

	return 0;
}

static inline __u32 gen_true_ecc(__u8 *ecc_buf)
{
	return ecc_buf[0] | (ecc_buf[1] << 16) | \
		(ecc_buf[2] & 0xF0) << 20 | (ecc_buf[2] & 0x0F) << 8;
}

static int omap3_nand_correct_data(struct nand_chip *nand,
				__u8 *data, __u8 *ecc_read, __u8 *ecc_calc)
{
	__u32 ecc_old, ecc_new;

	ecc_old = gen_true_ecc(ecc_read);
	ecc_new = gen_true_ecc(ecc_calc);

	if (!(ecc_old ^ ecc_new))
		return 0;

	// TODO:  add data correction code here
	return -EIO;
}

static int __init omap3_nand_init(struct nand_ctrl *nfc)
{
	writel(VA(GPMC_BASE + SYSCONFIG), 0x10);
	writel(VA(GPMC_BASE + IRQENABLE), 0x0);
	writel(VA(GPMC_BASE + TIMEOUT), 0x0);
	writel(VA(GPMC_BASE + GPMC_CONFIG_CS0 + GPMC_CONFIG_7), 0x0);

	udelay(0x100);

	writel(VA(GPMC_BASE + GPMC_CONFIG_CS0 + GPMC_CONFIG_1), 0x1800);
	writel(VA(GPMC_BASE + GPMC_CONFIG_CS0 + GPMC_CONFIG_2), 0x00141400);
	writel(VA(GPMC_BASE + GPMC_CONFIG_CS0 + GPMC_CONFIG_3), 0x00141400);
	writel(VA(GPMC_BASE + GPMC_CONFIG_CS0 + GPMC_CONFIG_4), 0x0F010F01);
	writel(VA(GPMC_BASE + GPMC_CONFIG_CS0 + GPMC_CONFIG_5), 0x010C1414);
	writel(VA(GPMC_BASE + GPMC_CONFIG_CS0 + GPMC_CONFIG_6), 0x1F0F0A80);
	writel(VA(GPMC_BASE + GPMC_CONFIG_CS0 + GPMC_CONFIG_7),
		(0x8 & 0xf) << 8 | ((0x0c000000 >> 24) & 0x3f) | 1 << 6);

	udelay(0x100);

	// HW ECC init
	if (nfc->ecc_mode == NAND_ECC_HW) {
	    writel(VA(0x6E000000 + 0x1F8), 0x101);
	    writel(VA(0x6E000000 + 0x1FC), 0x3fcff000);
	}

	return 0;
}

static struct nand_oob_layout g_omap3_oob64_layout = {
	.ecc_code_len = 24,
	.ecc_pos = {
	   2, 3, 4, 5, 6, 7, 8, 9,
	   10, 11, 12, 13, 14, 15, 16, 17,
	   18, 19, 20, 21, 22, 23, 24, 25
	},
	.free_region = {{26, 38}}
};

static int __init omap3_nand_probe(void)
{
	int ret;
	struct nand_ctrl *nfc;

	nfc = nand_ctrl_new();
	if (NULL == nfc)
		return -ENOMEM;

	nfc->cmmd_reg = VA(GPMC_BASE + GPMC_NAND_COMMAND_0);
	nfc->addr_reg = VA(GPMC_BASE + GPMC_NAND_ADDRESS_0);
	nfc->data_reg = VA(GPMC_BASE + GPMC_NAND_DATA_0);

	nfc->name = "omap2-nand"; // FIXME
	nfc->flash_ready  = omap3_nand_ready;
	// ECC
	nfc->ecc_data_len = 512;
	nfc->ecc_code_len = 3;
	nfc->hard_oob_layout = &g_omap3_oob64_layout;
	nfc->ecc_enable   = omap3_nand_enable_hwecc;
	nfc->ecc_generate = omap3_nand_calc_hwecc;
	nfc->ecc_correct  = omap3_nand_correct_data;

	omap3_nand_init(nfc);

	ret = nand_ctrl_register(nfc);
	if (ret < 0) {
		ret = -ENODEV;
		goto L1;
	}

	return 0;

L1:
	free(nfc);
	return ret;
}

module_init(omap3_nand_probe);
