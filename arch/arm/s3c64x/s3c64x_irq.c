#include <io.h>
#include <init.h>
#include <irq.h>
#include <assert.h>

static void s3c6410_irq_ack(__u32 irq);
static void s3c6410_irq_mask(__u32 irq);
static void s3c6410_irq_umask(__u32 irq);

static inline __u32 get_bit_index(__u32 val)
{
	__u32 shift = 0;

	while (shift < 32) {
		if ((0x1UL << shift) & val)
			break;
		shift++;
	}

	return shift;
}

__u32 read_irq_num(void)
{
	__u32 irq_num, val;

	irq_num = readl(VA(VIC0_ADDRESS));
	if (!irq_num) {
		irq_num = readl(VA(VIC1_ADDRESS));
		if (!irq_num)
			BUG();
	}

	irq_num--;

	switch (irq_num) {
		case 0 ... 1:
			val = readl(VA(EINT0PEND));
			irq_num = get_bit_index(val) + MAX_INTERNAL_IRQ + 1;
			break;

		case 32 ... 33:
			//external interrupt 12~27
			break;

		default:
			break;
	}

	//	printf("irq_num is %d\n", irq_num);

	return irq_num;
}

static void s3c6410_irq_mask(__u32 irq)
{
	__u32 val;

	switch (irq) {
		case 0 ... 31:
			writel(VA(VIC0_INTENCLEAR), 0x1UL << irq);
			break;

		case 32 ... 63:
			writel(VA(VIC1_INTENCLEAR), 0x1UL << (irq - 32));
			break;

		case INT_EINT(0) ... INT_EINT(11):
			val = readl(VA(EINT0MASK));
			val |= 0x1UL << (irq - INT_EINT(0));
			writel(VA(EINT0MASK), val);
			break;

		default:
			return;
	}
}

static void s3c6410_irq_umask(__u32 irq)
{
	__u32 val;

	switch (irq) {
		case 0 ... 31:
			writel(VA(VIC0_INTENABLE), 0x1UL << irq);
			break;

		case 32 ... 63:
			writel(VA(VIC1_INTENABLE), 0x1UL << (irq - 32));
			break;

		case INT_EINT(0) ... INT_EINT(3):
			val = readl(VA(EINT0MASK));
			val &= ~(0x1UL << (irq - 64));
			writel(VA(EINT0MASK), val);
			writel(VA(VIC0_INTENABLE), 0x1UL);
			break;

		case INT_EINT(4) ... INT_EINT(11):
			val = readl(VA(EINT0MASK));
			val &= ~(0x1UL << (irq - 64));
			writel(VA(EINT0MASK), val);
			writel(VA(VIC0_INTENABLE), 0x1UL << 1);
			break;

		default:
			return;
	}
}

static void s3c6410_irq_ack(__u32 irq)
{
	//FIXME, should deal with the INT_EINT12 ~ 19 and GROUP1 ~ GROUP9
	if ((irq >= INT_EINT(0)) && (irq <= INT_EINT(11))) {
		irq -= INT_EINT(0);
		writel(VA(EINT0PEND), 0x1UL << irq);
	}

	if (irq < 32)
		writel(VA(VIC0_ADDRESS), 0x0);
	else
		writel(VA(VIC1_ADDRESS), 0x0);
}

static void s3c6410_irq_mack(__u32 irq)
{
}

struct int_ctrl s3c6410_intctl_level = {
	.ack   = s3c6410_irq_ack,
	.mask  = s3c6410_irq_mask,
	.mack  = s3c6410_irq_mack,
	.umask = s3c6410_irq_umask,
};

struct int_ctrl s3c6410_intctl_edge = {
	.ack   = s3c6410_irq_ack,
	.mask  = s3c6410_irq_mask,
	.mack  = s3c6410_irq_mack,
	.umask = s3c6410_irq_umask,
};

int __init s3c6410_interrupt_init(void)
{
	int irq_num;

	for (irq_num = 0; irq_num <= MAX_IRQ_NUM; irq_num++) {
		switch (irq_num) {
			case 0 ... 31:
				writel(VA(VIC0_VECTADDR0 + 0x4 * irq_num), irq_num + 1);
				irq_assoc_intctl(irq_num, &s3c6410_intctl_level);
				irq_set_handler(irq_num, vectorirq_handle_level, 0);
				break;

			case 32 ... 63:
				writel(VA(VIC1_VECTADDR0 + 0x4 * (irq_num - 32)), irq_num + 1);
				irq_assoc_intctl(irq_num, &s3c6410_intctl_level);
				irq_set_handler(irq_num, vectorirq_handle_level, 0);
				break;

			case INT_EINT(0) ... INT_EINT(11):
				irq_assoc_intctl(irq_num, &s3c6410_intctl_level);
				irq_set_handler(irq_num, vectorirq_handle_level, 0);
				break;

			default:
				break;
		}
	}

	irq_enable();

	return 0;
}
