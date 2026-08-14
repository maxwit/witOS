#include <io.h>
#include <init.h>
#include <arm/at91sam926x.h>

#define CONFIG_PIO_PC10 (1 << 10)

static int __init at91sam9263_init(void)
{
	__u32 mask;

	at91_gpio_conf_input(PIOE, 1 << 31, 0);

	at91_gpio_conf_periB(PIOC, 1 << 25, 0);	// ERXDV

	mask = 1 << 21 | // ETXCK_EREFCK
		1 << 23 |  // ETX0
		1 << 24 |  // ETX1
		1 << 25 |  // ERX0
		1 << 26 |  // ERX1
		1 << 27 |  // ERXER
		1 << 28 |  // ETXEN
		1 << 29 |  // EMDC
		1 << 30;   // EMDIO
	at91_gpio_conf_periA(PIOE, mask, 0);

	// FIXME
	at91_rstc_writel(RSTC_MR, (0xA5 << 24) | (0x0d << 8) | 0x1);
	at91_rstc_writel(RSTC_CR, (0xA5 << 24) | (0x1 << 3));

	while (!(at91_rstc_readl(RSTC_SR) & (0x1 << 16)));

#ifdef CONFIG_IRQ_SUPPORT
	at91sam926x_interrupt_init();

#ifdef CONFIG_TIMER_SUPPORT
	at91sam926x_timer_init();
#endif
#endif

	return 0;
}

plat_init(at91sam9263_init);
