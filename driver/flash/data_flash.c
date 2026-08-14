#include <flash/data_flash.h>
#include <mtd/mtd.h>
#include <bitops.h>
#include <arm/at91sam926x.h>

static struct part_attr g_defDataFlashParts[] =
{
	{
		PT_BL_GTH,
		KB(4) / 512 * 528,
	},
	{
		PT_BL_GBH,
		KB(256 - 4) / 512 * 528,
	},
	{
		PT_BL_GCONF,
		(MB(2) - KB(256)) / 512 * 528,
	},
};

static int DataFlashSpiEnable(__u32);

static int DataFlashSpiOps(struct mtd_info *);

static int DataFlashStatus(struct mtd_info *, __u8 *);

static int DataFlashWaitReady(struct mtd_info *);

static int DataFlashErase(struct mtd_info *, struct erase_info *);

static int DataFlashWrite(struct mtd_info *, __u32, __u32, __u32 *, const __u8 *);

static int DataFlashRead(struct mtd_info *, __u32, __u32, __u32 *, __u8 *);

static int DataFlashAdd(const char *, __u32, __u32, __u32);

static int DataFlashSpiEnable(__u32 ulCS)
{
	volatile struct AT9261_SPI *pSpi0 = (struct AT9261_SPI *)AT91SAM926X_PA_SPI0;

	switch (ulCS) {
	case 0:
		pSpi0->dwSPIMR &= 0xFFF0FFFF;
		pSpi0->dwSPIMR |= ((AT91C_SPI_PCS0_SERIAL_DATAFLASH) & AT91C_SPI_PCS);

		break;

	default:
		return -EINVAL;
	}

	pSpi0->dwSPICR = AT91C_SPI_SPIEN;

	return 0;
}

static int DataFlashSpiOps(struct mtd_info *mtd)
{
	struct DataFlash *pDataflash = container_of(flash, struct DataFlash, parent);
	__u32 ulTimeout;
	__u32 ulSrcAddr;
	volatile struct AT9261_SPI *pSpi0 = (struct AT9261_SPI *)AT91SAM926X_PA_SPI0;

	DPRINT("\ncmd:%02x%02x%02x%02x\n",
			*((char *)pDataflash->stOprMsg.pRxCmdBuf),
			*((char *)pDataflash->stOprMsg.pRxCmdBuf + 1),
			*((char *)pDataflash->stOprMsg.pRxCmdBuf + 2),
			*((char *)pDataflash->stOprMsg.pRxCmdBuf + 3)
		   );

	DPRINT("nRxCmdLen=%x\nnTxCmdLen=%x\nhasData=%x\npRxDataBuf=0x%08x\npTxDataBuf=0x%08x\nnRxDataLen=%x\nnTxDataLen=%x\n",
			pDataflash->stOprMsg.nRxCmdLen,
			pDataflash->stOprMsg.nTxCmdLen,
			pDataflash->stOprMsg.hasData,
			pDataflash->stOprMsg.pRxDataBuf,
			pDataflash->stOprMsg.pTxDataBuf,
			pDataflash->stOprMsg.nRxDataLen,
			pDataflash->stOprMsg.nTxDataLen
		   );

	pSpi0->dwSPIPTCR = AT91C_PDC_TXTDIS + AT91C_PDC_RXTDIS;

	pSpi0->dwSPIRPR = (__u32)pDataflash->stOprMsg.pRxCmdBuf;
	pSpi0->dwSPITPR = (__u32)pDataflash->stOprMsg.pTxCmdBuf;

	pSpi0->dwSPIRCR = (__u16)pDataflash->stOprMsg.nRxCmdLen;
	pSpi0->dwSPITCR = (__u16)pDataflash->stOprMsg.nTxCmdLen;

	if (true == pDataflash->stOprMsg.hasData) {
		pSpi0->dwSPIRNPR = (__u32)pDataflash->stOprMsg.pRxDataBuf;
		pSpi0->dwSPITNPR = (__u32)pDataflash->stOprMsg.pTxDataBuf;

		pSpi0->dwSPIRNCR = (__u16)pDataflash->stOprMsg.nRxDataLen;
		pSpi0->dwSPITNCR = (__u16)pDataflash->stOprMsg.nTxDataLen;
	}

	ulTimeout = 0;
	pSpi0->dwSPIPTCR = AT91C_PDC_TXTEN + AT91C_PDC_RXTEN;
	while(!(pSpi0->dwSPISR & AT91C_SPI_RXBUFF) && ulTimeout < CFG_SPI_WRITE_TOUT) // AT91C_SPI_TXBUFE is error!!
		ulTimeout++;

	pSpi0->dwSPIPTCR = AT91C_PDC_TXTDIS + AT91C_PDC_RXTDIS;

	if (ulTimeout >= CFG_SPI_WRITE_TOUT)
		return -EFAULT;

	return 0;
}

static int DataFlashStatus(struct mtd_info *mtd, __u8 *pbStatus)
{
	struct DataFlash *pDataFlash = container_of(flash, struct DataFlash, parent);
	__u8 *pCmd;

	pDataFlash->stOprMsg.pRxCmdBuf = pDataFlash->stOprMsg.pTxCmdBuf = pCmd = pDataFlash->bCommand;
	pDataFlash->stOprMsg.nRxCmdLen = pDataFlash->stOprMsg.nTxCmdLen = 2;
	pDataFlash->stOprMsg.hasData   = false;

	pCmd[0] = OP_READ_STATUS;
	pCmd[1] = 0;

	if (DataFlashSpiOps(flash) < 0)
		return -EINVAL;

	*pbStatus = *((__u8 *)pDataFlash->stOprMsg.pRxCmdBuf + 1);

	DPRINT("status:%02x\n", *pbStatus);

	return 0;
}

static int DataFlashWaitReady(struct mtd_info *mtd)
{
	struct DataFlash *pDataFlash = container_of(flash, struct DataFlash, parent);
	int	ret;
	__u8 status;

	DataFlashSpiEnable(pDataFlash->ulChipSelect);

	for (;;) {
		ret = DataFlashStatus(flash, &status);
		if (ret < 0) {
			DPRINT("%s: status %d?\n", flash->name, status);
			continue;
		}

		if ((status & (1 << 7)))
			break;
	}

	return 0;
}

static int DataFlashErase(struct mtd_info *mtd, struct erase_info *pErsOp)
{
	struct DataFlash *pDataFlash = container_of(flash, struct DataFlash, parent);
	__u32 blocksize = pDataFlash->block_size, nLen = pErsOp->len;
	__u8   *pCmd;
	__u32 do_block = 0;

	DataFlashSpiEnable(pDataFlash->ulChipSelect);

	if ((pErsOp->addr + pErsOp->len) > flash->chip_size
		|| pErsOp->len % pDataFlash->page_size != 0
		|| pErsOp->addr % pDataFlash->page_size != 0
	   )
	{
		DPRINT("ERROR: address or lenght is not correct!\n");
		return -EINVAL;
	}

	DataFlashWaitReady(flash);

	while (pErsOp->len > 0) {
		__u32	pageaddr;
		int		status;
		int		do_block;

		pageaddr = pErsOp->addr / pDataFlash->page_size;
#if 0
		do_block = ((pageaddr & 0x7) == 0) && (pErsOp->len >= blocksize);
#else
		do_block = false;
#endif
		pageaddr = pageaddr << pDataFlash->nPageShift;

		pDataFlash->stOprMsg.pTxCmdBuf = pDataFlash->stOprMsg.pRxCmdBuf = pCmd = pDataFlash->bCommand;
		pDataFlash->stOprMsg.nTxCmdLen = pDataFlash->stOprMsg.nRxCmdLen = 4;
		pDataFlash->stOprMsg.hasData   = false;

		pCmd[0] = do_block ? OP_ERASE_BLOCK : OP_ERASE_PAGE;
		pCmd[1] = (pageaddr & 0x00FF0000) >> 16;
		pCmd[2] = (pageaddr & 0x0000FF00) >> 8;
		pCmd[3] = 0;

		status = DataFlashSpiOps(flash);
		(void) DataFlashWaitReady(flash);

		if (status < 0) {
			DPRINT( "%s: erase %stOpMsg, err %d\n", flash->name, pageaddr, status);

			continue;
		}

		if (NULL != flash->callback_func && NULL != flash->callback_args) {
			flash->callback_args->nPageIndex = pageaddr >> pDataFlash->nPageShift;
			flash->callback_func(flash, flash->callback_args);
		}

		if (do_block) {
			pErsOp->addr += blocksize;
			pErsOp->len  -= blocksize;
		} else {
			pErsOp->addr += pDataFlash->page_size;
			pErsOp->len  -= pDataFlash->page_size;
		}

	}

	pErsOp->state = FLASH_ERASE_DONE;

	return 0;
}

static int DataFlashWrite(struct mtd_info *mtd, __u32 to, __u32 len,
							   __u32 *retlen, const __u8 *buf)
{
	struct DataFlash *pDataFlash = container_of(flash, struct DataFlash, parent);
	__u32 pageaddr, addr, offset, writelen;
	__u32 remaining = len;
	__u8 *writebuf = (__u8 *)buf;
	__u8 *pbCmd, *pTmpBuf;
	int	status = -EINVAL;

	DataFlashSpiEnable(pDataFlash->ulChipSelect);

	DPRINT("%s: write 0x%x..0x%x\n", flash->name, (unsigned)to, (unsigned)(to + len));

	*retlen = 0;

	if (0 == len)
		return 0;
	if ((to + len) > flash->chip_size)
		return -EINVAL;

	pTmpBuf = malloc(pDataFlash->page_size);
	if (NULL == pTmpBuf) {
		DPRINT("%s, %d\n", __func__, __LINE__);
		return -ENOMEM;
	}

	pageaddr = to / pDataFlash->page_size;
	offset   = to % pDataFlash->page_size;
	if (offset + len > pDataFlash->page_size)
		writelen = pDataFlash->page_size - offset;
	else
		writelen = len;

	(void) DataFlashWaitReady(flash);

	while (remaining > 0) {
		DPRINT("write @ %x:%x len=%x\n", pageaddr, offset, writelen);

		addr = pageaddr << pDataFlash->nPageShift;

		if (writelen != pDataFlash->page_size) {
			pDataFlash->stOprMsg.pTxCmdBuf = pDataFlash->stOprMsg.pRxCmdBuf = pbCmd = pDataFlash->bCommand;
			pDataFlash->stOprMsg.nTxCmdLen = pDataFlash->stOprMsg.nRxCmdLen = 4;
			pDataFlash->stOprMsg.hasData   = false;
			pbCmd[0] = OP_TRANSFER_BUF1;
			pbCmd[1] = (addr & 0x00FF0000) >> 16;
			pbCmd[2] = (addr & 0x0000FF00) >> 8;
			pbCmd[3] = 0;

			DPRINT("TRANSFER: (%x) %x %x %x\n",	pbCmd[0], pbCmd[1], pbCmd[2], pbCmd[3]);

			status = DataFlashSpiOps(flash);
			if (status < 0)
				DPRINT("%s: xfer %u -> %d \n", flash->name, addr, status);

			(void) DataFlashWaitReady(flash);
		}

		addr += offset;
		pDataFlash->stOprMsg.pTxCmdBuf  = pDataFlash->stOprMsg.pRxCmdBuf = pbCmd = pDataFlash->bCommand;
		pDataFlash->stOprMsg.nTxCmdLen  = pDataFlash->stOprMsg.nRxCmdLen = 4;
		pDataFlash->stOprMsg.hasData    = true;
		//pDataFlash->stOprMsg.pTxDataBuf = pDataFlash->stOprMsg.pRxDataBuf = writebuf;
		pDataFlash->stOprMsg.pTxDataBuf = writebuf;
		pDataFlash->stOprMsg.pRxDataBuf = pTmpBuf;
		pDataFlash->stOprMsg.nTxDataLen = pDataFlash->stOprMsg.nRxDataLen = writelen;
		pbCmd[0] = OP_PROGRAM_VIA_BUF1;
		pbCmd[1] = (addr & 0x00FF0000) >> 16;
		pbCmd[2] = (addr & 0x0000FF00) >> 8;
		pbCmd[3] = (addr & 0x000000FF);

		DPRINT("PROGRAM: (%x) %x %x %x, len:%x\n", pbCmd[0], pbCmd[1], pbCmd[2], pbCmd[3], writelen);

		status = DataFlashSpiOps(flash);

		if (status < 0)
			DPRINT("%s: pgm %u/%u -> %d \n", flash->name, addr, writelen, status);

		(void) DataFlashWaitReady(flash);

		if (NULL != flash->callback_func && NULL != flash->callback_args) {
			flash->callback_args->nPageIndex = pageaddr;
			flash->callback_func(flash, flash->callback_args);
		}

#ifdef CONFIG_DATAFLASH_WRITE_VERIFY // test ok

		addr = pageaddr << pDataFlash->nPageShift;
		pDataFlash->stOprMsg.pTxCmdBuf = pDataFlash->stOprMsg.pRxCmdBuf = pbCmd = pDataFlash->bCommand;
		pDataFlash->stOprMsg.nTxCmdLen = pDataFlash->stOprMsg.nRxCmdLen = 4;
		pDataFlash->stOprMsg.hasData   = false;
		pbCmd[0] = OP_COMPARE_BUF1;
		pbCmd[1] = (addr & 0x00FF0000) >> 16;
		pbCmd[2] = (addr & 0x0000FF00) >> 8;
		pbCmd[3] = 0;

		DPRINT("COMPARE: (%x) %x %x %x\n", pbCmd[0], pbCmd[1], pbCmd[2], pbCmd[3]);

		status = DataFlashSpiOps(flash);
		if (status < 0)
			DPRINT("%s: compare %u -> %d \n", flash->name, addr, status);

		status = DataFlashWaitReady(flash);

		if ((status & (1 << 6)) == 1) {
			DPRINT("%s: compare page %u, err %d\n",	flash->name, pageaddr, status);
			remaining = 0;
			status = -EIO;
			break;
		} else
			status = 0;
#endif

		remaining = remaining - writelen;
		offset    = 0;
		writebuf += writelen;
		*retlen  += writelen;
		pageaddr++;

		if (remaining > pDataFlash->page_size)
			writelen = pDataFlash->page_size;
		else
			writelen = remaining;

	}

	free(pTmpBuf);

	return status;
}

static int DataFlashRead(struct mtd_info *mtd, __u32 from, __u32 len,
			__u32 *retlen, __u8 *buf)
{
	struct DataFlash *pDataFlash = container_of(flash, struct DataFlash, parent);
	__u32 addr, nPage ,nRemainLen, nReadLen;
	__u8 *pbCmd, *pbBuffer;
	int	status;
	__u32 isBlock;

	DataFlashSpiEnable(pDataFlash->ulChipSelect);

	DPRINT("%s: read 0x%x..0x%x\n", flash->name, from, (from + len));

	*retlen = 0;

	if (0 == len)
		return 0;
	if (from + len > flash->chip_size)
		return -EINVAL;

	nPage = from / pDataFlash->page_size;
	addr  = (nPage << pDataFlash->nPageShift) + (from % pDataFlash->page_size);

	nRemainLen = len;
	pbBuffer   = buf;

	(void) DataFlashWaitReady(flash);

	// spi tx/rx count is 16-bit registers,  so need to split the length, here choose pagesize as basic unit
	while (nRemainLen > 0) {
#if 0
		if (nRemainLen >= pDataFlash->block_size)
			nReadLen = pDataFlash->block_size;
		else if (nRemainLen >= pDataFlash->page_size)
			nReadLen = pDataFlash->page_size;
		else
			nReadLen = nRemainLen;
#else
		if (nRemainLen >= pDataFlash->page_size)
			nReadLen = pDataFlash->page_size;
		else
			nReadLen = nRemainLen;
#endif

		pDataFlash->stOprMsg.pTxCmdBuf  = pDataFlash->stOprMsg.pRxCmdBuf = pbCmd = pDataFlash->bCommand;
		pDataFlash->stOprMsg.nTxCmdLen  = pDataFlash->stOprMsg.nRxCmdLen = 8;
		pDataFlash->stOprMsg.hasData    = true;
		pDataFlash->stOprMsg.pRxDataBuf = pDataFlash->stOprMsg.pTxDataBuf = pbBuffer;
		pDataFlash->stOprMsg.nRxDataLen = pDataFlash->stOprMsg.nTxDataLen = nReadLen;

		pbCmd[0] = OP_READ_CONTINUOUS;
		pbCmd[1] = (addr & 0x00FF0000) >> 16;
		pbCmd[2] = (addr & 0x0000FF00) >> 8;
		pbCmd[3] = (addr & 0x000000FF);

		status = DataFlashSpiOps(flash);
		(void) DataFlashWaitReady(flash);

		if (status >= 0) {
			(*retlen) += nReadLen;
			status = 0;
		} else
			DPRINT("%s: read %x..%x --> %d\n", flash->name, (unsigned)from, (unsigned)(from + len), status);

		if (NULL != flash->callback_func && NULL != flash->callback_args) {
			flash->callback_args->nPageIndex = nPage;
			flash->callback_func(flash, flash->callback_args);
		}

		pbBuffer   += nReadLen;
		nRemainLen -= nReadLen;
		if (pDataFlash->page_size == nReadLen)
			nPage  += 1;
		else if (pDataFlash->block_size == nReadLen)
			nPage  += 8;

		addr = nPage << pDataFlash->nPageShift;

	}

	return status;
}

static int DataFlashReadOOB(struct mtd_info *mtd, __u32 ulLen, struct mtd_oob_ops *pOps)
{
	flash = flash;
	ulLen  = ulLen;
	pOps   = pOps;

	return 0;
}

static int DataFlashWriteOOB(struct mtd_info *mtd, __u32 ulLen, struct mtd_oob_ops *pOps)
{
	flash = flash;
	ulLen  = ulLen;
	pOps   = pOps;

	return 0;
}

static int DataFlashIsBad(struct mtd_info *mtd, __u32 nAddr)
{
	flash = flash;
	nAddr  = nAddr;

	return 0;
}

static int DataFlashMarkBad(struct mtd_info *mtd, __u32 nAddr)
{
	flash = flash;
	nAddr  = nAddr;

	return 0;
}

static int DataFlashAdd(const char *pszName, __u32 nPages, __u32 page_size, __u32 nPageOffset)
{
	struct DataFlash *pDataFlash;
	struct mtd_info	 *flash;
	__u32 ulRet;
	struct part_info *pt_info;
	__u32 nMbrLen;

	pDataFlash = malloc(sizeof(struct DataFlash));
	if (NULL == pDataFlash)
		return -ENOMEM;

	//pDataFlash->page_size   = 1 << (nPageOffset - 1);
	pDataFlash->page_size    = page_size;
	pDataFlash->nPageShift   = nPageOffset;
	pDataFlash->block_size   = pDataFlash->page_size << 3; // FIXME: whether 8 is also correct for other dataflash
	pDataFlash->nBlockShift  = pDataFlash->nPageShift + 3;
	pDataFlash->ulChipSelect = 0;

	flash = &pDataFlash->parent;
	strcpy(pDataFlash->name, pszName);
	strcpy(flash->name, pDataFlash->name);	// FIXME : how to name this flash

	flash->type = MTD_DATAFLASH;

	//flash->chip_size    = nPages * pDataFlash->page_size;
	flash->chip_size    = nPages * page_size;
	flash->erase_size   = pDataFlash->page_size;
	flash->write_size   = pDataFlash->page_size;
	flash->erase_shift  = pDataFlash->nPageShift;
	flash->write_shift  = pDataFlash->nPageShift;
	flash->oob_size     = 0;
//	nPartNum     = 0;

	flash->read         = DataFlashRead;
	flash->write        = DataFlashWrite;
	flash->erase        = DataFlashErase;

	flash->bad_allow   = false;
	flash->read_oob    = DataFlashReadOOB;
	flash->write_oob   = DataFlashWriteOOB;
	flash->block_isbad   = DataFlashIsBad;
	flash->block_markbad = DataFlashMarkBad;

	flash->callback_func  = NULL;
	flash->callback_args  = NULL;

	DPRINT("pagesize=%d\nblocksize=%d\npageshift=%d\nblockshift=%d\nchipsize=%x\n",
			  pDataFlash->page_size, pDataFlash->block_size,
			  pDataFlash->nPageShift, pDataFlash->nBlockShift, flash->chip_size);

	ulRet = flash_register(flash);
	if (ulRet < 0) {
		DPRINT("%s(): fail to register deivce %S!\n", __func__, flash->name);
		// FIXME: destroy

		return ulRet;
	}

	printf("%s dataflash detected!\n", pszName);

	// FIXME: partition support may change
	flash->nMbrAddr = MB(2) / 512 * 528;
	flash->nMbrSize = flash->erase_size;

	{
		nMbrLen  = flash->nMbrSize;

		pt_info = (struct part_info *)malloc(nMbrLen);
		assert(pt_info);

		memset(pt_info, 0, nMbrLen);

		flash->read(flash, flash->nMbrAddr, nMbrLen, &nMbrLen, (__u8 *)pt_info);

		// FIXME: add partition checking
		if (GB_MAGIC_MBR == pt_info->nMagic)
			GuPartCreateAll(flash, pt_info->vParts, pt_info->nParts);
		else {
			printf("Partitions: set to default.\n");
			GuPartCreateAll(flash, g_defDataFlashParts, ARRAY_ELEM_NUM(g_defDataFlashParts));
		}

		free(pt_info);
	}

	return ulRet;
}

void DataFlashSpiInit(void)
{
	volatile struct AT9261_PIO *pPio = (struct AT9261_PIO *)AT91SAM926X_PA_PIOA;
	volatile struct AT9261_PMC *pPmc = (struct AT9261_PMC *)AT91SAM926X_PA_PMC;
	volatile struct AT9261_SPI *pSpi = (struct AT9261_SPI *)AT91SAM926X_PA_SPI0;
	__u32 ulPAEnable, ulPBEnable;

	ulPAEnable = 0x1 << 0 | 0x1 << 1 | 0x1 << 2 | 0x1 << 3 | 0x1 << 4 | 0x1 << 5 | 0x1 << 6;
	ulPBEnable = 0x1 << 27 | 0x1 << 28 | 0x1 << 29;

	pPio->dwASR = ulPAEnable;
	pPio->dwBSR = ulPBEnable;
	pPio->dwPDR = ulPAEnable | ulPBEnable;

	//disable pullup for PA0-PA6
	pPio->dwPPUDR = ~0xFFFFFF80;
	pPio->dwPPUER = 0xFFFFFF80;

	pPmc->dwPCER = 0x1 << 12;

	pSpi->dwSPICR = 0x1 << 7;

	pSpi->dwSPIMR = 0x1 << 0 | 0x1 << 4 | 0xF << 16;

	pSpi->dwSPICSR[0] = 0x1 << 1 | 0x0 << 4 | 0x3 << 8 | (0xFF << 16 & 0x1A << 16) | (0xFF << 24 & 0x1 << 24);

	pSpi->dwSPICR = AT91C_SPI_SPIEN;
	while(!(pSpi->dwSPISR & AT91C_SPI_SPIENS));
}

static int __init DataFlashProbe(void)
{
	struct DataFlash stTmpDataFlash;
	__u8 status;
	__u32 ulRet;

	DataFlashSpiInit();

	DataFlashSpiEnable(0);

	ulRet = DataFlashStatus(&stTmpDataFlash.parent, &status);
	if (ulRet < 0 || status == 0xff) {
		DPRINT("ERROR: status error %d\n", status);
		return -ENODEV;
	}

	switch (status & 0x3c) {
	case 0x0c:
		ulRet = DataFlashAdd("AT45DB011B", 512, 264, 9);
		break;

	case 0x14:
		ulRet = DataFlashAdd("AT45DB021B", 1025, 264, 9); // 1025 ???
		break;

	case 0x1c:
		ulRet = DataFlashAdd("AT45DB041x", 2048, 264, 9);
		break;

	case 0x24:
		ulRet = DataFlashAdd("AT45DB081B", 4096, 264, 9);
		break;

	case 0x2c:
		ulRet = DataFlashAdd("AT45DB161x", 4096, 528, 10);
		break;

	case 0x34:
		ulRet = DataFlashAdd("AT45DB321x", 8192, 528, 10);
		break;

	case 0x38:
	case 0x3c:
		ulRet = DataFlashAdd("AT45DB642x", 8192, 1056, 11);
		break;

	default:
		DPRINT("ERROR: unsupported device (%x)\n", ulRet & 0x3c);
		ulRet = -ENODEV;
	}

	if (ulRet < 0)
		DPRINT("ERROR: DataFlashAdd --> %d\n", ulRet);

	return ulRet;
}

module_init(DataFlashProbe);
