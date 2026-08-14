#pragma once

#include <arm/s3c6410_register.h>

#define SDRAM_BASE      0x50000000
#define SDRAM_SIZE      0x04000000

#define UART_NUM         5

#define S3C6410_SRAM_BASE 0x0c000000
#define S3C6410_SRAM_SIZE 0x2000

///
#define FIN       12000000

#define A_MDIV    333
#define A_PDIV    3
#define A_SDIV    1

#define M_MDIV    266
#define M_PDIV    3
#define M_SDIV    2

#define AM_PLL_RATE(mdiv, pdiv, sdiv) \
	(FIN / ((pdiv) * (1 << (sdiv))) * (mdiv))

#define APLL_RATE AM_PLL_RATE(A_MDIV, A_PDIV, A_SDIV)
#define MPLL_RATE AM_PLL_RATE(M_MDIV, M_PDIV, M_SDIV)

#define ACLK_RATE    APLL_RATE
#define HCLKx2_RATE  MPLL_RATE
#define HCLK_RATE   (MPLL_RATE / 2)
#define PCLK_RATE   (HCLK_RATE / 2)

// FIXME
#define ATAG_BASE          SDRAM_BASE

#define INITRD_BASE_ADDR   (SDRAM_BASE + SDRAM_SIZE / 4)
#define INITRD_MAX_SIZE    (SDRAM_SIZE / 4)

#define LINUX_BASE_ADDR    (INITRD_BASE_ADDR + SDRAM_SIZE / 4)
#define LINUX_MAX_SIZE     (SDRAM_SIZE / 4)

// SPI
#ifndef __ASSEMBLY__
#include <types.h>

void chip_select(void);

void chip_unselect(void);

void spi_send_receive(__u8 *send, int send_count, __u8 *receive, int receive_count);

__u8 is_rx_overrun(void);

__u8 is_tx_overrun(void);

int s3c6410_interrupt_init(void);

int s3c6410_timer_init(void);

#endif
