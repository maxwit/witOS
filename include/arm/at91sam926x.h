#pragma once

#ifdef CONFIG_AT91SAM9261
#include <arm/at91sam9261_register.h>
#elif defined(CONFIG_AT91SAM9263)
#include <arm/at91sam9263_register.h>
#else
#error "AT91 platform not defined!"
#endif

#define AT91SAM926X_PA_AIC           0xfffff000
	#define AIC_SMR(n)               ((n) * 4)
	#define AIC_SVR(n)		         (0x80 + ((n) * 4))
	#define AIC_IVR                       0x100
	#define AIC_FVR                       0x104
	#define AIC_ISR                       0x108
	#define AIC_IPR                       0x10c
	#define AIC_IMR                       0x110
	#define AIC_CISR                      0x114
	#define AIC_IECR                      0x120
	#define AIC_IDCR                      0x124
	#define AIC_ICCR                      0x128
	#define AIC_ISCR                      0x12c
	#define AIC_EOICR                     0x130
	#define AIC_SPU                       0x134
	#define AIC_DCR                       0x138
	#define AIC_FFER                      0x140
	#define AIC_FFDR                      0x144
	#define AIC_FFSR                      0x148

#define MAX_IRQ_NUM          (32 + 5 * 32)

#define PIOA    0
#define PIOB    1
#define PIOC    2
#ifdef CONFIG_AT91SAM9263
#define PIOD    3
#define PIOE    4
#endif

#define PIO_BASE(n) (AT91SAM926X_PA_PIOA + 0x200 * (n))

#define at91_rstc_readl(reg) \
	readl(VA(AT91SAM926X_PA_RSTC + reg))
#define at91_rstc_writel(reg, val) \
	writel(VA(AT91SAM926X_PA_RSTC + reg), val)

#define at91_pmc_readl(reg) \
	readl(VA(AT91SAM926X_PA_PMC + reg))
#define at91_pmc_writel(reg, val) \
	writel(VA(AT91SAM926X_PA_PMC + reg), val)

#define at91_aic_writel(off, val) \
	writel(VA(AT91SAM926X_PA_AIC + off), val)
#define at91_aic_readl(off) \
	readl(VA(AT91SAM926X_PA_AIC + off))

#define at91_smc_readl(reg) \
	readl(VA(AT91SAM926X_PA_SMC + reg))
#define at91_smc_writel(reg, val) \
	writel(VA(AT91SAM926X_PA_SMC + reg), val)

// #define GTH_STACK_BASE	 0x10000
#define AT91SAM926X_SRAM_SIZE  0x10000

#define GBH_STACK_BASE   SVC_STACK_BASE
#define GBH_STACK_LIMIT (FIQ_STACK_BASE - FIQ_STACK_SIZE)

#define SDRAM_CONFIG_RATE_100   0x85227259

#define CONFIG_PERI_A           0xffff0000

// FIXME

#define INITRD_BASE_ADDR   (SDRAM_BASE + SDRAM_SIZE / 4)
#define INITRD_MAX_SIZE    (SDRAM_SIZE / 4)

#define LINUX_BASE_ADDR    (INITRD_BASE_ADDR + SDRAM_SIZE / 4)
#define LINUX_MAX_SIZE     (SDRAM_SIZE / 4)

// FIXME
#define ATAG_BASE          SDRAM_BASE

#define PLLACK_RATE      (MAINCK_RATE * ((MULA) + 1) / (DIVA))
#define MCK_RATE         (PLLACK_RATE / 2)
//
#define ACLK_RATE        PLLACK_RATE
#define HCLK_RATE        MCK_RATE
#define PCLK_RATE        MCK_RATE

#ifndef __ASSEMBLY__
#include <types.h>

int at91sam926x_interrupt_init(void);

int at91sam926x_timer_init(void);

int at91_clock_enable(int);

void at91_gpio_conf_periA(__u32, __u32, int);

void at91_gpio_conf_periB(__u32, __u32, int);

void  at91_gpio_conf_input(__u32, __u32, int);

void  at91_gpio_conf_output(__u32, __u32, int);

void calibrate_delay(__u32);

#endif

