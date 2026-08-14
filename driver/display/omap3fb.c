#include <fb.h>
#include <io.h>
#include <init.h>
#include <string.h>
#include <malloc.h>
#include <platform.h>
#include <delay.h>

#define dss_readl(reg) \
	readl(omapfb->mmio + reg)

#define dss_writel(reg, val) \
	writel(omapfb->mmio + reg, val)

#define CLKSEL_DSS1 8

struct omapfb_info {
	struct omapfb_panel *panel;
	void *mmio;
};

static int omapfb_check_var(struct fb_info *fb,
	struct fb_var_screeninfo *var)
{
	struct omapfb_info *omapfb = fb->par;

	if (var->xres != omapfb->panel->width || var->yres != omapfb->panel->height)
		return -ENOTSUPP;

	fb->var.xres = var->xres;
	fb->var.yres = var->yres;

	return 0;
}

static int omapfb_set_par(struct fb_info *fb)
{
	__u32 val;
	struct fb_var_screeninfo *var = &fb->var;
	// struct fb_fix_screeninfo *fix = &fb->fix;
	struct omapfb_info *omapfb = fb->par;
	struct omapfb_panel *pan;

	pan = omapfb->panel;

	dss_writel(DISPC_TIMING_H, pan->hbp << 20 | pan->hfp << 8 | pan->hpw);
	dss_writel(DISPC_TIMING_V, pan->vbp << 20 | pan->vfp << 8 | pan->vpw);

	// FIXME
	val = readl(VA(CM_CLKSEL_DSS));
	val &= ~0x1F;
	val |= CLKSEL_DSS1;
	writel(VA(CM_CLKSEL_DSS), val);

	dss_writel(DISPC_DIVISOR, 1 << 16 | 2 /* dss1_alwon_fclk / var->pix_clk */);
	dss_writel(DISPC_SIZE_LCD, (var->yres - 1) << 16 | (var->xres - 1));
	dss_writel(DISPC_GFX_POSITION, 0);
	dss_writel(DISPC_GFX_SIZE, (var->yres - 1) << 16 | (var->xres - 1));

	// FIXME
	switch (pan->bpp) {
	case 16:
		val = 0x6;
		break;

	case 24:
		val = 0x9;
		break;

	case 32:
		val = 0xd;
		break;

	default:
		// printf("%s(): invalid pixel format (%d)!\n", __func__, disp->pix_fmt);
		return -EINVAL;
	}

	dss_writel(DISPC_GFX_ATTRIBUTES, val << 1 | 1);

	dss_writel(DISPC_CONTROL, 1 << 16 | 1 << 15 | 1 << 8 | 1 << 3 | 1);

	return 0;
}


static const struct fb_ops omapfb_ops = {
	.fb_check_var = omapfb_check_var,
	.fb_set_par = omapfb_set_par,
};

static int omapfb_reset(struct fb_info *fb)
{
#define OMAP_TIMEOUT 0x100
	int to;
	__u32 val;
	struct omapfb_info *omapfb = fb->par;

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

	// configure clock
	dss_writel(DSS_CONTROL, 1);

	dss_writel(DISPC_SYSCONFIG, 1);
	dss_writel(DISPC_CONFIG, 1 << 9 | 1 << 1);

	dss_writel(DISPC_GFX_BA0, fb->fix.smem_start);

	return 0;
}

static int __init omapfb_probe(struct platform_device *pdev)
{
	int ret;
	struct fb_info *fb;
	struct omapfb_info *omapfb;
	struct resource *mres;
	struct fb_fix_screeninfo *fix;
	struct fb_var_screeninfo var;

	mres = platform_get_mem(pdev, 0);
	if (!mres)
		return -ENODEV;

	if (!pdev->platform_data)
		return -ENODEV;

	fb = framebuffer_alloc(sizeof(*omapfb));
	// ...

	fix = &fb->fix;
	omapfb = fb->par;

	omapfb->mmio = VA(mres->start);
	omapfb->panel = pdev->platform_data;

	fix->smem_len = omapfb->panel->width * omapfb->panel->height * \
					((omapfb->panel->bpp + 7) / 8);
	fb->ops = &omapfb_ops;

	fb->screenbase = dma_alloc_coherent(fix->smem_len, &fix->smem_start);
	// if (!fb->screenbase)

	ret = omapfb_reset(fb);
	// ret ...

	var.xres = omapfb->panel->width;
	var.yres = omapfb->panel->height;

	ret = fb->ops->fb_check_var(fb, &var);
	// if ret

	ret = fb->ops->fb_set_par(fb);
	// if ret < 0 ..

	ret = framebuffer_register(fb);
	// if ret < 0 ...

	return ret;
}

static struct platform_driver omapfb_driver = {
	.drv = {
		.name = "omap disp",
	},
	.probe = omapfb_probe,
};

static int __init omapfb_init(void)
{
	return platform_driver_register(&omapfb_driver);
}

module_init(omapfb_init);
