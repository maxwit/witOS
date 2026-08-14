#include <io.h>
#include <irq.h>
#include <init.h>
#include <arm/at91sam926x.h>

#define  PIO_IRQ_OFFSET  2

static void at91sam926x_aic_ack(__u32 irq)
{
	at91_aic_writel(AIC_ICCR, 1 << irq);
}

static void at91sam926x_aic_mask(__u32 irq)
{
	at91_aic_writel(AIC_IDCR, 1 << irq);
}

static void at91sam926x_aic_mack(__u32 irq)
{
	at91sam926x_aic_mask(irq);
	at91sam926x_aic_ack(irq);
}

static void at91sam926x_aic_umask(__u32 irq)
{
	at91_aic_writel(AIC_IECR, 1 << irq);
}

static struct int_ctrl at91sam926x_aic_intctrl =
{
	.ack   = at91sam926x_aic_ack,
	.mask  = at91sam926x_aic_mask,
	.mack  = at91sam926x_aic_mack,
	.umask = at91sam926x_aic_umask,
};

static void at91sam926x_gpio_mask(__u32 irq)
{
	__u32 nPioIdx, nPioPin;

	irq -= 32;
	nPioIdx = irq >> 5;
	nPioPin = irq & 0x1f;

	writel(VA(PIO_BASE(nPioIdx) + PIO_IDR), 1 << nPioPin);
}

static void at91sam926x_gpio_umask(__u32 irq)
{
	__u32 nPioIdx, nPioPin;

	irq -= 32;
	nPioIdx = irq >> 5;
	nPioPin = irq & 0x1f;

	writel(VA(PIO_BASE(nPioIdx) + PIO_IER), 1 << nPioPin);
}

static struct int_ctrl at91sam926x_gpio_intctrl = {
	.mask  = at91sam926x_gpio_mask,
	.umask = at91sam926x_gpio_umask,
};

__u32 read_irq_num(void)
{
	return at91_aic_readl(AIC_IVR);
}

static void __init at91_aic_init(void)
{
	int irq;

	for(irq = 0; irq < 32; irq++) {
		at91_aic_writel(AIC_SVR(irq), irq);
		at91_aic_writel(AIC_SMR(irq), (0 << 5) | 0);

		irq_assoc_intctl(irq, &at91sam926x_aic_intctrl);
		irq_set_handler(irq, irq_handle_level, 0);
	}

	//	at91_aic_writel(AIC_SPU, MAX_IRQ_NUM);
	//	at91_aic_writel(AIC_DCR, 0);

	//	at91_aic_writel(AIC_IDCR, ~0UL);
	//	at91_aic_writel(AIC_ICCR, ~0UL);
}

static void at91sam926x_gpio_irqparse(struct int_pin *ipin, __u32 irq)
{
	__u32 dwPioStat;
	__u32 nPioIdx, nPioPin;

	nPioIdx = irq - PIO_IRQ_OFFSET;
	dwPioStat = readl(VA(PIO_BASE(nPioIdx) + PIO_ISR));
	dwPioStat &= readl(VA(PIO_BASE(nPioIdx) + PIO_IMR));
	//	printf("%s(): stat = 0x%08x\n", __func__, dwPioStat);

	for (nPioPin = 0; nPioPin < 32; nPioPin++) {
		if (dwPioStat & (1 << nPioPin))
			irq_handle(32 + 32 * nPioIdx + nPioPin);
	}
}

int __init at91sam926x_interrupt_init(void)
{
	__u32 i, j;
	__u32 irq;

	at91_aic_init();

	for (i = 0; i < 3; i++) {
		for (j = 0; j < 32; j++) {
			irq = 32 + ((i << 5) | j);

			irq_assoc_intctl(irq, &at91sam926x_gpio_intctrl);
			irq_set_handler(irq, irq_handle_simple, 0);
		}

		irq_set_handler(PIO_IRQ_OFFSET + i, at91sam926x_gpio_irqparse, 1);
	}

	irq_enable(); // FIXME

	return 0;
}
