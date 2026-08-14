#include <io.h>
#include <init.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <delay.h>
#include <platform.h>
#include <graphic/display.h>

#define dss_readl(reg) \
	readl(VA(DSS_BASE + reg))

#define dss_writel(reg, val) \
	writel(VA(DSS_BASE + reg), val)

#define CLKSEL_DSS1 8
static int omap3_set_vmode(struct display *disp, const struct lcd_vmode *vm)
{
	__u32 val;
	// __u32 dss1_alwon_fclk;

	dss_writel(DISPC_TIMING_H, vm->hbp << 20 | vm->hfp << 8 | vm->hpw);
	dss_writel(DISPC_TIMING_V, vm->vbp << 20 | vm->vfp << 8 | vm->vpw);

	//
	val = readl(VA(CM_CLKSEL_DSS));
	val &= ~0x1F;
	val |= CLKSEL_DSS1;
	writel(VA(CM_CLKSEL_DSS), val);

	// dss1_alwon_fclk =

	// GEN_DBG("divsor = %d\n", dss1_alwon_fclk / vm->pix_clk);
	dss_writel(DISPC_DIVISOR, 1 << 16 | 2 /* dss1_alwon_fclk / vm->pix_clk */);

	dss_writel(DISPC_SIZE_LCD, (vm->height - 1) << 16 | (vm->width - 1));

	dss_writel(DISPC_GFX_BA0, disp->video_mem_pa);
	dss_writel(DISPC_GFX_POSITION, 0);
	dss_writel(DISPC_GFX_SIZE, (vm->height - 1) << 16 | (vm->width - 1));

	// FIXME
	switch (disp->pix_fmt) {
	case PIX_RGB16:
		val = 0x6;
		break;

	case PIX_RGB24:
		val = 0x9;
		break;

	case PIX_RGB32:
		val = 0xd;
		break;

	default:
		printf("%s(): invalid pixel format (%d)!\n", __func__, disp->pix_fmt);
		return -EINVAL;
	}

	dss_writel(DISPC_GFX_ATTRIBUTES, val << 1 | 1);

	dss_writel(DISPC_CONTROL, 1 << 16 | 1 << 15 | 1 << 8 | 1 << 3 | 1);

	return 0;
}

#define OMAP_TIMEOUT 0x100

static int __init omap3_disp_probe(struct platform_device *pdev)
{
	int ret, to;
	__u32 val;
	struct display *disp;

	disp = display_create();
	// if NULL

	// enable clock
	writel(VA(CM_FCLKEN_DSS), 7);
	writel(VA(CM_ICLKEN_DSS), 1);
	writel(VA(CM_CLKEN_PLL), 0x7 << 16);

	// reset
	dss_writel(DISPC_SYSCONFIG, 0x02);
	for (to = 0; to < OMAP_TIMEOUT; to++) {
		val = dss_readl(DISPC_SYSCONFIG);
		if (!(val & 0x2)) // FIXME
			break;

		udelay(0x100);
	}

	if (OMAP_TIMEOUT == to)
		return -ETIMEDOUT;
		// printf("%s() line %d: reset timeout!\n", __func__, __LINE__);

	// configure clock
	dss_writel(DSS_CONTROL, 1);

	dss_writel(DISPC_SYSCONFIG, 1);
	dss_writel(DISPC_CONFIG, 1 << 9 | 1 << 1);

	disp->set_vmode = omap3_set_vmode;

	ret = display_register(disp);
	if (ret < 0)
		goto error;

	return 0;

error:
	display_destroy(disp);
	return ret;
}

static struct platform_driver omap3_disp_driver = {
	.drv = {
		.name = "omap disp",
	},
	.probe = omap3_disp_probe,
};

static int __init omap3_display_init(void)
{
	return platform_driver_register(&omap3_disp_driver);
}

module_init(omap3_display_init);
