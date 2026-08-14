#include <font/font.h>
#include <graphic/display.h>

static int is_dot(int x, int y, char c, struct font_desc *font)
{
	int w, h;
	__u8 *data;

	w = (font->width + 7) / 8;
	h = c * font->height + y;

	data = (__u8 *)font->data + w * h;
	data += x / 8;
	x = x % 8;

	return (*data >> (7 - x)) & 0x01;
}

int draw_char(struct display *disp, int x, int y, struct font_desc *font, __u32 color, char c)
{
	int width  = disp->video_mode->width;
	__u16 *buff;
	int i, j;

	buff = (__u16 *)disp->video_mem_va;

	for (i = 0; i < font->height; i++) {
		for (j = 0; j < font->width; j++) {
			// FIXME
			if (is_dot(j, i, c, font))
				buff[(i + y) * width + x + j] = color;
		}
	}

	return 0;
}

int draw_string(struct display *disp, int x, int y, struct font_desc *font, __u32 color, const char * str)
{
	while (*str) {
		draw_char(disp, x, y, font, color, *str);
		str++;
		x += font->width;
	}

	return 0;
}

int set_screen(__u32 pixel)
{
	__u16 *buff;
	int height, width;
	int i, j;
	struct display *disp;

	disp = get_system_display();
	buff = (__u16 *)disp->video_mem_va;
	height = disp->video_mode->height;
	width  = disp->video_mode->width;

	for (i = 0; i < height; i++) {
		for (j = 0; j < width; j++) {
			// FIXME
			buff[i * width + j] = pixel;
		}
	}

	return 0;
}
