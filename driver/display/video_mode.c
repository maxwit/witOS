#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <graphic/display.h>

static const struct lcd_vmode g_lcd_vm_tab[] =
{
	{
		.model  = "P043F1A4",
		.pix_clk = 9200000,
		.width  = 480,
		.height = 272,
		.hbp = 4,
		.hfp = 8,
		.hpw = 41,
		.vbp = 2,
		.vfp = 4,
		.vpw = 10,
	},
	{
		.model  = "3.5\" LCD",
		.pix_clk = 5600000,
		.width  = 240,
		.height = 320,
		.hbp = 13,
		.hfp = 13,
		.hpw = 31,
		.vbp = 4,
		.vfp = 4,
		.vpw = 3,
	},
	{
		.model  = "3.5\" LCD",
		.width  = 240,
		.height = 320,
		.pix_clk = 5600000, // FIXME!!!!
		.vbp = 4,
		.vfp = 4,
		.vpw = 2,
		.hbp = 38,
		.hfp = 21,
		.hpw = 6,
	},
	{
		.model  = "VGA",
		.width  = 800,
		.height = 600,
		.pix_clk = 9000000, // FIXME!!!!
		.vbp = 4,
		.vfp = 4,
		.vpw = 2,
		.hbp = 38,
		.hfp = 21,
		.hpw = 6,
	},
};

const struct lcd_vmode *lcd_get_vmode_by_id(int lcd_id)
{
	if (lcd_id >= ARRAY_ELEM_NUM(g_lcd_vm_tab))
		return NULL;

	return g_lcd_vm_tab + lcd_id;
}

// TODO: configure script
const struct lcd_vmode *lcd_get_vmode_by_name(const char *model)
{
	int lcd_id = 0;

	while (lcd_id < ARRAY_ELEM_NUM(g_lcd_vm_tab)) {
		if (!strcmp(g_lcd_vm_tab[lcd_id].model, model))
			return g_lcd_vm_tab + lcd_id;

		lcd_id++;
	}

	return NULL;
}
