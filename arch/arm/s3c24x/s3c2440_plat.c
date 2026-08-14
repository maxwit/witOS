#include <io.h>
#include <irq.h>
#include <init.h>
#include <platform.h>
#include <arm/s3c24x0.h>

static struct resource dm9000_res[] = {
	[0] = {
		.start = 0x20000000,
		.size = 4,
		.flag = IORESOURCE_MEM,
	},
	[1] = {
		.start = 0x20000004,
		.size = 4,
		.flag = IORESOURCE_MEM,
	},
	[2] = {
		.start = IRQ_EINT7,
		.size = 1,
		.flag = IORESOURCE_IRQ,
	},
};

static struct platform_device dm9000_device = {
	.dev = {
		.resources = dm9000_res,
		.res_num = ARRAY_ELEM_NUM(dm9000_res),
	},
	.name = "dm9000",
};

static int __init s3c2440_init(void)
{
	__u32 val;

#if 1
	val = readl(VA(S3C24X0_GPBCON));
	val &= ~3;
	val |= 2;
	writel(VA(S3C24X0_GPBCON), val);
#endif

	// LED @ GPE12/13
	val = readl(VA(0x56000040));
	val &= ~0x0f000000;
	val |= 0x05000000;
	writel(VA(0x56000040), val);

	val = readl(VA(0x56000044));
	val |= 0x3000;
	writel(VA(0x56000044), val);

	// LED @ GPF4/5/6/7
	val = readl(VA(0x56000050));
	val &= ~0xff00;
	val |= 0x5500;
	writel(VA(0x56000050), val);

	val = readl(VA(0x56000054));
	val |= 0xf0;
	writel(VA(0x56000054), val);

#ifdef CONFIG_IRQ_SUPPORT
	s3c24x0_interrupt_init();

	// FIXME
	irq_set_trigger(IRQ_EINT9, IRQ_TYPE_RISING);
	irq_set_trigger(IRQ_EINT7, IRQ_TYPE_RISING);

#ifdef CONFIG_TIMER_SUPPORT
	s3c24x0_timer_init();
#endif
#endif

	platform_device_register(&dm9000_device);

	return 0;
}

plat_init(s3c2440_init);
