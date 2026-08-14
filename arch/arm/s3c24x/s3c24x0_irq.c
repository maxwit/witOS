#include <io.h>
#include <irq.h>
#include <init.h>
#include <stdio.h>
#include <bitops.h>
#include <arm/s3c24x0.h>

#define EXTINT_OFF       (IRQ_EINT4 - 4)
#define INTMSK_TCnADC    (1UL << (IRQ_TC_ADC - IRQ_EINT0))

__u32 read_irq_num(void)
{
	return readl(VA(S3C2410_INTOFFSET));
}

static void s3c24x0_irq_mask(__u32 irq)
{
	__u32 mask;

	mask = readl(VA(S3C2410_INTMSK));
	mask |= 1UL << (irq - IRQ_EINT0);
	writel(VA(S3C2410_INTMSK), mask);
}

static void s3c24x0_irq_ack(__u32 irq)
{
	__u32 dwVal = 1UL << (irq - IRQ_EINT0);

	writel(VA(S3C2410_SRCPND), dwVal);
	writel(VA(S3C2410_INTPND), dwVal);
}

static void s3c24x0_irq_mack(__u32 irq)
{
	__u32 dwVal = 1UL << (irq - IRQ_EINT0);
	__u32 mask;

	mask = readl(VA(S3C2410_INTMSK));
	writel(VA(S3C2410_INTMSK), mask | dwVal);

	writel(VA(S3C2410_SRCPND), dwVal);
	writel(VA(S3C2410_INTPND), dwVal);
}

static void s3c24x0_irq_umask(__u32 irq)
{
	__u32 mask;

	irq -= IRQ_EINT0;

	mask = readl(VA(S3C2410_INTMSK));
	mask &= ~(1UL << irq);
	writel(VA(S3C2410_INTMSK), mask);
}

struct int_ctrl s3c24x0_intctl_level = {
	.ack   = s3c24x0_irq_mack,
	.mask  = s3c24x0_irq_mask,
	.mack  = s3c24x0_irq_mack,
	.umask = s3c24x0_irq_umask,
};

struct int_ctrl s3c24x0_intctl = {
	.ack   = s3c24x0_irq_ack,
	.mask  = s3c24x0_irq_mask,
	.umask = s3c24x0_irq_umask,
};

static void s3c24x0_ext_mask(__u32 irq)
{
	__u32 mask;

	irq -= EXTINT_OFF;

	mask = readl(VA(S3C2410_EINTMASK));
	mask |= 1UL << irq;
	writel(VA(S3C2410_EINTMASK), mask);
}

static void s3c24x0_ext_ack(__u32 irq)
{
	__u32 req;

	req = readl(VA(S3C2410_EINTPEND));
	writel(VA(S3C2410_EINTPEND), req);

	// __u32 bit;
	// bit = 1UL << (irq - EXTINT_OFF);
	// writel(VA(S3C2410_EINTPEND), bit);

	// FIXME
	if (irq <= IRQ_EINT7) {
		if ((req & 0xf0) != 0)
			s3c24x0_irq_ack(IRQ_EINT_4TO7);
	} else {
		if ((req >> 8) != 0)
			s3c24x0_irq_ack(IRQ_EINT_8TO23);
	}
}

static void s3c24x0_ext_umask(__u32 irq)
{
	__u32 mask;

	irq -= EXTINT_OFF;

	mask = readl(VA(S3C2410_EINTMASK));
	mask &= ~( 1UL << irq);
	writel(VA(S3C2410_EINTMASK), mask);
}

int s3c24x0_ext_set_trigger(__u32 irq, unsigned int type)
{
	__u32 extint_reg, gpcon_reg;
	__u32 gpcon_offset, extint_offset;
	__u32 newvalue = 0, value;

	if ((irq >= IRQ_EINT0) && (irq <= IRQ_EINT3)) {
		gpcon_reg = GPIO_BASE + GPF_CON;
		extint_reg = GPIO_BASE + EXTINT0;
		gpcon_offset = (irq - IRQ_EINT0) * 2;
		extint_offset = (irq - IRQ_EINT0) * 4;
	} else if ((irq >= IRQ_EINT4) && (irq <= IRQ_EINT7)) {
		gpcon_reg = GPIO_BASE + GPF_CON;
		extint_reg = GPIO_BASE + EXTINT0;
		gpcon_offset = (irq - (EXTINT_OFF)) * 2;
		extint_offset = (irq - (EXTINT_OFF)) * 4;
	} else if ((irq >= IRQ_EINT8) && (irq <= IRQ_EINT15)) {
		gpcon_reg = GPIO_BASE + GPG_CON;
		extint_reg = GPIO_BASE + EXTINT1;
		gpcon_offset = (irq - IRQ_EINT8) * 2;
		extint_offset = (irq - IRQ_EINT8) * 4;
	} else if ((irq >= IRQ_EINT16) && (irq <= IRQ_EINT23)) {
		gpcon_reg = GPIO_BASE + GPG_CON;
		extint_reg = GPIO_BASE + EXTINT2;
		gpcon_offset = (irq - IRQ_EINT8) * 2;
		extint_offset = (irq - IRQ_EINT16) * 4;
	} else
		return -1;

	value = readl(VA(gpcon_reg));
	value = (value & ~(3 << gpcon_offset)) | (0x02 << gpcon_offset);
	writel(VA(gpcon_reg), value);

	switch (type) {
		case IRQ_TYPE_NONE:
			printf("No edge setting!\n");
			break;

		case IRQ_TYPE_RISING:
			newvalue = S3C24X0_EXTINT_RISE;
			break;

		case IRQ_TYPE_FALLING:
			newvalue = S3C24X0_EXTINT_FALL;
			break;

		case IRQ_TYPE_BOTH:
			newvalue = S3C24X0_EXTINT_BOTH;
			break;

		case IRQ_TYPE_LOW:
			newvalue = S3C24X0_EXTINT_LOW;
			break;

		case IRQ_TYPE_HIGH:
			newvalue = S3C24X0_EXTINT_HIGH;
			break;

		default:
			printf("No such irq type %d", type);
			return -1;
	}

	value = readl(VA(extint_reg));
	value = (value & ~(7 << extint_offset)) | (newvalue << extint_offset);
	writel(VA(extint_reg), value);

	return 0;
}

static struct int_ctrl s3c24x0_intctl_ext4to23 =
{
	.ack   = s3c24x0_ext_ack,
	.mask  = s3c24x0_ext_mask,
	.umask = s3c24x0_ext_umask,
	.set_trigger = s3c24x0_ext_set_trigger,
};

static struct int_ctrl s3c24x0_intctl_ext0to3 =
{
	.ack   = s3c24x0_irq_ack,
	.mask  = s3c24x0_irq_mask,
	.umask = s3c24x0_irq_umask,
	.set_trigger = s3c24x0_ext_set_trigger,
};

static inline void s3c24x0_subic_mask(__u32 irq, unsigned int parentbit, int subcheck)
{
	__u32 mask, submask;

	submask = readl(VA(S3C2410_INTSUBMSK));
	mask = readl(VA(S3C2410_INTMSK));

	submask |= (1UL << (irq - IRQ_UART0_RX));

	if ((submask  & subcheck) == subcheck)
		writel(VA(S3C2410_INTMSK), mask | parentbit);

	writel(VA(S3C2410_INTSUBMSK), submask);
}

static inline void s3c24x0_subic_umask(__u32 irq, unsigned int parentbit)
{
	__u32 mask, submask;

	submask = readl(VA(S3C2410_INTSUBMSK));
	mask = readl(VA(S3C2410_INTMSK));

	submask &= ~(1UL << (irq - IRQ_UART0_RX));
	mask &= ~parentbit;

	writel(VA(S3C2410_INTSUBMSK), submask);
	writel(VA(S3C2410_INTMSK), mask);
}

static inline void s3c24x0_subic_mack(__u32 irq, unsigned int parentmask, unsigned int group)
{
	__u32 bit = 1UL << (irq - IRQ_UART0_RX);

	s3c24x0_subic_mask(irq, parentmask, group);

	writel(VA(S3C2410_SUBSRCPND), bit);

	writel(VA(S3C2410_SRCPND), parentmask);
	writel(VA(S3C2410_INTPND), parentmask);
}

static inline void s3c24x0_subic_ack(__u32 irq, unsigned int parentmask, unsigned int group)
{
	unsigned int bit = 1UL << (irq - IRQ_UART0_RX);

	writel(VA(S3C2410_SUBSRCPND), bit);

	writel(VA(S3C2410_SRCPND), parentmask);
	writel(VA(S3C2410_INTPND), parentmask);
}

static void s3c24x0_adc_mask(__u32 irq)
{
	s3c24x0_subic_mask(irq, INTMSK_TCnADC, 3 << 9);
}

static void s3c24x0_adc_umask(__u32 irq)
{
	s3c24x0_subic_umask(irq, INTMSK_TCnADC);
}

static void s3c24x0_adc_ack(__u32 irq)
{
	s3c24x0_subic_ack(irq, INTMSK_TCnADC, 3 << 9);
}

static struct int_ctrl s3c_irq_adc =
{
	.ack   = s3c24x0_adc_ack,
	.mask  = s3c24x0_adc_mask,
	.umask = s3c24x0_adc_umask,
};

static void s3c24x0_parse_adc_irq(struct int_pin *ipin, __u32 irq)
{
	unsigned int nSubPnd;

	nSubPnd = readl(VA(S3C2410_SUBSRCPND));

	nSubPnd >>= 9;
	nSubPnd &= 3;

	if (nSubPnd & 1)
		irq_handle(IRQ_TC);

	if (nSubPnd & 2)
		irq_handle(IRQ_ADC);
}

static void s3c24x0_parse_ext_irq(struct int_pin *ipin, __u32 irq)
{
	__u32 ext_pnd = readl(VA(S3C2410_EINTPEND));

	switch (irq) {
	case IRQ_EINT_4TO7:
		ext_pnd &= 0xf0;
		break;

	case IRQ_EINT_8TO23:
		ext_pnd &= ~0xff;
		break;

	default:
		break;
	}

	while (ext_pnd) {
		unsigned int ext_irq;

		ext_irq = ffs(ext_pnd) - 1;

		ext_pnd &= ~(1 << ext_irq);
		ext_irq += EXTINT_OFF;

		irq_handle(ext_irq);
	}
}

int __init s3c24x0_interrupt_init(void)
{
	int irq;

	for (irq = IRQ_EINT0; irq < MAX_IRQ_NUM; irq++) {
		switch (irq) {
		case IRQ_EINT0 ... IRQ_EINT3:
			irq_assoc_intctl(irq, &s3c24x0_intctl_ext0to3);
			irq_set_handler(irq, irq_handle_edge, 0);
			break;

		case IRQ_EINT4 ... IRQ_EINT23:
			irq_assoc_intctl(irq, &s3c24x0_intctl_ext4to23);
			irq_set_handler(irq, irq_handle_edge, 0);
			break;

		case IRQ_EINT_4TO7:
		case IRQ_EINT_8TO23:
			irq_assoc_intctl(irq, &s3c24x0_intctl_level);
			irq_set_handler(irq, s3c24x0_parse_ext_irq, 1);
			break;

		case IRQ_UART0: // FIXME
		case IRQ_UART1:
		case IRQ_UART2:
		case IRQ_TC_ADC:
			irq_assoc_intctl(irq, &s3c24x0_intctl_level);
			break;

		case IRQ_RESERVED6:
		case IRQ_RESERVED24:
			break;

		case IRQ_TC:
		case IRQ_ADC:
			irq_assoc_intctl(irq, &s3c_irq_adc);
			irq_set_handler(irq, irq_handle_edge, 0);
			break;

		default:
			irq_assoc_intctl(irq, &s3c24x0_intctl);
			irq_set_handler(irq, irq_handle_edge, 0);
		}
	}

	irq_set_handler(IRQ_TC_ADC, s3c24x0_parse_adc_irq, 1);

	irq_enable(); // FIXME

	return 0;
}

