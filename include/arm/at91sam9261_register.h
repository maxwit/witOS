#pragma once

#define SDRAM_BASE       0x20000000
#define SDRAM_SIZE       0x04000000

#define MULA            64
#define DIVA            6
#define MAINCK_RATE      18432000

#define AT91SAM926X_PA_NAND           0x40000000
	#define NAND_DATA                 0x000000
	#define NAND_CMMD                 0x200000
	#define NAND_ADDR                 0x400000

#define AT91SAM926X_PA_UHP            0x00500000

#define AT91SAM926X_PA_LCDC           0x00600000
#define DMABADDR1 0x0000
#define DMABADDR2 0x0004
#define DMAFRMCFG 0x0018
#define DMACON    0x001c
#define LCDCON1   0x0800
#define LCDCON2   0x0804
#define LCDTIM1   0x0808
#define LCDTIM2   0x080C
#define LCDFRMCFG 0x0810
#define LCDFIFO   0x0814
#define PWRCON    0x083c

#define AT91SAM926X_PA_SMC            0xFFFFEC00
	#define SMC_SETUP(n)       ((n << 4) + 0x00)
	#define SMC_PULSE(n)       ((n << 4) + 0x04)
	#define SMC_CYCLE(n)       ((n << 4) + 0x08)
	#define SMC_MODE(n)        ((n << 4) + 0x0c)

#define AT91SAM926X_PA_SYS            0xFFFFEA00

#define AT91SAM926X_PA_SDRAMC         0xFFFFEA00
	#define SDRAMC_MR                   0x00
	#define SDRAMC_TR                   0x04
	#define SDRAMC_CR                   0x08
	#define SDRAMC_HSR                  0x0c
	#define SDRAMC_LPR                  0x10
	#define SDRAMC_IER                  0x14
	#define SDRAMC_IDR                  0x18
	#define SDRAMC_IMR                  0x1c
	#define SDRAMC_ISR                  0x20
	#define SDRAMC_MDR                  0x24

#define SDR_CAS(n)           ((n) << 5)
#define SDR_tWR(n)           ((n) << 8)
#define SDR_tRC(n)           ((n) << 12)
#define SDR_tRP(n)           ((n) << 16)
#define SDR_tRCD(n)          ((n) << 20)
#define SDR_tRAS(n)          ((n) << 24)
#define SDR_tXSR(n)          ((n) << 28)

#define AT91SAM926X_PA_MATRIX         0xFFFFEE00
	#define MATRIX_MCFG                 0x00
	#define MATRIX_SCFG0                0x04
	#define MATRIX_SCFG1                0x08
	#define MATRIX_SCFG2                0x0c
	#define MATRIX_SCFG3                0x10
	#define MATRIX_SCFG4                0x14
	#define MATRIX_TCMR                 0x24
	#define MATRIX_EBICSA               0x30
	#define MATRIX_USBPCR               0x34
	#define MATRIX_VERSION              0x44

#define AT91SAM926X_PA_DBGU           0xFFFFF200
	#define US_CR                      0x000
	#define US_MR                      0x004
	#define US_IER                     0x008
	#define US_IDR                     0x00c
	#define US_IMR                     0x010
	#define US_CSR                     0x014
	#define US_RHR                     0x018
	#define US_THR                     0x01c
	#define US_BRGR                    0x020
	#define US_RTOR                    0x024
	#define US_TTGR                    0x028
	#define US_FIDI                    0x040
	#define US_NER                     0x044
	#define US_IF                      0x04c
	#define US_RPR                     0x100
	#define US_RCR                     0x104
	#define US_TPR                     0x108
	#define US_TCR                     0x10c
	#define US_RNPR                    0x110
	#define US_RNCR                    0x114
	#define US_TNPR                    0x118
	#define US_TNCR                    0x11c
	#define US_PTCR                    0x120
	#define US_PTSR                    0x124

#define AT91SAM926X_PA_PIOA           0xFFFFF400
#define AT91SAM926X_PA_PIOB           0xFFFFF600
#define AT91SAM926X_PA_PIOC           0xFFFFF800
	#define PIO_PER 	0x000
	#define PIO_PDR 	0x004
	#define PIO_PSR 	0x008
	#define PIO_OER 	0x010
	#define PIO_ODR 	0x014
	#define PIO_OSR 	0x018
	#define PIO_IFER	0x020
	#define PIO_IFDR	0x024
	#define PIO_SODR	0x030
	#define PIO_CODR	0x034
	#define PIO_ODSR	0x038
	#define PIO_PDSR	0x03c
	#define PIO_IER 	0x040
	#define PIO_IDR 	0x044
	#define PIO_IMR 	0x048
	#define PIO_ISR 	0x04C
	#define PIO_MDDR	0x054
	#define PIO_PUDR	0x060
	#define PIO_PUER	0x064
	#define PIO_ASR 	0x070
	#define PIO_BSR 	0x074

#define AT91SAM926X_PA_CKGR           0xFFFFFC20

#define AT91SAM926X_PA_PMC            0xFFFFFC00
	#define PMC_SCER                    0x00
	#define PMC_SCDR                    0x04
	#define PMC_SCSR                    0x08
	#define PMC_PCER                    0x10
	#define PMC_PCDR                    0x14
	#define PMC_PCSR                    0x18
	#define PMC_MOR                     0x20
	#define PMC_MCFR                    0x24
	#define PMC_PLLAR                   0x28
	#define PMC_PLLBR                   0x2c
	#define PMC_MCKR                    0x30
	#define PMC_PCKR                    0x40
	#define PMC_IER                     0x60
	#define PMC_IDR                     0x64
	#define PMC_SR                      0x68
	#define PMC_IMR                     0x6c

#define AT91SAM926X_PA_RSTC           0xFFFFFD00
	#define RSTC_CR 						0x00
	#define RSTC_SR 						0x04
	#define RSTC_MR 						0x08

#define AT91SAM926X_PA_SHDWC          0xFFFFFD10

#define AT91SAM926X_PA_RTTC           0xFFFFFD20
	#define RTTC_MR						0x00
	#define RTTC_AR						0x04
	#define RTTC_VR						0x08
	#define RTTC_SR						0x0c

#define AT91SAM926X_PA_PITC           0xFFFFFD30
	#define PITC_MR						0x00
	#define PITC_SR						0x04
	#define PITC_PIVR					0x08
	#define PITC_PIIR					0x0c

#define AT91SAM926X_PA_WDTC           0xFFFFFD40
	#define WDTC_WDCR                   0x00
	#define WDTC_WDMR                   0x04
	#define WDTC_WDSR                   0x08

#define AT91SAM926X_PA_TC0            0xFFFA0000
#define AT91SAM926X_PA_TC1            0xFFFA0040
#define AT91SAM926X_PA_TC2            0xFFFA0080

#define AT91SAM926X_PA_TCB0           0xFFFA0000
#define AT91SAM926X_PA_UDP            0xFFFA4000
#define AT91SAM926X_PA_PDC_MCI        0xFFFA8100
#define AT91SAM926X_PA_MCI            0xFFFA8000
#define AT91SAM926X_PA_TWI            0xFFFAC000

#define AT91SAM926X_PA_PDC_US0        0xFFFB0100
#define AT91SAM926X_PA_US0            0xFFFB0000
#define AT91SAM926X_PA_PDC_US1        0xFFFB4100
#define AT91SAM926X_PA_US1            0xFFFB4000
#define AT91SAM926X_PA_PDC_US2        0xFFFB8100
#define AT91SAM926X_PA_US2            0xFFFB8000
#define AT91SAM926X_PA_PDC_SSC0       0xFFFBC100
#define AT91SAM926X_PA_SSC0           0xFFFBC000
#define AT91SAM926X_PA_PDC_SSC1       0xFFFC0100
#define AT91SAM926X_PA_SSC1           0xFFFC0000
#define AT91SAM926X_PA_PDC_SSC2       0xFFFC4100
#define AT91SAM926X_PA_SSC2           0xFFFC4000
#define AT91SAM926X_PA_PDC_SPI0       0xFFFC8100
#define AT91SAM926X_PA_PDC_SPI1       0xFFFCC100
#define AT91SAM926X_PA_SPI0           0xFFFC8000
#define AT91SAM926X_PA_SPI1           0xFFFCC000
	#define SPI_CR                        0x00
	#define SPI_MR                        0x04
	#define SPI_RDR                     0x08
	#define SPI_TDR                     0x0c
	#define SPI_SR                        0x10
	#define SPI_IER                     0x14
	#define SPI_IDR                     0x18
	#define SPI_IMR                     0x1c
	#define SPI_CSR0                    0x30
	#define SPI_CSR1                    0x34
	#define SPI_CSR2                    0x38
	#define SPI_CSR3                    0x3c
	//PDC
	#define SPI_RPR                    0x100
	#define SPI_RCR                    0x104
	#define SPI_TPR                    0x108
	#define SPI_TCR                    0x10c
	#define SPI_RNPR                   0x110
	#define SPI_RNCR                   0x114
	#define SPI_TNPR                   0x118
	#define SPI_TNCR                   0x11c
	#define SPI_PTCR                   0x120
	#define SPI_PTSR                   0x124

// FIXME!
#define PIO_NAND      AT91SAM926X_PA_PIOC
#define PIO_NAND_RDY  (0x1 << 15)
#define PIO_NAND_CE   (0x1 << 14)

// FIXME
#define PID_AIC       0
#define PID_SYSIRQ    1
#define PID_PIOA      2
#define PID_PIOB      3
#define PID_PIOC      4
#define PID_RESV      5
#define PID_US0       6
#define PID_US1       7
#define PID_US2       8
#define PID_MCI       9
#define PID_UDP       10
