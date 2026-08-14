#include <io.h>
#include <irq.h>
#include <init.h>
#include <platform.h>

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

static int __init s3c2410_init(void)
{
#ifdef CONFIG_IRQ_SUPPORT
	s3c24x0_interrupt_init();
	// FIXME
	irq_set_trigger(IRQ_EINT9, IRQ_TYPE_RISING);

#ifdef CONFIG_TIMER_SUPPORT
	s3c24x0_timer_init();
#endif
#endif

	platform_device_register(&dm9000_device);

	return 0;
}

plat_init(s3c2410_init);
