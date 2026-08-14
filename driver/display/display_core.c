#include <stdio.h>
#include <delay.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <malloc.h>
#include <graphic/display.h>
#include <djpeg/djpeg.h>

#define NR_DISP 4

static struct display* g_system_display[NR_DISP];

#ifdef CONFIG_BOOTUP_LOGO
static __u16 RGB24toRGB16(__u8 r, __u8 g, __u8 b)
{
	return (r >> 3) << 11 | (g >> 2) << 5 | b >> 3;
}
#endif

static void draw_logo(void * const video_buff, __u32 width, __u32 height, pix_fmt_t pix_format)
{
	int i;
	void *vbuff = video_buff; // FIXME

#ifdef CONFIG_BOOTUP_LOGO
	int j, x, y;
	__u8  *buff;
	__u32 step_h;
	__u32 step_w;
	__u32 rgb_size;
	__u32 line_width;
	extern unsigned char _witos_jpg[];
	// extern unsigned int _witos_jpg_len;
	struct djpeg_opts djpeg2bmp;

	djpeg2bmp.jpegbuf = _witos_jpg;

	jpeg2bmp_decode(&djpeg2bmp);

	step_h = (djpeg2bmp.imgbi->biHeight << 10) / height;
	step_w = (djpeg2bmp.imgbi->biWidth << 10) / width;

	rgb_size = djpeg2bmp.imgbf->bfSize - djpeg2bmp.imgbf->bfOffBits;

	buff = djpeg2bmp.rgbdata;

	line_width = rgb_size / djpeg2bmp.imgbi->biHeight;

	for (i = 0; i < height; i++) {
		for (j = 0; j < width * 3; j += 3) {
			x = (djpeg2bmp.imgbi->biHeight - ((i * step_h) >> 10) - 1) * line_width;
			y = (((j * step_w) >> 10) / 3) * 3;

			// FIXME
			switch (pix_format) {
			case PIX_RGB24:
			case PIX_RGB32:
				*((__u8 *)vbuff + 0) = buff[x + y + 0];
				*((__u8 *)vbuff + 1) = buff[x + y + 1];
				*((__u8 *)vbuff + 2) = buff[x + y + 2];

				vbuff += 4;
				break;

			case PIX_RGB15:
			case PIX_RGB16:
				*(__u16 *)vbuff = (__u16)RGB24toRGB16(
										buff[x + y + 2],
										buff[x + y + 1],
										buff[x + y + 0]
										);
				vbuff += 2;
				break;

			default:
				break;
			}
		}
	}

	free(djpeg2bmp.bmpbuf);
#else // FIXME
	__u32 pix;

	struct rgb_format
	{
		int r_len, r_off;
		int g_len, g_off;
		int b_len, b_off;
		int bytes;
	} rgb;

#define MAKE_PIX(c) \
	(((1 << rgb.c##_len) - 1) << rgb.c##_off)

	switch (pix_format) {
	case PIX_RGB15:
		rgb.r_len = 5;
		rgb.r_off = 0;
		rgb.g_len = 5;
		rgb.g_off = 5;
		rgb.b_len = 5;
		rgb.b_off = 10;
		rgb.bytes = 2;
		break;

	case PIX_RGB16:
		rgb.r_len = 5;
		rgb.r_off = 0;
		rgb.g_len = 6;
		rgb.g_off = 5;
		rgb.b_len = 5;
		rgb.b_off = 11;
		rgb.bytes = 2;
		break;

	case PIX_RGB24:
	case PIX_RGB32:
		rgb.r_len = 8;
		rgb.r_off = 0;
		rgb.g_len = 8;
		rgb.g_off = 8;
		rgb.b_len = 8;
		rgb.b_off = 16;
		rgb.bytes = 4;
		break;
	default:
		rgb.r_len = 0;
		rgb.r_off = 0;
		rgb.g_len = 0;
		rgb.g_off = 0;
		rgb.b_len = 0;
		rgb.b_off = 0;
		rgb.bytes = 0;
		break;
	}

	// FIXME
	pix = MAKE_PIX(r);
	for (i = 0; i < width * (height / 3); i++) {
		memcpy(vbuff, &pix, rgb.bytes);
		vbuff += rgb.bytes;
	}

	pix = MAKE_PIX(g);
	for (i = 0; i < width * (height / 3); i++) {
		memcpy(vbuff, &pix, rgb.bytes);
		vbuff += rgb.bytes;
	}

	pix = MAKE_PIX(b);
	for (i = 0; i < width * (height / 3); i++) {
		memcpy(vbuff, &pix, rgb.bytes);
		vbuff += rgb.bytes;
	}
#endif
}

struct display *display_create(void)
{
	struct display *disp;
	char conf_pixel[CONF_VAL_LEN];

	disp = zalloc(sizeof(*disp));
	if (!disp)
		return NULL;

	// FIXME!
	if (conf_get_attr("display.lcd.pixel", conf_pixel) == 0) {
		if (strncmp(conf_pixel, "PIX_RGB24", sizeof(conf_pixel)))
			disp->pix_fmt = PIX_RGB24;
		else if (strncmp(conf_pixel, "PIX_RGB16", sizeof(conf_pixel)))
			disp->pix_fmt = PIX_RGB16;
		else
			disp->pix_fmt = PIX_RGB15;
	} else {
		disp->pix_fmt = PIX_RGB24;
	}

	return disp;
}

void display_destroy(struct display *disp)
{
	free(disp);
}

static int display_config(struct display *disp)
{
	int ret;
	char attr_val[CONF_VAL_LEN];

	ret = conf_get_attr("display.lcd.pixel", attr_val);
	if (ret < 0)
		disp->pix_fmt = PIX_RGB16;
	else if (!strcasecmp(attr_val, "RGB15"))
		disp->pix_fmt = PIX_RGB15;
	else if (!strcasecmp(attr_val, "RGB16"))
		disp->pix_fmt = PIX_RGB16;
	else {
		DPRINT("%s(): fail to get lcd pixel format\n", __func__);
		return -EINVAL;
	}

	return 0;
}

int display_register(struct display* disp)
{
	int ret;
	void *va;
	unsigned long dma;
	const struct lcd_vmode *vm;
	char attr_val[CONF_VAL_LEN];
	size_t size;
	int minor;

	if (!disp->set_vmode)
		return -EINVAL;

	// FIXME
	ret = conf_get_attr("display.lcd.model", attr_val);
	if (ret < 0) {
		DPRINT("%s(): fail to get lcd model\n", __func__);
		return -EINVAL;
	}

	vm = lcd_get_vmode_by_name(attr_val);
	if (NULL == vm) {
		printf("No LCD video mode found!\n");
		return -ENOENT;
	}

	printf("using panel \"%s\"(%d X %d)\n", vm->model, vm->width, vm->height);

	ret = display_config(disp);
	if (ret < 0)
		return ret;

	size = vm->width * vm->height;

	switch (disp->pix_fmt) {
	case PIX_RGB15:
	case PIX_RGB16:
		size *= 2;
		break;

	case PIX_RGB24:
	case PIX_RGB32:
		size *= 4;
		break;

	default:
		BUG();
	}

	va = dma_alloc_coherent(size, &dma);
	if (NULL == va) {
		GEN_DBG("no free memory (size = %d)!\n", __func__, __LINE__, size);
		return -ENOMEM;
	}

	DPRINT("DMA: addr = 0x%08x, size = 0x%08x\n", dma, size);

	disp->video_mem_va = va;
	disp->video_mem_pa = dma;

	draw_logo(va, vm->width, vm->height, disp->pix_fmt);

	ret = disp->set_vmode(disp, vm);
	if (ret < 0)
		return ret;

	disp->video_mode = (void *)vm; // FIXME

	for (minor = 0; minor < NR_DISP; minor++) {
		if (!g_system_display[minor]) {
			g_system_display[minor] = disp;
			break;
		}
	}

	if (NR_DISP == minor)
		return -EBUSY;

	return 0;
}

struct display* get_system_display(void)
{
	return g_system_display[0];
}
