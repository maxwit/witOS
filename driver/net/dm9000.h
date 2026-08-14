/*
 *  comment here
 */

#pragma once

// FIXME !!
#if defined(CONFIG_S3C2440) || defined(CONFIG_S3C2410)
#define CONFIG_DM9000_IRQ        IRQ_EINT7
#define DM9000_PHYS_BASE         0x20000000
#define DM9000_INDEX_PORT        (DM9000_PHYS_BASE + 0x0)
#define DM9000_DATA_PORT         (DM9000_PHYS_BASE + 0x4)
#elif defined(CONFIG_AT91SAM9261)
#define CONFIG_DM9000_IRQ        (32 + 2 * 32 + 11)
#define DM9000_PHYS_BASE         0x30000000
#define DM9000_INDEX_PORT        (DM9000_PHYS_BASE + 0x0)
#define DM9000_DATA_PORT         (DM9000_PHYS_BASE + 0x4)
#elif defined(CONFIG_S3C6410)
#define CONFIG_DM9000_IRQ        INT_EINT(7)
#define DM9000_PHYS_BASE         0x18000000
#define DM9000_INDEX_PORT        (DM9000_PHYS_BASE + 0x0)
#define DM9000_DATA_PORT         (DM9000_PHYS_BASE + 0x4)
#elif defined(CONFIG_BOARD_DEVKIT8000)
#define CONFIG_DM9000_IRQ        0
#define DM9000_PHYS_BASE         0x2c000000
#define DM9000_INDEX_PORT        (DM9000_PHYS_BASE + 0x0)
#define DM9000_DATA_PORT         (DM9000_PHYS_BASE + 0x400)
#endif

#define DM9000_NCR               0x00
#define DM9000_NSR               0x01
#define DM9000_TCR               0x02
#define DM9000_TSR1              0x03
#define DM9000_RCR               0x05
#define DM9000_RSR               0x06
#define DM9000_ROCR              0x07
#define DM9000_BPTR              0x08
#define DM9000_FCTR              0x09
#define DM9000_FCR               0x0A
#define DM9000_EPCR              0x0B
#define DM9000_EPAR              0x0C
#define DM9000_EPDRL             0x0D
#define DM9000_EPDRH             0x0E
#define DM9000_WCR               0x0F
#define DM9000_PAR               0x10
#define DM9000_MAR               0x16
#define DM9000_GPCR              0x1E
#define DM9000_GPR               0x1F
#define DM9000_TRPAL             0x22
#define DM9000_TRPAH             0x23
#define DM9000_RWPAL             0x24
#define DM9000_RWPAH             0x25
#define DM9000_VIDL              0x28
#define DM9000_VIDH              0x29
#define DM9000_PIDL              0x2A
#define DM9000_PIDH              0x2B
#define DM9000_REV               0x2C
#define DM9000_SMCR              0x2F
#define DM9000_MRCMDX            0xF0
#define DM9000_MRCMD             0xF2
#define DM9000_MRRL              0xF4
#define DM9000_MRRH              0xF5
#define DM9000_MWCMDX            0xF6
#define DM9000_MWCMD             0xF8
#define DM9000_MWRL              0xFA
#define DM9000_MWRH              0xFB
#define DM9000_TXPLL             0xFC
#define DM9000_TXPLH             0xFD
#define DM9000_ISR               0xFE
#define DM9000_IMR               0xFF

#define VENDOR_ID_DAVICOM        0x0A46

#define IMR_VAL                  0xaf

// DM9000_EPCR
#define DM9000_PHY_BUSY         (1 << 0)
#define DM9000_PHY_WRITE        (1 << 1)
#define DM9000_PHY_READ         (1 << 2)
#define DM9000_PHY_SELECT       (1 << 3)

// DM9000_EPAR
#define DM9000_PHY_INTER        (1 << 6)
