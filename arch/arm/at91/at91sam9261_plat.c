#include <io.h>
#include <init.h>
#include <platform.h>
#include <arm/at91sam926x.h>

// FIXME
#define CONFIG_DM9000_PIO_RESET  PIOC
#define CONFIG_DM9000_PIN_RESET  (1 << 10)

#define CONFIG_DM9000_PIO_IRQ    PIOC
#define CONFIG_DM9000_PIN_IRQ    (1 << 11)

#if 1 // to be removed
#define CONFIG_DM9000_IRQ        (32 + 2 * 32 + 11)
#define DM9000_PHYS_BASE         0x30000000
#define DM9000_INDEX_PORT        (DM9000_PHYS_BASE + 0x0)
#define DM9000_DATA_PORT         (DM9000_PHYS_BASE + 0x4)
#endif

// FIXME: to be checked
static struct resource dm9000_res[] = {
	[0] = {
		.start = DM9000_PHYS_BASE,
		.size = 4,
		.flag = IORESOURCE_MEM,
	},
	[1] = {
		.start = DM9000_PHYS_BASE + 4,
		.size = 4,
		.flag = IORESOURCE_MEM,
	},
	[2] = {
		.start = CONFIG_DM9000_IRQ,
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

static int __init at91sam9261_init(void)
{
	writel(VA(AT91SAM926X_PA_PMC + PMC_PCER), 0x11);

	writel(VA(AT91SAM926X_PA_SMC + SMC_SETUP(2)), 2 << 16 | 2);
	writel(VA(AT91SAM926X_PA_SMC + SMC_PULSE(2)), 0x08040804);
	writel(VA(AT91SAM926X_PA_SMC + SMC_CYCLE(2)), 16 << 16 | 16);
	writel(VA(AT91SAM926X_PA_SMC + SMC_MODE(2)), 1 << 16 | 0x1103);

	at91_gpio_conf_output(CONFIG_DM9000_PIO_RESET, CONFIG_DM9000_PIN_RESET, 0);

#ifdef CONFIG_IRQ_SUPPORT
	at91_gpio_conf_input(CONFIG_DM9000_PIO_IRQ, CONFIG_DM9000_PIN_IRQ, 0);
	readl(VA(PIO_BASE(CONFIG_DM9000_PIO_IRQ) + PIO_ISR));

	at91sam926x_interrupt_init();

#ifdef CONFIG_TIMER_SUPPORT
	at91sam926x_timer_init();
#endif
#endif

	platform_device_register(&dm9000_device);

	return 0;
}

plat_init(at91sam9261_init);
