#include <io.h>
#include <irq.h>
#include <init.h>
#include <board.h>
#include <platform.h>

#ifdef CONFIG_SMSC91X
static int __init smsc91x_device_init(struct platform_device *pdev)
{
	__u32 val;

	val = readl(VA(GPMC_CONFIG1(1)));
	val &= ~(0x3 << 10);
	val &= ~(0x1 << 9);
	val |= 0x1;
	writel(VA(GPMC_CONFIG1(1)), val);

#if 0
	val = 0xF << 8 | 0x1 << 6; // | 0x8;
	writel(VA(GPMC_CONFIG7(1)), val);
#endif

	return 0;
}

//  FIXME: __initdata
static struct resource smsc91x_res[] = {
	[0] = {
		.start = 0x28000000, // FIXME
		.size = 4,
		.flag = IORESOURCE_MEM,
	},
	[1] = {
		.start = GPIO_IRQ(19),
		.size = 1,
		.flag = IORESOURCE_IRQ,
	},
};

static struct platform_device smsc91x_device = {
	.dev = {
		.resources = smsc91x_res,
		.res_num = ARRAY_ELEM_NUM(smsc91x_res),
	},
	.name = "SMSC SMSC91X",
	.init = smsc91x_device_init,
};
#endif

// FIXME: __INITDATA__
static struct platform_device *beagle_devices[] = {
#ifdef CONFIG_SMSC91X
	&smsc91x_device,
#endif
};

static int __init beagle_init(struct board_desc *board)
{
	int i, ret;

	for (i = 0; i < ARRAY_ELEM_NUM(beagle_devices); i++) {
		ret = platform_device_register(beagle_devices[i]);
		if (ret < 0)
			return ret;
	}

	return 0;
}

BOARD_DESC("beagle", 1546, beagle_init);
