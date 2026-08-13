#include <io.h>
#include <irq.h>
#include <timer.h>
#include <arm/s3c24x0.h>

// #define PWM_FREQUENCY 50.625MHz //depends on PCLK

static int s3c24x0_pwm_isr(__u32 irq, void *dev)
{
	inc_tick();
	// printf("%s() pwm interrupt!\n", __func__);

	writel(VA(PWM_BASE + TCNTO(3)), readl(VA(PWM_BASE + TCNTO(3))));

	return IRQ_HANDLED;
}

#define TEST_INS_NUM 100000

int __init s3c24x0_timer_init(void)
{
	int ret;

	writel(VA(PWM_BASE + TCFG1), 0x0);
	writel(VA(PWM_BASE + TCFG0), 0x0);
	writel(VA(PWM_BASE + TCON), 0x1 << 19);
	writel(VA(PWM_BASE + TCMPB(3)), 20);
	writel(VA(PWM_BASE + TCNTB(3)), 25333);
	writel(VA(PWM_BASE + TCON), (0x1 << 17) | readl(VA(PWM_BASE + TCON)));
	writel(VA(PWM_BASE + TCMPB(3)), 0);
	writel(VA(PWM_BASE + TCNTB(3)), 25313);
	writel(VA(PWM_BASE + TCON), (~(0x1 << 17)) & readl(VA(PWM_BASE + TCON)));

	ret = irq_request(13, s3c24x0_pwm_isr, NULL);
	if (ret < 0) {
		printf("%s(): irq_request() fail!\n", __func__);
		return ret;
	}

	writel(VA(PWM_BASE + TCON), (1 << 16) | readl(VA(PWM_BASE + TCON)));

	calibrate_delay(1000);

	return ret;
}
