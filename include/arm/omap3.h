#pragma once

#include <arm/omap3_register.h>

#define KHz(n) (1000 * (n))
#define MHz(n) (1000 * KHz(n))

#define OMAP3_SRAM_BASE 0x40200000
#define OMAP3_SRAM_SIZE 0x00004000

#define SDRAM_BASE  0x80000000
#define SDRAM_SIZE  0x04000000

#define ATAG_BASE   SDRAM_BASE

#define MPU_CLK    MHz(720)
#define L3_CLK     MHz(133)
#define L4_CLK     MHz(66.5)
#define HCLK_RATE  L3_CLK

#define INT_VECTOR_RAM_BASE 0x4020FFC8

#ifndef __ASSEMBLY__
#include <types.h>

int omap3_irq_init(void);

// FIXME: move to omapfb.h
struct omapfb_panel {
	int width, height;
	int bpp;

	// FIXME
	unsigned int hfp, hbp, hpw;
	unsigned int vfp, vbp, vpw;
};
#endif
