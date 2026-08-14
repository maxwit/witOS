#pragma once

#define SDRAM_BASE       0x20000000
#define SDRAM_SIZE       0x04000000

#define MULA            31
#define DIVA            3
#define MAINCK_RATE      18432000

#define AT91SAM926X_PA_NAND           0x40000000
	#define NAND_DATA                 0x000000
	#define NAND_CMMD				  0x400000
	#define NAND_ADDR                 0x200000

#define AT91SAM926X_PA_RSTC           0xFFFFFD00
	#define RSTC_CR 						0x00
	#define RSTC_SR 						0x04
	#define RSTC_MR 						0x08

#define AT91SAM9263_PA_EMAC              0xFFFBC000
	#define EMAC_NCR				0x00
	#define EMAC_NCFGR				0x04
	#define EMAC_NSR				0x08
	#define EMAC_TSR				0x14
	#define EMAC_RBQP				0x18
	#define EMAC_TBQP				0x1c
	#define EMAC_RSR				0x20
	#define EMAC_ISR				0x24
	#define EMAC_IER				0x28
	#define EMAC_IDR				0x2c
	#define EMAC_IMR				0x30
	#define EMAC_MAN				0x34
	#define EMAC_PTR				0x38
	#define EMAC_PFR				0x3c
	#define EMAC_FTO				0x40
	#define EMAC_SCF				0x44
	#define EMAC_MCF				0x48
	#define EMAC_FRO				0x4c
	#define EMAC_FCSE				0x50
	#define EMAC_ALE				0x54
	#define EMAC_DTF				0x58
	#define EMAC_LCOL				0x5c
	#define EMAC_EXCOL				0x60
	#define EMAC_TUND				0x64
	#define EMAC_CSE				0x68
	#define EMAC_RRE				0x6c
	#define EMAC_ROVR				0x70
	#define EMAC_RSE				0x74
	#define EMAC_ELE				0x78
	#define EMAC_RJA				0x7c
	#define EMAC_USF				0x80
	#define EMAC_STE				0x84
	#define EMAC_RLE				0x88
	#define EMAC_TPF				0x8c
	#define EMAC_HRB				0x90
	#define EMAC_HRT				0x94
	#define EMAC_SA1B				0x98
	#define EMAC_SA1T				0x9c
	#define EMAC_SA2B				0xa0
	#define EMAC_SA2T				0xa4
	#define EMAC_SA3B				0xa8
	#define EMAC_SA3T				0xac
	#define EMAC_SA4B				0xb0
	#define EMAC_SA4T				0xb4
	#define EMAC_TID				0xb8
	#define EMAC_TPQ				0xbc
	#define EMAC_USRIO				0xc0
	#define EMAC_WOL				0xc4

#define AT91SAM926X_PA_SMC            0xFFFFE400
	#define SMC_SETUP(n)       ((n << 4) + 0x00)
	#define SMC_PULSE(n)       ((n << 4) + 0x04)
	#define SMC_CYCLE(n)       ((n << 4) + 0x08)
	#define SMC_MODE(n)        ((n << 4) + 0x0c)

#define AT91SAM926X_PA_SDRAMC         0xFFFFE200
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

#define AT91SAM926X_PA_MATRIX         0xFFFFEC00
	#define MATRIX_TCMR               0x114
	#define MATRIX_EBI0CSA            0x120
	#define MATRIX_EBI1CSA			  0x124

#define AT91SAM926X_HECC0_MR          0xFFFFE004

#define AT91SAM926X_PA_DBGU           0xFFFFEE00
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

#define AT91SAM926X_PA_PIOA           0xFFFFF200
#define AT91SAM926X_PA_PIOB           0xFFFFF400
#define AT91SAM926X_PA_PIOC           0xFFFFF600
#define AT91SAM926X_PA_PIOD           0xFFFFF800
#define AT91SAM926X_PA_PIOE           0xFFFFFA00
	#define PIO_PER                    0x000
	#define PIO_PDR                    0x004
	#define PIO_PSR                    0x008
	#define PIO_OER                    0x010
	#define PIO_ODR                    0x014
	#define PIO_OSR                    0x018
	#define PIO_IFER                   0x020
	#define PIO_IFDR                   0x024
	#define PIO_SODR                   0x030
	#define PIO_CODR                   0x034
	#define PIO_ODSR                   0x038
	#define PIO_PDSR                   0x03c
	#define PIO_IER                    0x040
	#define PIO_IDR                    0x044
	#define PIO_IMR                    0x048
	#define PIO_ISR                    0x04C
	#define PIO_MDDR                   0x054
	#define PIO_PUDR                   0x060
	#define PIO_PUER                   0x064
	#define PIO_ASR                    0x070
	#define PIO_BSR                    0x074

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

#define AT91SAM926X_PA_SPI0           0xFFFA4000
#define AT91SAM926X_PA_SPI1           0xFFFA8000
	#define SPI_CR                      0x00
	#define SPI_MR                      0x04
	#define SPI_RDR                     0x08
	#define SPI_TDR                     0x0c
	#define SPI_SR                      0x10
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

// FIXME
#define PIO_NAND     AT91SAM926X_PA_PIOA
#define PIO_NAND_RDY (0x1 << 22)
#define PIO_NAND_CE  (0x1 << 15)

// FIXME
// FIXME
#define PID_AIC       0
#define PID_SYSC      1
#define PID_PIOA      2
#define PID_PIOB      3
#define PID_PIOC      4
#define PID_PIOD      4
#define PID_PIOE      4
#define PID_US0       7
#define PID_US1       8
#define PID_US2       9
#define PID_MCI0      10
#define PID_MCI1      11

#define PID_EMAC      21

#define PID_UDP       24

