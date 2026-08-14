#pragma once

// Memory Controller

// Interrupt Controller
#define INTCON_VIC0_BASE 0x71200000
#define INTCON_VIC1_BASE 0x71300000

// GPIO
#define GPIO_BASE 0x7F008000
#define GPC_BASE			0x7F008040
#define GPCCON				      0x00
#define GPCDAT				      0x04
#define GPG_BASE			(GPIO_BASE + 0xc0)
#define GPCON                    0x00
#define GPDAT				     0x04

#define VIC0_IRQREG(x)   ((x) + INTCON_VIC0_BASE)
#define VIC1_IRQREG(x)   ((x) + INTCON_VIC1_BASE)
#define EINTREG(x)  ((x) + GPIO_BASE)

#define VIC0_IRQSTATUS		VIC0_IRQREG(0x000)
#define VIC0_INTSELECT	    VIC0_IRQREG(0x00C)
#define VIC0_INTENABLE	    VIC0_IRQREG(0x010)
#define VIC0_INTENCLEAR		VIC0_IRQREG(0x014)
#define VIC0_VECTADDR0		VIC0_IRQREG(0x100)
#define VIC0_VECTADDR1		VIC0_IRQREG(0x104)
#define VIC0_VECTADDR2		VIC0_IRQREG(0x108)
#define VIC0_VECTADDR3		VIC0_IRQREG(0x10c)
#define VIC0_VECTADDR4		VIC0_IRQREG(0x110)
#define VIC0_VECTADDR5		VIC0_IRQREG(0x114)
#define VIC0_VECTADDR6		VIC0_IRQREG(0x118)
#define VIC0_VECTADDR7		VIC0_IRQREG(0x11c)
#define VIC0_VECTADDR8		VIC0_IRQREG(0x120)
#define VIC0_VECTADDR9		VIC0_IRQREG(0x124)
#define VIC0_VECTADDR10		VIC0_IRQREG(0x128)
#define VIC0_VECTADDR11		VIC0_IRQREG(0x12c)
#define VIC0_VECTADDR12		VIC0_IRQREG(0x130)
#define VIC0_VECTADDR13		VIC0_IRQREG(0x134)
#define VIC0_VECTADDR14		VIC0_IRQREG(0x138)
#define VIC0_VECTADDR15		VIC0_IRQREG(0x13c)
#define VIC0_VECTADDR16		VIC0_IRQREG(0x140)
#define VIC0_VECTADDR17		VIC0_IRQREG(0x144)
#define VIC0_VECTADDR18		VIC0_IRQREG(0x148)
#define VIC0_VECTADDR19		VIC0_IRQREG(0x14c)
#define VIC0_VECTADDR20		VIC0_IRQREG(0x150)
#define VIC0_VECTADDR21		VIC0_IRQREG(0x154)
#define VIC0_VECTADDR22		VIC0_IRQREG(0x158)
#define VIC0_VECTADDR23		VIC0_IRQREG(0x15c)
#define VIC0_VECTADDR24		VIC0_IRQREG(0x160)
#define VIC0_VECTADDR25		VIC0_IRQREG(0x164)
#define VIC0_VECTADDR26		VIC0_IRQREG(0x168)
#define VIC0_VECTADDR27		VIC0_IRQREG(0x16c)
#define VIC0_VECTADDR28		VIC0_IRQREG(0x170)
#define VIC0_VECTADDR29		VIC0_IRQREG(0x174)
#define VIC0_VECTADDR30		VIC0_IRQREG(0x178)
#define VIC0_VECTADDR31		VIC0_IRQREG(0x17c)
#define VIC0_ADDRESS		VIC0_IRQREG(0xf00)

#define VIC1_IRQSTATUS		VIC1_IRQREG(0x000)
#define VIC1_INTSELECT	    VIC1_IRQREG(0x00C)
#define VIC1_INTENABLE	    VIC1_IRQREG(0x010)
#define VIC1_INTENCLEAR		VIC1_IRQREG(0x014)
#define VIC1_VECTADDR0		VIC1_IRQREG(0x100)
#define VIC1_VECTADDR1		VIC1_IRQREG(0x104)
#define VIC1_VECTADDR2		VIC1_IRQREG(0x108)
#define VIC1_VECTADDR3		VIC1_IRQREG(0x10c)
#define VIC1_VECTADDR4		VIC1_IRQREG(0x110)
#define VIC1_VECTADDR5		VIC1_IRQREG(0x114)
#define VIC1_VECTADDR6		VIC1_IRQREG(0x118)
#define VIC1_VECTADDR7		VIC1_IRQREG(0x11c)
#define VIC1_VECTADDR8		VIC1_IRQREG(0x120)
#define VIC1_VECTADDR9		VIC1_IRQREG(0x124)
#define VIC1_VECTADDR10		VIC1_IRQREG(0x128)
#define VIC1_VECTADDR11		VIC1_IRQREG(0x12c)
#define VIC1_VECTADDR12		VIC1_IRQREG(0x130)
#define VIC1_VECTADDR13		VIC1_IRQREG(0x134)
#define VIC1_VECTADDR14		VIC1_IRQREG(0x138)
#define VIC1_VECTADDR15		VIC1_IRQREG(0x13c)
#define VIC1_VECTADDR16		VIC1_IRQREG(0x140)
#define VIC1_VECTADDR17		VIC1_IRQREG(0x144)
#define VIC1_VECTADDR18		VIC1_IRQREG(0x148)
#define VIC1_VECTADDR19		VIC1_IRQREG(0x14c)
#define VIC1_VECTADDR20		VIC1_IRQREG(0x150)
#define VIC1_VECTADDR21		VIC1_IRQREG(0x154)
#define VIC1_VECTADDR22		VIC1_IRQREG(0x158)
#define VIC1_VECTADDR23		VIC1_IRQREG(0x15c)
#define VIC1_VECTADDR24		VIC1_IRQREG(0x160)
#define VIC1_VECTADDR25		VIC1_IRQREG(0x164)
#define VIC1_VECTADDR26		VIC1_IRQREG(0x168)
#define VIC1_VECTADDR27		VIC1_IRQREG(0x16c)
#define VIC1_VECTADDR28		VIC1_IRQREG(0x170)
#define VIC1_VECTADDR29		VIC1_IRQREG(0x174)
#define VIC1_VECTADDR30		VIC1_IRQREG(0x178)
#define VIC1_VECTADDR31		VIC1_IRQREG(0x17c)
#define VIC1_ADDRESS		VIC1_IRQREG(0xf00)

#define EINT0MASK		EINTREG(0x920)
#define EINT0PEND		EINTREG(0X924)

// IRQ Number
#define S3C6410_IRQ(x) (x)
//vic0
#define INT_EINT0		S3C6410_IRQ(0)
#define INT_EINT1		S3C6410_IRQ(1)
#define INT_RTC_TIC		S3C6410_IRQ(2)
#define INT_CAMIF_C		S3C6410_IRQ(3)
#define INT_CAMIF_P		S3C6410_IRQ(4)
#define INT_I2C1		S3C6410_IRQ(5)
#define INT_I2S			S3C6410_IRQ(6) //FIX ME
#define INT_RESERVED	S3C6410_IRQ(7)
#define INT_3D          S3C6410_IRQ(8)
#define INT_POST0		S3C6410_IRQ(9)
#define INT_ROTATOR		S3C6410_IRQ(10)
#define INT_2D			S3C6410_IRQ(11)
#define INT_TVENC		S3C6410_IRQ(12)
#define INT_SCALER		S3C6410_IRQ(13)
#define INT_BATF		S3C6410_IRQ(14)
#define INT_JPEG		S3C6410_IRQ(15)
#define INT_MFC			S3C6410_IRQ(16)
#define INT_SDMA0		S3C6410_IRQ(17)
#define INT_SDMA1		S3C6410_IRQ(18)
#define INT_ARM_DMAERR	S3C6410_IRQ(19)
#define INT_ARM_DMA		S3C6410_IRQ(20)
#define INT_ARM_DMAS	S3C6410_IRQ(21)
#define INT_KEYPAD		S3C6410_IRQ(22)
#define INT_TIMER0		S3C6410_IRQ(23)
#define INT_TIMER1		S3C6410_IRQ(24)
#define INT_TIMER2		S3C6410_IRQ(25)
#define INT_WDT			S3C6410_IRQ(26)
#define INT_TIMER3		S3C6410_IRQ(27)
#define INT_TIMER4		S3C6410_IRQ(28)
#define INT_LCD0		S3C6410_IRQ(29)
#define INT_LCD1		S3C6410_IRQ(30)
#define INT_LCD2		S3C6410_IRQ(31)
//vic1
#define INT_EINT2		S3C6410_IRQ(32)
#define INT_EINT3		S3C6410_IRQ(33)
#define INT_PCM0		S3C6410_IRQ(34)
#define INT_PCM1		S3C6410_IRQ(35)
#define INT_AC97		S3C6410_IRQ(36)
#define INT_UART0		S3C6410_IRQ(37)
#define INT_UART1		S3C6410_IRQ(38)
#define INT_UART2		S3C6410_IRQ(39)
#define INT_UART3		S3C6410_IRQ(40)
#define INT_DMA0		S3C6410_IRQ(41)
#define INT_DMA1		S3C6410_IRQ(42)
#define INT_ONENAND0	S3C6410_IRQ(43)
#define INT_ONENAND1	S3C6410_IRQ(44)
#define INT_NFC			S3C6410_IRQ(45)
#define INT_CFC			S3C6410_IRQ(46)
#define INT_UHOST		S3C6410_IRQ(47)
#define INT_SPI0		S3C6410_IRQ(48)
#define INT_SPI1		S3C6410_IRQ(49)
#define INT_I2C0		S3C6410_IRQ(50)
#define INT_HSItx		S3C6410_IRQ(51)
#define INT_HSIrx		S3C6410_IRQ(52)
#define INT_EINT4		S3C6410_IRQ(53)
#define INT_MSM			S3C6410_IRQ(54)
#define INT_HOSTIF		S3C6410_IRQ(55)
#define INT_HSMMC0		S3C6410_IRQ(56)
#define INT_HSMMC1		S3C6410_IRQ(57)
#define INT_OTG			S3C6410_IRQ(58)
#define INT_IrDA		S3C6410_IRQ(59)
#define INT_RTC_ALARM	S3C6410_IRQ(60)
#define INT_SEC			S3C6410_IRQ(61)
#define INT_PENDNUP		S3C6410_IRQ(62)
#define INT_ADC			S3C6410_IRQ(63)

#define MAX_INTERNAL_IRQ INT_ADC
#define INT_EINT(n)		(INT_ADC + 1 + (n))

//FIXME,not include external interrupt group1~group9
#define MAX_IRQ_NUM	INT_EINT(27)

#define EXTINT_LOWLEV	 (0x00)
#define EXTINT_HILEV	 (0x01)
#define EXTINT_FALLEDGE	 (0x02)
#define EXTINT_RISEEDGE	 (0x04)
#define EXTINT_BOTHEDGE	 (0x06)

// Clock and Power Management
#define CLOCK_BASE      0x4c000000
#define LOCKTIME              0x00
#define MPLLCON               0x04
#define UPLLCON               0x08
#define CLKDIVN               0x14

// LCD
#define S3C64XX_LCD_BASE 0x4d000000
#define LCDCON1                0x00
#define LCDCON2                0x04
#define LCDCON3                0x08
#define LCDCON4                0x0c
#define LCDCON5                0x10
#define LCDSADDR1              0x14
#define LCDSADDR2              0x18
#define LCDSADDR3              0x1c
#define LCDINTPND              0x54
#define LCDSRCPND              0x58
#define LCDINTMSK              0x5c
#define TCONSEL                0x60

#define VIDCON0        0x00
#define VIDCON1        0x04
#define VIDCON2        0x08
#define VIDTCON0       0x10
#define VIDTCON1       0x14
#define VIDTCON2       0x18
#define WINCON0        0x20
#define WINCON1        0x24
#define WINCON2        0x28
#define WINCON3        0x2C
#define WINCON4        0x30

#define VIDOSD0A        0x40
#define VIDOSD0B        0x44
#define VIDOSD0C        0x48
#define VIDOSD1A        0x50
#define VIDOSD1B        0x54
#define VIDOSD1C        0x58

#define VIDOSD1D        0x5C
#define VIDOSD2A        0x60
#define VIDOSD2B        0x64
#define VIDOSD2C        0x68
#define VISOSD2D        0x6C
#define VIDOSD3A        0x70
#define VIDOSD3B        0x74
#define VIDOSD3C        0x78
#define VIDOSD4A        0x80
#define VIDOSD4B        0x84
#define VIDOSD4C        0x88
#define VIDW00ADD0B0    0xA0
#define VIDW00ADD0B1    0xA4
#define VIDW01ADD0B0    0xA8
#define VIDW01ADD0B1    0xAC
#define VIDW02ADD0      0xB0
#define VIDW03ADD0      0xB8
#define VIDW04ADD0      0xC0
#define VIDW00ADD1B0    0xD0
#define VIDW00ADD1B1    0xD4
#define VIDW01ADD1B0    0xD8
#define VIDW01ADD1B1    0xDC
#define VIDW02ADD1      0x00E0
#define VIDW03ADD1      0x00E8
#define VIDW04ADD1      0x00F0
#define VIDW00ADD2      0x0100
#define VIDW01ADD2      0x0104
#define VIDW02ADD2      0x0108
#define VIDW03ADD2      0x010C
#define VIDW04ADD2      0x0110
#define VIDINTCON0      0x0130
#define VIDINTCON1      0x0134
#define W1KEYCON0       0x0140
#define W1KEYCON1       0x0144
#define W2KEYCON0       0x0148
#define W2KEYCON1       0x014C
#define W3KEYCON0       0x0150
#define W3KEYCON1       0x0154
#define W4KEYCON0       0x0158
#define W4KEYCON1       0x015C
//
// #define BPP16_TFT    12
// #define BPP24_TFT    13

#define HCLOCK           100000000
#define PCLOCK_50M        50000000

// NAND
#define NAND_CTRL_BASE 0x70200000
#define NF_CONF        0x00
#define NF_CONT        0x04
#define NF_CMMD        0x08
#define NF_ADDR        0x0c
#define NF_DATA        0x10
#define NF_STAT        0x28
#define NF_MECC0       0x34

// FIXME
#define tALS  12
#define tWP   12
#define tALH  5

#define TACLS  (tALS - tWP)
#define TWRPH0  tWP
#define TWRPH1  tALH

// UART is OK
#define UART_BASE    0x7F005000
//#define UART_BASE(n)  (UART_BASE + (n) * xxx)
#define ULCON        0x00
#define UCON         0x04
#define UFCON        0x08
#define UMCON        0x0C
#define UTRSTAT      0x10
#define UERSTAT      0x14
#define UFSTAT       0x18
#define UMSTAT       0x1C
#define UTXH         0x20
#define URXH         0x24
#define UBRDIV       0x28
#define UDIVSLOT     0x2C

#define CONFIG_UART_ENABLE_FIFO
#define CURR_UART_BASE   (UART_BASE + CONFIG_UART_INDEX * 0x400)

// FIXME: to support CONFIG_DEFAULT_UART
#define s3c_uart_readb(reg)        readb(VA(UART_BASE + reg))
#define s3c_uart_readl(reg)        readl(VA(UART_BASE + reg))
#define s3c_uart_writeb(reg, val)  writeb(VA(UART_BASE + reg), val)
#define s3c_uart_writel(reg, val)  writel(VA(UART_BASE + reg), val)

// SDRAMC is OK
#define DRAMC_BASE     0x7E001000
#define P1MEMSTAT      0x000
#define P1MEMCCMD      0x004
#define P1DIRECTCMD    0x008
#define P1MEMCFG       0x00C
#define P1REFRESH      0x010
#define P1CASLAT       0x014
#define P1T_DQSS       0x018
#define P1T_MRD        0x01C
#define P1T_RAS        0x020
#define P1T_RC         0x024
#define P1T_RCD        0x028
#define P1T_RFC        0x02C
#define P1T_RP         0x030
#define P1T_RRD        0x034
#define P1T_WR         0x038
#define P1T_WTR        0x03C
#define P1T_XP         0x040
#define P1T_XSR        0x044
#define P1T_ESR        0x048
#define P1MEMCFG2      0x04C
#define P1ID_0_CFG     0x100
#define P1CHIP_0_CFG   0x200
#define P1CHIP_1_CFG   0x204
#define P1USER_STAT    0x300
#define P1USER_CFG     0x304

// Watchdog
#define WATCHDOG_BASE   0x7E004000
#define WTCON                 0x00
#define WTDATA                0x04
#define WTCNT                 0x08

// SPI SPECIAL REGISTER
#define CH_CFG				0x7F00B000
#define CLK_CFG				0x7F00B004
#define MODE_CFG			0x7F00B008
#define SPI_INT_EN			0x7F00B010
#define CS_REG				0x7F00B00C
#define SPI_STATUS			0x7F00B014
#define SPI_TX_DATA			0x7F00B018
#define SPI_RX_DATA			0X7F00B01C
#define PACKET_CNT_REG		0x7F00B020
#define FEEDBACK            0x7F00B02C

#define TCFG0     0x7F006000
#define TCFG1     0x7F006004
#define TCON      0x7F006008
#define TCNTB0    0x7F00600c
#define TCMPB0    0x7F006010
#define TCNTB1    0x7F006018
#define TCMPB1    0x7F00601c
#define TIN_CSTAT 0x7F006044

#define S3C64XX_GPBCON   0x56000010
#define S3C64XX_GPBDAT   0x56000014
#define S3C64XX_GPBUP    0x56000018

#define S3C64XX_GPCCON   0x56000020
#define S3C64XX_GPCDAT   0x56000024
#define S3C64XX_GPCUP    0x56000028

#define S3C64XX_GPDCON   0x56000030
#define S3C64XX_GPDDAT   0x56000034
#define S3C64XX_GPDUP    0x56000038

#define S3C64XX_GPECON   0x56000040
#define S3C64XX_GPEDAT   0x56000044
#define S3C64XX_GPEUP    0x56000048

#define S3C64XX_GPFCON   0x56000050
#define S3C64XX_GPFDAT   0x56000054
#define S3C64XX_GPFUP    0x56000058

// SYSCON is OK
#define SYSCON_BASE  0x7E00F000
#define APLL_LOCK    0x000
#define MPLL_LOCK    0x004
#define EPLL_LOCK    0x008
#define APLL_CON     0x00C
#define MPLL_CON     0x010
#define EPLL_CON0    0x014
#define EPLL_CON1    0x018
#define CLK_SRC      0x01C
#define CLK_DIV0     0x020
#define CLK_DIV1     0x024
#define CLK_DIV2     0x028
#define MISC_CON     0x838
#define OTHERS       0x900

#define syscon_write(reg, val)  writel(VA(SYSCON_BASE + reg), val)
#define syscon_read(reg)        readl(VA(SYSCON_BASE + reg))

// SROM controller
#define SROM_BASE    0x70000000
#define SROM_BW      0x00
#define SROM_BC0     0x04
#define SROM_BC1     0x08

#define MMC0_BASE    0x7c200000
#define MMC1_BASE    0x7c300000
#define MMC2_BASE    0x7c400000
#define BLKSIZE      0x04
#define BLKCNT       0x06
#define ARGUMENT     0x08
#define TRANSMOD     0x0c
#define CMDREG       0x0e
#define RESPREG0     0x10
#define RESPREG1     0x14
#define RESPREG2     0x18
#define RESPREG3     0x1c
#define BDAT         0x20
#define PRNSTS       0x24
#define HOSTCTRL     0x28
#define PWRCON      0x29
#define BLKGAP       0x2a
#define WAKUP        0x2b
#define CLKCON       0x2c
#define TIMEOUTCON   0x2e
#define SWRST        0x2f
#define NORINTSTS    0x30
#define ERRINTSTS    0x32
#define NORINTSTSEN  0x34
#define ERRINTSTSE   0x36
#define NORINTSIGEN  0x38
#define ERRINTSIGEN  0x3a
#define ACMD12ERRSTS 0x3c
#define CAPAREG      0x40
#define MAXCURR      0x48
#define CONTRL2      0x80
#define CONTRL3      0x84
#define CONTRL4      0x8c
#define FEAER        0x50
#define FEERR        0x52
#define ADMAERR      0x54
#define ADMASYSADDR  0x58
#define HCVER        0xfe

// FIXME
#define LCD_BASE  0x77100000

#define MIFPCON   0x7410800C

#define SPCON     0x7F0081A0
#define GPI_CON   0x7F008100
#define GPJ_CON   0x7F008120

#define GPN_CON         0x7f008830
#define EINT0_CON       0x7f008900
#define EINT0_MASK      0x7f008920
#define VICO_INT_ENABLE 0x71200010

// reset
#define RST_BASE	0x7E00F900
#define RST_STAT	0x4