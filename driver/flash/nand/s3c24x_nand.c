#include <io.h>
#include <init.h>
#include <errno.h>
#include <malloc.h>
#include <mtd/nand.h>

static struct nand_oob_layout g_s3c24x_oob16_layout =
{
	.ecc_code_len = 3,
	.ecc_pos    = {0, 1, 2},
	.free_region = {{8, 8}}
};

static struct nand_oob_layout g_s3c24x_oob64_layout =
{
	.ecc_code_len = 24,
	.ecc_pos =
	{
	   40, 41, 42, 43, 44, 45, 46, 47,
	   48, 49, 50, 51, 52, 53, 54, 55,
	   56, 57, 58, 59, 60, 61, 62, 63
	},
	.free_region = {{2, 38}}
};

static int s3c24x_nand_init(struct nand_ctrl *nfc);

static int s3c24x_nand_is_ready(struct nand_chip *nand);

static void s3c24x_nand_read_buff(struct nand_ctrl *nfc, __u8 *buff, int size);
static void s3c24x_nand_write_buff(struct nand_ctrl *nfc, const __u8 *buff, int size);

static void s3c24x_nand_enable_hwecc(struct nand_chip *nand, int mode);
static int s3c24x_nand_calc_hwecc(struct nand_chip *nand, const __u8 *data, __u8 *ecc);
static int s3c24x_nand_correct_data(struct nand_chip *nand,
								__u8 *data, __u8 *ecc_read, __u8 *ecc_calc);

static int __init s3c24x_nand_probe(void);

static int s3c24x_nand_is_ready(struct nand_chip *nand)
{
	return readb(VA(NAND_CTRL_BASE + NF_STAT)) & 0x01;
}

static void  s3c24x_nand_read_buff(struct nand_ctrl *nfc, __u8 *buff, int size)
{
	while (size > 3) {
		*(__u32 *)buff = readl(VA(NAND_CTRL_BASE + NF_DATA));

		buff += 4;
		size -= 4;
	}

	while (size > 0) {
		*buff = readb(VA(NAND_CTRL_BASE + NF_DATA));

		buff++;
		size--;
	}
}

static void  s3c24x_nand_write_buff(struct nand_ctrl *nfc, const __u8 *buff, int size)
{
	while (size > 3) {
		writel(VA(NAND_CTRL_BASE + NF_DATA), *(__u32 *)buff);

		buff += 4;
		size -= 4;
	}

	while (size > 0) {
		writeb(VA(NAND_CTRL_BASE + NF_DATA), *buff);

		buff++;
		size--;
	}
}

static void s3c24x_nand_enable_hwecc(struct nand_chip *nand, int mode)
{
	__u32 val;

#ifdef CONFIG_S3C2410
	val = readl(VA(NAND_CTRL_BASE + NF_CONF));
	val |= 1 << 12;
	writel(VA(NAND_CTRL_BASE + NF_CONF), val);
#elif defined(CONFIG_S3C2440) || defined(CONFIG_S3C6410)
	val = readl(VA(NAND_CTRL_BASE + NF_CONT));
	val |= 1 << 5 | 1 << 4;
	writel(VA(NAND_CTRL_BASE + NF_CONT), val);
#else // FIXME
	#error
#endif
}

static int s3c24x_nand_calc_hwecc(struct nand_chip *nand, const __u8 *data, __u8 *ecc)
{
#ifdef CONFIG_S3C2410
	ecc[0] = readb(VA(NAND_CTRL_BASE + NF_ECC + 0));
	ecc[1] = readb(VA(NAND_CTRL_BASE + NF_ECC + 1));
	ecc[2] = readb(VA(NAND_CTRL_BASE + NF_ECC + 2));
#elif defined(CONFIG_S3C2440) || defined(CONFIG_S3C6410)
	__u32 val;

#if defined(CONFIG_S3C2440)
	val = readl(VA(NAND_CTRL_BASE + NF_ECC0));
#elif defined(CONFIG_S3C6410)
	val = readl(VA(NAND_CTRL_BASE + NF_MECC0));
#endif

	ecc[0] = val & 0xFF;
	ecc[1] = (val >> 8) & 0xFF;
	ecc[2] = (val >> 16) & 0xFF;
#endif

	return 0;
}

static int s3c24x_nand_correct_data(struct nand_chip *nand,
				__u8 *data, __u8 *ecc_read, __u8 *ecc_calc)
{
	__u32 diff0, diff1, diff2;

	diff0 = ecc_read[0] ^ ecc_calc[0];
	diff1 = ecc_read[1] ^ ecc_calc[1];
	diff2 = ecc_read[2] ^ ecc_calc[2];

	if (diff0 == 0 && diff1 == 0 && diff2 == 0)
		return 0;

	// TODO:  add data correction code here

	return -EIO;
}

static int s3c24x_nand_init(struct nand_ctrl *nfc)
{
#ifdef CONFIG_S3C2410
	writel(VA(NAND_CTRL_BASE + NF_CONF), 0x9333);
#elif defined(CONFIG_S3C2440)
	writel(VA(NAND_CTRL_BASE + NF_CONF), 0x2440);
	writel(VA(NAND_CTRL_BASE + NF_CONT), 0x1);
#elif defined(CONFIG_S3C6410)
	writel(VA(NAND_CTRL_BASE+ NF_CONF), 0 << 12 | 2 << 8 | 1 << 4 | 0x4);
	writel(VA(NAND_CTRL_BASE + NF_CONT), 0x1);
#endif

	return 0;
}

static int __init s3c24x_nand_probe(void)
{
	int i, ret;
#ifdef CONFIG_S3C2410
	__u32 stat;
#endif
	struct nand_ctrl *nfc;
	struct nand_chip *nand;

	nfc = nand_ctrl_new();
	if (NULL == nfc)
		return -ENOMEM;

// FIXME
#ifdef CONFIG_S3C2410
	nfc->name = "s3c2410_nand";
#elif defined(CONFIG_S3C2440)
	nfc->name = "s3c2440_nand";
#elif defined(CONFIG_S3C6410)
	nfc->name = "s3c6410_nand";
#endif

	nfc->cmmd_reg = VA(NAND_CTRL_BASE + NF_CMMD);
	nfc->addr_reg = VA(NAND_CTRL_BASE + NF_ADDR);
	nfc->data_reg = VA(NAND_CTRL_BASE + NF_DATA);

	nfc->flash_ready  = s3c24x_nand_is_ready;
	nfc->read_buff    = s3c24x_nand_read_buff;
	nfc->write_buff   = s3c24x_nand_write_buff;
	// ECC
	nfc->ecc_enable   = s3c24x_nand_enable_hwecc;
	nfc->ecc_generate = s3c24x_nand_calc_hwecc;
	nfc->ecc_correct  = s3c24x_nand_correct_data;

	s3c24x_nand_init(nfc);

	for (i = 0; i < nfc->max_slaves; i++) { //
		nand = nand_probe(nfc, i);
		if (NULL == nand)
			break; //: or conitnue

		if (NAND_TO_FLASH(nand)->write_size < KB(2)) {
			nfc->hard_oob_layout = &g_s3c24x_oob16_layout;
			nfc->ecc_data_len    = 512;
		} else {
			nfc->hard_oob_layout = &g_s3c24x_oob64_layout;
			nfc->ecc_data_len    = 256;
		}
		nfc->ecc_code_len = 3;

		ret = nand_register(nand);
		if (ret < 0)
			goto L1;
	}

	return 0;
L1:

#ifdef CONFIG_S3C2410 // FIXME
	// disable nand
	stat = readl(VA(NAND_CTRL_BASE + NF_CONF));
	stat &= ~(1 << 15);
	writel(VA(NAND_CTRL_BASE + NF_CONF), stat);
#endif

	free(nfc);
	return ret;
}

module_init(s3c24x_nand_probe);
