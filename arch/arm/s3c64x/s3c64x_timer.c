#include <irq.h>

static int s3c6410_timer_isr(__u32 irq, void *dev)
{
	__u32 val;

	if (!(readl(VA(TIN_CSTAT)) & 1 << 6))
		return IRQ_NONE;

	inc_tick();

	val = readl(VA(TIN_CSTAT));
	val |= 1 << 6;
	writel(VA(TIN_CSTAT), val);

	val = readl(VA(TCON));
	val |= 1 << 9;
	writel(VA(TCON), val);
	val &= ~(2 << 8);
	writel(VA(TCON), val);

	return IRQ_HANDLED;
}

int __init s3c6410_timer_init(void)
{
	__u32 val;
	int ret;

	val = readl(VA(TCFG0));
	val &= ~0xff;
	val |= 254;
	writel(VA(TCFG0), val);

	val = readl(VA(TCFG1));
	val &= ~(0xf << 4);
	val |= (0x1 << 4);
	writel(VA(TCFG1), val);

	//auto reload
	val = readl(VA(TCON));
	val |= 1 << 11;
	writel(VA(TCON), val);

	//1000 interrupts per second
	writel(VA(TCNTB1), 129);
	writel(VA(TCMPB1), 0);

	//manual load
	val = readl(VA(TCON));
	val |= 1 << 9;
	writel(VA(TCON), val);

	//clean manual load
	val = readl(VA(TCON));
	val &= ~(2<< 8);
	writel(VA(TCON), val);

	ret = irq_request(INT_TIMER1, s3c6410_timer_isr, NULL);
	if (ret < 0) {
		printf("%s %d irq_request() failed!\n", __FILE__, __LINE__);
		return ret;
	}

	//enable time0
	val = readl(VA(TIN_CSTAT));
	val |= (0x1 << 1);
	writel(VA(TIN_CSTAT), val);

	//start time0
	val = readl(VA(TCON));
	val |= (0x1 << 8);
	writel(VA(TCON), val);

	calibrate_delay(1000);

	return 0;
}
