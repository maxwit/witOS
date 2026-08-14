#include <io.h>
#include <init.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <graphic/display.h>

#define lcd_6410_readl(reg) \
	readl(VA(LCD_BASE + reg))

#define lcd_6410_writel(reg, val) \
	writel(VA(LCD_BASE + reg), val)

static int s3c6410_set_vmode(struct display *disp, const struct lcd_vmode *vm)
{
	__u32 fmt, bpp;
	__u32 dma = disp->video_mem_pa;

	// FIXME
	switch (disp->pix_fmt) {
	case PIX_RGB24:
	case PIX_RGB32:
		fmt = 0xB;
		bpp = 4;
		break;

	case PIX_RGB16:
		fmt = 0x5;
		bpp = 2;
		break;

	case PIX_RGB15:
		fmt = 0x7;
		bpp = 2;
		break;

	default:
		BUG();
	}

	// H/W init
	lcd_6410_writel(VIDCON0, (HCLK_RATE / vm->pix_clk - 1) << 6 | 1 << 4 | 0x3);
	lcd_6410_writel(VIDCON1, 0);
	lcd_6410_writel(VIDCON2, 0);

	lcd_6410_writel(VIDTCON0, (vm->vbp - 1)<< 16 | (vm->vfp - 1) << 8 | (vm->vpw - 1));
	lcd_6410_writel(VIDTCON1, (vm->hbp - 1) << 16 | (vm->hfp -1) << 8 | (vm->hpw - 1));
	lcd_6410_writel(VIDTCON2, (vm->height - 1) << 11 | (vm->width - 1));

	lcd_6410_writel(WINCON0, fmt << 2 | 1);
	lcd_6410_writel(VIDOSD0A, 0);
	lcd_6410_writel(VIDOSD0B, vm->width << 11 | vm->height);
	lcd_6410_writel(VIDOSD0C, vm->width * vm->height * bpp / 4);

	lcd_6410_writel(VIDW00ADD0B0, dma);
	lcd_6410_writel(VIDW00ADD1B0, (dma & 0xffffff) + vm->width * vm->height * bpp);
	// lcd_6410_writel(VIDW00ADD2, vm->width * 4);

	return 0;
}

static int __init s3c6410_display_init(void)
{
	int ret;
	__u32 val;
	struct display *disp;

	writel(VA(GPI_CON), 0xaaaaaaaa);
	writel(VA(GPJ_CON), 0xaaaaaaaa);

	// Enable Clock
	val  = readl(VA(MIFPCON));
	val &= ~(1 << 3);
	writel(VA(MIFPCON), val);

	val  = readl(VA(SPCON));
	val &= ~3;
	val |= 1;
	writel(VA(SPCON), val);

	disp = display_create();
	// if NULL

	disp->set_vmode = s3c6410_set_vmode;

	ret = display_register(disp);
	if (ret < 0)
		goto error;

	return 0;

error:
	display_destroy(disp);
	return ret;
}

module_init(s3c6410_display_init);
