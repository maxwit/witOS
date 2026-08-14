#pragma once

#include <arm/s3c24x0_register.h>

#define UART_NUM         3

// #define GTH_STACK_BASE	 0x1000
// #define S3C24XX_SRAM_BASE
#define S3C24XX_SRAM_SIZE  0x1000

#if 0
#define SVC_STACK_BASE   (SDRAM_BASE + SDRAM_SIZE)
#define SVC_STACK_SIZE   0x00080000

#define UND_STACK_BASE   (SVC_STACK_BASE - 1 * SVC_STACK_SIZE)
#define UND_STACK_SIZE   SVC_STACK_SIZE

#define ABT_STACK_BASE   (SVC_STACK_BASE - 2 * SVC_STACK_SIZE)
#define ABT_STACK_SIZE   SVC_STACK_SIZE

#define IRQ_STACK_BASE   (SVC_STACK_BASE - 3 * SVC_STACK_SIZE)
#define IRQ_STACK_SIZE   SVC_STACK_SIZE

#define FIQ_STACK_BASE   (SVC_STACK_BASE - 4 * SVC_STACK_SIZE)
#define FIQ_STACK_SIZE   SVC_STACK_SIZE

#define GBH_STACK_BASE   SVC_STACK_BASE
#define GBH_STACK_LIMIT  (FIQ_STACK_BASE - FIQ_STACK_SIZE)

#define NAND_PAGE_SIZE     512
#define NAND_PAGE_MASK     (NAND_PAGE_SIZE - 1)
#define NAND_BLOCK_SIZE    (NAND_PAGE_SIZE * 32)
#define NAND_CHIP_SIZE     (64 << 20)
#endif

// FIXME
#define ATAG_BASE          SDRAM_BASE

#define INITRD_BASE_ADDR   (SDRAM_BASE + SDRAM_SIZE / 4)
#define INITRD_MAX_SIZE    (SDRAM_SIZE / 4)

#define LINUX_BASE_ADDR    (INITRD_BASE_ADDR + SDRAM_SIZE / 4)
#define LINUX_MAX_SIZE     (SDRAM_SIZE / 4)

#define CURR_UART_BASE   (UART0_BASE + CONFIG_UART_INDEX * 0x4000)

// FIXME
#ifdef CONFIG_S3C2410
#define FIFO_FULL  0x200
#define RX_COUNT   0xf
#elif defined(CONFIG_S3C2440)
#define FIFO_FULL  0x4000
#define RX_COUNT   0x3f
#else
#error
#endif

#ifndef __ASSEMBLY__
#include <types.h>

int s3c24x0_interrupt_init(void);

int s3c24x0_timer_init(void);

void calibrate_delay(__u32);

#endif
