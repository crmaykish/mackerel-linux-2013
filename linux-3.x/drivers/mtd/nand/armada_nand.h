#ifndef __ASM_ARCH_ORION_NFC_H
#define __ASM_ARCH_ORION_NFC_H

#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>

/* arch/arm/mach-armada370/include/mach/armada370.h. See also arch/arm/mach-armada370/sysmap.c */
//#define INTER_REGS_PHYS_BASE           0xD0000000
//#define INTER_REGS_BASE                0xFBB00000    // for LSP
/* see MEM_TABLE, SYSMAP_ARMADA_370 (INTER_REGS_PHYS_BASE:0->INTER_REGS_SIZE) */

// also defined as 0xfec00000 in arch/arm/mach-mvebu/armada-370-xp.h, different from LSP !
#ifndef ARMADA_370_XP_REGS_VIRT_BASE
#define ARMADA_370_XP_REGS_VIRT_BASE 0xfec00000
#endif

#define INTER_REGS_BASE			ARMADA_370_XP_REGS_VIRT_BASE

//#define INTER_REGS_PHYS_BASE		ARMADA_370_XP_REGS_PHYS_BASE

/* arch/arm/mach-armadaxp/armada_xp_family/ctrlEnv/mvCtrlEnvSpec.h */
#define MV_NFC_REGS_OFFSET                    (0xD0000)
/* arch/arm/mach-armadaxp/config/mvSysNfcConfig.h */
#define MV_NFC_REGS_BASE         (MV_NFC_REGS_OFFSET)


/* mvOs.h */
#if 1
#define MV_REG_READ(offset)             \
	(readl_relaxed((void *)(INTER_REGS_BASE | (offset))))

#define MV_REG_WRITE(offset, val)    \
	writel_relaxed((val), (void *)(INTER_REGS_BASE | (offset)))
#endif

/*
#define MV_MEMIO32_WRITE(addr, data)    \
	((*((volatile unsigned int *)(addr))) = ((unsigned int)(data)))

#define MV_MEMIO32_READ(addr)           \
	((*((volatile unsigned int *)(addr))))

#define MV_MEMIO16_WRITE(addr, data)    \
	((*((volatile unsigned short *)(addr))) = ((unsigned short)(data)))

#define MV_MEMIO16_READ(addr)           \
	((*((volatile unsigned short *)(addr))))

#define MV_MEMIO8_WRITE(addr, data)     \
	((*((volatile unsigned char *)(addr))) = ((unsigned char)(data)))

#define MV_MEMIO8_READ(addr)            \
	((*((volatile unsigned char *)(addr))))

#define MV_REG_BIT_SET(offset, bitMask)                 \
	(MV_MEMIO32_WRITE((INTER_REGS_BASE | (offset)), \
	(MV_MEMIO32_READ(INTER_REGS_BASE | (offset)) | \
	MV_32BIT_LE_FAST(bitMask))))
*/

#define MV_REG_BIT_SET(offset, bitmask)                 \
	writel_relaxed(readl_relaxed((void *)(INTER_REGS_BASE | (offset))) | (bitmask), \
	               (void *)(INTER_REGS_BASE | (offset)))

#define MV_REG_BIT_RESET(offset, bitmask)                 \
	writel_relaxed(readl_relaxed((void *)(INTER_REGS_BASE | (offset))) & ~(bitmask), \
	               (void *)(INTER_REGS_BASE | (offset)))

/* arch/arm/mach-armadaxp/include/mach/irqs.h */
#define IRQ_AURORA_NFC          113

/****** from mvNfc.h ***********************************/

/********************************/
/* Enums and structures 	*/
/********************************/

/* Maximum Chain length */
#define MV_NFC_MAX_DESC_CHAIN		0x800

/* Supported page sizes */
#define MV_NFC_512B_PAGE		512
#define MV_NFC_2KB_PAGE			2048
#define MV_NFC_4KB_PAGE			4096
#define MV_NFC_8KB_PAGE			8192

#define MV_NFC_MAX_CHUNK_SIZE		(2048)

/* Nand controller status bits.		*/
#define MV_NFC_STATUS_CMD_REQ 		0x1
#define MV_NFC_STATUS_RDD_REQ 		0x2
#define MV_NFC_STATUS_WRD_REQ 		0x4
#define MV_NFC_STATUS_COR_ERROR 	0x8
#define MV_NFC_STATUS_UNC_ERROR 	0x10
#define MV_NFC_STATUS_BBD 		0x20	/* Bad Block Detected */
#define MV_NFC_STATUS_CMDD 		0x80	/* Command Done */
#define MV_NFC_STATUS_PAGED 		0x200	/* Page Done */
#define MV_NFC_STATUS_RDY		0x800	/* Device Ready */

/* Nand controller interrupt bits.	*/
#define MV_NFC_WR_CMD_REQ_INT		0x1
#define MV_NFC_RD_DATA_REQ_INT		0x2
#define MV_NFC_WR_DATA_REQ_INT		0x4
#define MV_NFC_CORR_ERR_INT		0x8
#define MV_NFC_UNCORR_ERR_INT		0x10
#define MV_NFC_CS1_BAD_BLK_DETECT_INT	0x20
#define MV_NFC_CS0_BAD_BLK_DETECT_INT	0x40
#define MV_NFC_CS1_CMD_DONE_INT		0x80
#define MV_NFC_CS0_CMD_DONE_INT		0x100
#define MV_NFC_DEVICE_READY_INT		0x800

/* Max number of buffers chunks for as single read / write operation */
#define MV_NFC_RW_MAX_BUFF_NUM		16

/* ECC mode options.                    */
typedef enum {
	MV_NFC_ECC_HAMMING = 0,         /* 1 bit */
	MV_NFC_ECC_BCH_2K,              /* 4 bit */
	MV_NFC_ECC_BCH_1K,              /* 8 bit */
	MV_NFC_ECC_BCH_704B,            /* 12 bit */
	MV_NFC_ECC_BCH_512B,            /* 16 bit */
	MV_NFC_ECC_DISABLE,
	MV_NFC_ECC_MAX_CNT
} MV_NFC_ECC_MODE;

typedef enum {
	MV_NFC_PIO_ACCESS,
	MV_NFC_PDMA_ACCESS
} MV_NFC_IO_MODE;

typedef enum {
	MV_NFC_PIO_READ,
	MV_NFC_PIO_WRITE,
	MV_NFC_PIO_NONE
} MV_NFC_PIO_RW_MODE;

typedef enum {
	MV_NFC_IF_1X8,
	MV_NFC_IF_1X16,
	MV_NFC_IF_2X8
} MV_NFC_IF_MODE;

/* Flash device CS.			*/
typedef enum {
	MV_NFC_CS_0,
	MV_NFC_CS_1,
	MV_NFC_CS_2,
	MV_NFC_CS_3,
	MV_NFC_CS_NONE
} MV_NFC_CHIP_SEL;

/*
 * 	ioMode		The access mode by which the unit will operate (PDMA / PIO).
 * 	eccMode		The ECC mode to configure the controller to.
 * 	ifMode		The NAND chip connection mode, 8-bit / 16-bit / gang mode.
 * 	autoStatusRead	Whether to automatically read the flash status after each
 *			erase / write commands.
 *	tclk		System TCLK.
 * 	readyBypass	Whether to wait for the RnB sugnal to be deasserted after
 *			waiting the tR or skip it and move directly to the next step.
 * 	osHandle	OS specific handle used for allocating command buffer
 *	regsPhysAddr	Physical address of internal registers (used in DMA
 *			mode only)
 *	dataPdmaIntMask Interrupt mask for PDMA data channel (used in DMA mode
 *			only).
 *	cmdPdmaIntMask	Interrupt mask for PDMA command channel (used in DMA
 *			mode only).
 */
typedef struct {
	MV_NFC_IO_MODE 		ioMode;
	MV_NFC_ECC_MODE 	eccMode;
	MV_NFC_IF_MODE 		ifMode;
	int 		autoStatusRead;
	uint32_t		tclk;
	int			readyBypass;
	void			*osHandle;
	uint32_t			regsPhysAddr;
#ifdef MV_INCLUDE_PDMA
	uint32_t			dataPdmaIntMask;
	uint32_t			cmdPdmaIntMask;
#endif
} MV_NFC_INFO;

typedef enum {
	MV_NFC_CMD_READ_ID = 0,
	MV_NFC_CMD_READ_STATUS,
	MV_NFC_CMD_ERASE,
	MV_NFC_CMD_MULTIPLANE_ERASE,
	MV_NFC_CMD_RESET,

	MV_NFC_CMD_CACHE_READ_SEQ,
	MV_NFC_CMD_CACHE_READ_RAND,
	MV_NFC_CMD_EXIT_CACHE_READ,
	MV_NFC_CMD_CACHE_READ_START,
	MV_NFC_CMD_READ_MONOLITHIC,
	MV_NFC_CMD_READ_MULTIPLE,
	MV_NFC_CMD_READ_NAKED,
	MV_NFC_CMD_READ_LAST_NAKED,
	MV_NFC_CMD_READ_DISPATCH,

	MV_NFC_CMD_WRITE_MONOLITHIC,
	MV_NFC_CMD_WRITE_MULTIPLE,
	MV_NFC_CMD_WRITE_NAKED,
	MV_NFC_CMD_WRITE_LAST_NAKED,
	MV_NFC_CMD_WRITE_DISPATCH,
	MV_NFC_CMD_WRITE_DISPATCH_START,
	MV_NFC_CMD_WRITE_DISPATCH_END,

	MV_NFC_CMD_COUNT	/* This should be the last enum */

} MV_NFC_CMD_TYPE;

/* This structure contains information describing one of buffers
 * (fragments) they are built Ethernet packet.
 */
typedef struct {
	uint8_t *bufVirtPtr;
	unsigned long bufPhysAddr;
	uint32_t bufSize;
	uint32_t dataSize;
	uint32_t memHandle;
	int32_t bufAddrShift;
} MV_BUF_INFO;

/*
 * Nand multi command information structure.
 *	cmd		The command to be issued.
 *	pageAddr	The flash page address to operate on.
 *	pageCount	Number of pages to read / write.
 *	virtAddr	The virtual address of the buffer to copy data to
 *			from (For relevant commands).
 *	physAddr	The physical address of the buffer to copy data to
 *			from (For relevant commands).
 *	The following parameters might only be used when working in Gagned PDMA
 *	and the pageCount must be set to 1.
 *	For ganged mode, the use might need to split the NAND stack read
 *	write buffer into several buffers according to what the HW expects.
 *	e.g. NAND stack expects data in the following format:
 *	---------------------------
 *	| Data (4K) | Spare | ECC |
 *	---------------------------
 *	While NAND controller expects data to be in the following format:
 *	-----------------------------------------------------
 *	| Data (2K) | Spare | ECC | Data (2K) | Spare | ECC |
 *	-----------------------------------------------------
 *	numSgBuffs	Number of buffers to split the HW buffer into
 *			If 1, then buffOffset & buffSize are ignored.
 *	sgBuffAddr	Array holding the address of the buffers into which the
 *			HW data should be split (Or read into).
 *	sgBuffSize	Array holding the size of each sub-buffer, entry "i"
 *			represents the size in bytes of the buffer starting at
 *			offset buffOffset[i].
 */
typedef struct {
	MV_NFC_CMD_TYPE cmd;
	uint32_t		pageAddr;
	uint32_t		pageCount;
	uint32_t 		*virtAddr;
	uint32_t		physAddr;
	uint32_t		numSgBuffs;
	uint32_t		sgBuffAddr[MV_NFC_RW_MAX_BUFF_NUM];
	uint32_t		*sgBuffAddrVirt[MV_NFC_RW_MAX_BUFF_NUM];
	uint32_t		sgBuffSize[MV_NFC_RW_MAX_BUFF_NUM];
	uint32_t		length;
} MV_NFC_MULTI_CMD;

/****** from mvNfcRegs.h ***********************************/

/* NAND Flash Control Register */
#define	NFC_CONTROL_REG			(MV_NFC_REGS_BASE + 0x0)
#define	NFC_CTRL_WRCMDREQ_MASK		(0x1 << 0)
#define NFC_CTRL_RDDREQ_MASK		(0x1 << 1)
#define NFC_CTRL_WRDREQ_MASK		(0x1 << 2)
#define NFC_CTRL_CORRERR_MASK		(0x1 << 3)
#define NFC_CTRL_UNCERR_MASK		(0x1 << 4)
#define NFC_CTRL_CS1_BBD_MASK		(0x1 << 5)
#define NFC_CTRL_CS0_BBD_MASK		(0x1 << 6)
#define NFC_CTRL_CS1_CMDD_MASK		(0x1 << 7)
#define NFC_CTRL_CS0_CMDD_MASK		(0x1 << 8)
#define NFC_CTRL_CS1_PAGED_MASK		(0x1 << 9)
#define NFC_CTRL_CS0_PAGED_MASK		(0x1 << 10)
#define NFC_CTRL_RDY_MASK		(0x1 << 11)
#define NFC_CTRL_ND_ARB_EN_MASK		(0x1 << 12)
#define NFC_CTRL_PG_PER_BLK_OFFS	13
#define NFC_CTRL_PG_PER_BLK_MASK	(0x3 << NFC_CTRL_PG_PER_BLK_OFFS)
#define NFC_CTRL_PG_PER_BLK_32		(0x0 << NFC_CTRL_PG_PER_BLK_OFFS)
#define NFC_CTRL_PG_PER_BLK_64		(0x2 << NFC_CTRL_PG_PER_BLK_OFFS)
#define NFC_CTRL_PG_PER_BLK_128		(0x1 << NFC_CTRL_PG_PER_BLK_OFFS)
#define NFC_CTRL_PG_PER_BLK_256		(0x3 << NFC_CTRL_PG_PER_BLK_OFFS)
#define NFC_CTRL_RA_START_MASK		(0x1 << 15)
#define NFC_CTRL_RD_ID_CNT_OFFS		16
#define NFC_CTRL_RD_ID_CNT_MASK		(0x7 << NFC_CTRL_RD_ID_CNT_OFFS)
#define NFC_CTRL_RD_ID_CNT_SP		(0x2 << NFC_CTRL_RD_ID_CNT_OFFS)
#define NFC_CTRL_RD_ID_CNT_LP		(0x4 << NFC_CTRL_RD_ID_CNT_OFFS)
#define NFC_CTRL_CLR_PG_CNT_MASK	(0x1 << 20)
#define NFC_CTRL_FORCE_CSX_MASK		(0x1 << 21)
#define NFC_CTRL_ND_STOP_MASK		(0x1 << 22)
#define NFC_CTRL_SEQ_DIS_MASK		(0x1 << 23)
#define NFC_CTRL_PAGE_SZ_OFFS		24
#define NFC_CTRL_PAGE_SZ_MASK		(0x3 << NFC_CTRL_PAGE_SZ_OFFS)
#define NFC_CTRL_PAGE_SZ_512B		(0x0 << NFC_CTRL_PAGE_SZ_OFFS)
#define NFC_CTRL_PAGE_SZ_2KB		(0x1 << NFC_CTRL_PAGE_SZ_OFFS)
#define NFC_CTRL_DWIDTH_M_MASK		(0x1 << 26)
#define NFC_CTRL_DWIDTH_C_MASK		(0x1 << 27)
#define NFC_CTRL_ND_RUN_MASK		(0x1 << 28)
#define NFC_CTRL_DMA_EN_MASK		(0x1 << 29)
#define NFC_CTRL_ECC_EN_MASK		(0x1 << 30)
#define NFC_CTRL_SPARE_EN_MASK		(0x1 << 31)

/* NAND Interface Timing Parameter 0 Register */
#define NFC_TIMING_0_REG		(MV_NFC_REGS_BASE + 0x4)
#define NFC_TMNG0_TRP_OFFS		0
#define NFC_TMNG0_TRP_MASK		(0x7 << NFC_TMNG0_TRP_OFFS)
#define NFC_TMNG0_TRH_OFFS		3
#define NFC_TMNG0_TRH_MASK		(0x7 << NFC_TMNG0_TRH_OFFS)
#define NFC_TMNG0_ETRP_OFFS		6
#define NFC_TMNG0_SEL_NRE_EDGE_OFFS	7
#define NFC_TMNG0_TWP_OFFS		8
#define NFC_TMNG0_TWP_MASK		(0x7 << NFC_TMNG0_TWP_OFFS)
#define NFC_TMNG0_TWH_OFFS		11
#define NFC_TMNG0_TWH_MASK		(0x7 << NFC_TMNG0_TWH_OFFS)
#define NFC_TMNG0_TCS_OFFS		16
#define NFC_TMNG0_TCS_MASK		(0x7 << NFC_TMNG0_TCS_OFFS)
#define NFC_TMNG0_TCH_OFFS		19
#define NFC_TMNG0_TCH_MASK		(0x7 << NFC_TMNG0_TCH_OFFS)
#define NFC_TMNG0_RD_CNT_DEL_OFFS	22
#define NFC_TMNG0_RD_CNT_DEL_MASK	(0xF << NFC_TMNG0_RD_CNT_DEL_OFFS)
#define NFC_TMNG0_SEL_CNTR_OFFS		26
#define NFC_TMNG0_TADL_OFFS		27
#define NFC_TMNG0_TADL_MASK		(0x1F << NFC_TMNG0_TADL_OFFS)

/* NAND Interface Timing Parameter 1 Register */
#define NFC_TIMING_1_REG		(MV_NFC_REGS_BASE + 0xC)
#define NFC_TMNG1_TAR_OFFS		0
#define NFC_TMNG1_TAR_MASK		(0xF << NFC_TMNG1_TAR_OFFS)
#define NFC_TMNG1_TWHR_OFFS		4
#define NFC_TMNG1_TWHR_MASK		(0xF << NFC_TMNG1_TWHR_OFFS)
#define NFC_TMNG1_TRHW_OFFS		8
#define NFC_TMNG1_TRHW_MASK		(0x3 << NFC_TMNG1_TRHW_OFFS)
#define NFC_TMNG1_PRESCALE_OFFS		14
#define NFC_TMNG1_WAIT_MODE_OFFS	15
#define NFC_TMNG1_TR_OFFS		16
#define NFC_TMNG1_TR_MASK		(0xFFFF << NFC_TMNG1_TR_OFFS)

/* NAND Controller Status Register - NDSR */
#define NFC_STATUS_REG			(MV_NFC_REGS_BASE + 0x14)
#define NFC_SR_WRCMDREQ_MASK		(0x1 << 0)
#define NFC_SR_RDDREQ_MASK		(0x1 << 1)
#define NFC_SR_WRDREQ_MASK		(0x1 << 2)
#define NFC_SR_CORERR_MASK		(0x1 << 3)
#define NFC_SR_UNCERR_MASK		(0x1 << 4)
#define NFC_SR_CS1_BBD_MASK		(0x1 << 5)
#define NFC_SR_CS0_BBD_MASK		(0x1 << 6)
#define NFC_SR_CS1_CMDD_MASK		(0x1 << 7)
#define NFC_SR_CS0_CMDD_MASK		(0x1 << 8)
#define NFC_SR_CS1_PAGED_MASK		(0x1 << 9)
#define NFC_SR_CS0_PAGED_MASK		(0x1 << 10)
#define NFC_SR_RDY0_MASK		(0x1 << 11)
#define NFC_SR_RDY1_MASK		(0x1 << 12)
#define NFC_SR_ALLIRQ_MASK		(0x1FFF << 0)
#define NFC_SR_TRUSTVIO_MASK		(0x1 << 15)
#define NFC_SR_ERR_CNT_OFFS		16
#define NFC_SR_ERR_CNT_MASK		(0x1F << NFC_SR_ERR_CNT_OFFS)

/* NAND Controller Page Count Register */
#define NFC_PAGE_COUNT_REG		(MV_NFC_REGS_BASE + 0x18)
#define NFC_PCR_PG_CNT_0_OFFS		0
#define NFC_PCR_PG_CNT_0_MASK		(0xFF << NFC_PCR_PG_CNT_0_OFFS)
#define NFC_PCR_PG_CNT_1_OFFS		16
#define NFC_PCR_PG_CNT_1_MASK		(0xFF << NFC_PCR_PG_CNT_1_OFFS)

/* NAND Controller Bad Block 0 Register */
#define NFC_BAD_BLOCK_0_REG		(MV_NFC_REGS_BASE + 0x1C)

/* NAND Controller Bad Block 1 Register */
#define NFC_BAD_BLOCK_1_REG		(MV_NFC_REGS_BASE + 0x20)

/* NAND ECC Controle Register */
#define NFC_ECC_CONTROL_REG		(MV_NFC_REGS_BASE + 0x28)
#define NFC_ECC_BCH_EN_MASK		(0x1 << 0)
#define NFC_ECC_THRESHOLD_OFFS		1
#define NFC_ECC_THRESHOLF_MASK		(0x3F << NFC_ECC_THRESHOLD_OFFS)
#define NFC_ECC_SPARE_OFFS		7
#define NFC_ECC_SPARE_MASK		(0xFF << NFC_ECC_SPARE_OFFS)

/* NAND Busy Length Count */
#define NFC_BUSY_LEN_CNT_REG		(MV_NFC_REGS_BASE + 0x2C)
#define NFC_BUSY_CNT_0_OFFS		0
#define NFC_BUSY_CNT_0_MASK		(0xFFFF << NFC_BUSY_CNT_0_OFFS)
#define NFC_BUSY_CNT_1_OFFS		16
#define NFC_BUSY_CNT_1_MASK		(0xFFFF << NFC_BUSY_CNT_1_OFFS)

/* NAND Mutex Lock */
#define NFC_MUTEX_LOCK_REG		(MV_NFC_REGS_BASE + 0x30)
#define NFC_MUTEX_LOCK_MASK		(0x1 << 0)

/* NAND Partition Command Match */
#define NFC_PART_CMD_MACTH_1_REG	(MV_NFC_REGS_BASE + 0x34)
#define NFC_PART_CMD_MACTH_2_REG	(MV_NFC_REGS_BASE + 0x38)
#define NFC_PART_CMD_MACTH_3_REG	(MV_NFC_REGS_BASE + 0x3C)
#define NFC_CMDMAT_CMD1_MATCH_OFFS	0
#define NFC_CMDMAT_CMD1_MATCH_MASK	(0xFF << NFC_CMDMAT_CMD1_MATCH_OFFS)
#define NFC_CMDMAT_CMD1_ROWADD_MASK	(0x1 << 8)
#define NFC_CMDMAT_CMD1_NKDDIS_MASK	(0x1 << 9)
#define NFC_CMDMAT_CMD2_MATCH_OFFS	10
#define NFC_CMDMAT_CMD2_MATCH_MASK	(0xFF << NFC_CMDMAT_CMD2_MATCH_OFFS)
#define NFC_CMDMAT_CMD2_ROWADD_MASK	(0x1 << 18)
#define NFC_CMDMAT_CMD2_NKDDIS_MASK	(0x1 << 19)
#define NFC_CMDMAT_CMD3_MATCH_OFFS	20
#define NFC_CMDMAT_CMD3_MATCH_MASK	(0xFF << NFC_CMDMAT_CMD3_MATCH_OFFS)
#define NFC_CMDMAT_CMD3_ROWADD_MASK	(0x1 << 28)
#define NFC_CMDMAT_CMD3_NKDDIS_MASK	(0x1 << 29)
#define NFC_CMDMAT_VALID_CNT_OFFS	30
#define NFC_CMDMAT_VALID_CNT_MASK	(0x3 << NFC_CMDMAT_VALID_CNT_OFFS)

/* NAND Controller Data Buffer */
#define NFC_DATA_BUFF_REG_4PDMA		(MV_NFC_REGS_OFFSET + 0x40)
#define NFC_DATA_BUFF_REG		(MV_NFC_REGS_BASE + 0x40)

/* NAND Controller Command Buffer 0 */
#define NFC_COMMAND_BUFF_0_REG_4PDMA	(MV_NFC_REGS_OFFSET + 0x48)
#define NFC_COMMAND_BUFF_0_REG		(MV_NFC_REGS_BASE + 0x48)
#define NFC_CB0_CMD1_OFFS		0
#define NFC_CB0_CMD1_MASK		(0xFF << NFC_CB0_CMD1_OFFS)
#define NFC_CB0_CMD2_OFFS		8
#define NFC_CB0_CMD2_MASK		(0xFF << NFC_CB0_CMD2_OFFS)
#define NFC_CB0_ADDR_CYC_OFFS		16
#define NFC_CB0_ADDR_CYC_MASK		(0x7 << NFC_CB0_ADDR_CYC_OFFS)
#define NFC_CB0_DBC_MASK			(0x1 << 19)
#define NFC_CB0_NEXT_CMD_MASK		(0x1 << 20)
#define NFC_CB0_CMD_TYPE_OFFS		21
#define NFC_CB0_CMD_TYPE_MASK		(0x7 << NFC_CB0_CMD_TYPE_OFFS)
#define NFC_CB0_CMD_TYPE_READ		(0x0 << NFC_CB0_CMD_TYPE_OFFS)
#define NFC_CB0_CMD_TYPE_WRITE		(0x1 << NFC_CB0_CMD_TYPE_OFFS)
#define NFC_CB0_CMD_TYPE_ERASE		(0x2 << NFC_CB0_CMD_TYPE_OFFS)
#define NFC_CB0_CMD_TYPE_READ_ID	(0x3 << NFC_CB0_CMD_TYPE_OFFS)
#define NFC_CB0_CMD_TYPE_STATUS		(0x4 << NFC_CB0_CMD_TYPE_OFFS)
#define NFC_CB0_CMD_TYPE_RESET		(0x5 << NFC_CB0_CMD_TYPE_OFFS)
#define NFC_CB0_CMD_TYPE_NAKED_CMD	(0x6 << NFC_CB0_CMD_TYPE_OFFS)
#define NFC_CB0_CMD_TYPE_NAKED_ADDR	(0x7 << NFC_CB0_CMD_TYPE_OFFS)
#define NFC_CB0_CSEL_MASK		(0x1 << 24)
#define NFC_CB0_AUTO_RS_MASK		(0x1 << 25)
#define NFC_CB0_ST_ROW_EN_MASK		(0x1 << 26)
#define NFC_CB0_RDY_BYP_MASK		(0x1 << 27)
#define NFC_CB0_LEN_OVRD_MASK		(0x1 << 28)
#define NFC_CB0_CMD_XTYPE_OFFS		29
#define NFC_CB0_CMD_XTYPE_MASK		(0x7 << NFC_CB0_CMD_XTYPE_OFFS)
#define NFC_CB0_CMD_XTYPE_MONOLITHIC	(0x0 << NFC_CB0_CMD_XTYPE_OFFS)
#define NFC_CB0_CMD_XTYPE_LAST_NAKED	(0x1 << NFC_CB0_CMD_XTYPE_OFFS)
#define NFC_CB0_CMD_XTYPE_MULTIPLE	(0x4 << NFC_CB0_CMD_XTYPE_OFFS)
#define NFC_CB0_CMD_XTYPE_NAKED		(0x5 << NFC_CB0_CMD_XTYPE_OFFS)
#define NFC_CB0_CMD_XTYPE_DISPATCH	(0x6 << NFC_CB0_CMD_XTYPE_OFFS)

/* NAND Controller Command Buffer 1 */
#define NFC_COMMAND_BUFF_1_REG		(MV_NFC_REGS_BASE + 0x4C)
#define NFC_CB1_ADDR1_OFFS		0
#define NFC_CB1_ADDR1_MASK		(0xFF << NFC_CB1_ADDR1_OFFS)
#define NFC_CB1_ADDR2_OFFS		8
#define NFC_CB1_ADDR2_MASK		(0xFF << NFC_CB1_ADDR2_OFFS)
#define NFC_CB1_ADDR3_OFFS		16
#define NFC_CB1_ADDR3_MASK		(0xFF << NFC_CB1_ADDR3_OFFS)
#define NFC_CB1_ADDR4_OFFS		24
#define NFC_CB1_ADDR4_MASK		(0xFF << NFC_CB1_ADDR4_OFFS)

/* NAND Controller Command Buffer 2 */
#define NFC_COMMAND_BUFF_2_REG		(MV_NFC_REGS_BASE + 0x50)
#define NFC_CB2_ADDR5_OFFS		0
#define NFC_CB2_ADDR5_MASK		(0xFF << NFC_CB2_ADDR5_OFFS)
#define NFC_CB2_CS_2_3_SELECT_MASK	(0x80 << NFC_CB2_ADDR5_OFFS)
#define NFC_CB2_PAGE_CNT_OFFS		8
#define NFC_CB2_PAGE_CNT_MASK		(0xFF << NFC_CB2_PAGE_CNT_OFFS)
#define NFC_CB2_ST_CMD_OFFS		16
#define NFC_CB2_ST_CMD_MASK		(0xFF << NFC_CB2_ST_CMD_OFFS)
#define NFC_CB2_ST_MASK_OFFS		24
#define NFC_CB2_ST_MASK_MASK		(0xFF << NFC_CB2_ST_MASK_OFFS)

/* NAND Controller Command Buffer 3 */
#define NFC_COMMAND_BUFF_3_REG		(MV_NFC_REGS_BASE + 0x54)
#define NFC_CB3_NDLENCNT_OFFS		0
#define NFC_CB3_NDLENCNT_MASK		(0xFFFF << NFC_CB3_NDLENCNT_OFFS)
#define NFC_CB3_ADDR6_OFFS		16
#define NFC_CB3_ADDR6_MASK		(0xFF << NFC_CB3_ADDR6_OFFS)
#define NFC_CB3_ADDR7_OFFS		24
#define NFC_CB3_ADDR7_MASK		(0xFF << NFC_CB3_ADDR7_OFFS)

/* NAND Arbitration Control */
#define NFC_ARB_CONTROL_REG		(MV_NFC_REGS_BASE + 0x5C)
#define NFC_ARB_CNT_OFFS		0
#define NFC_ARB_CNT_MASK		(0xFFFF << NFC_ARB_CNT_OFFS)

/* NAND Partition Table for Chip Select */
#define NFC_PART_TBL_4CS_REG(x)		(MV_NFC_REGS_BASE + (x * 0x4))
#define NFC_PT4CS_BLOCKADD_OFFS		0
#define NFC_PT4CS_BLOCKADD_MASK		(0xFFFFFF << NFC_PT4CS_BLOCKADD_OFFS)
#define NFC_PT4CS_TRUSTED_MASK		(0x1 << 29)
#define NFC_PT4CS_LOCK_MASK		(0x1 << 30)
#define NFC_PT4CS_VALID_MASK		(0x1 << 31)

/* NAND XBAR2AXI Bridge Configuration Register */
#define NFC_XBAR2AXI_BRDG_CFG_REG	(MV_NFC_REGS_BASE + 0x1022C)
#define NFC_XBC_CS_EXPAND_EN_MASK	(0x1 << 2)



/* FIXME: Should be in linux/mtd/bbm.h */
//#ifdef CONFIG_MTD_NAND_ARMADA_MLC_SUPPORT
/* Search the bad block indicators according to Marvell's Naked symantics */
#define NAND_BBT_SCANMVCUSTOM   0x10000000
//#endif

/*
 * Nand information structure.
 * 	flashId 	The ID of the flash information structure representing the timing
 *		    	and physical layout data of the flash device.
 *	cmdsetId  	The ID of the command-set structure holding the access
 *		   	commands for the flash device.
 *      flashWidth 	Flash device interface width in bits.
 * 	autoStatusRead	Whether to automatically read the flash status after each
 *		    	erase / write commands.
 * 	tclk		System TCLK.
 * 	readyBypass	Whether to wait for the RnB signal to be deasserted after
 * 			waiting the tR or skip it and move directly to the next step.
 *      ioMode		Controller access mode (PDMA / PIO).
 *      eccMode		Flash ECC mode (Hamming, BCH, None).
 *      ifMode		Flash interface mode.
 *      currC		The current flash CS currently being accessed.
 *	dataChanHndl	Pointer to the data DMA channel
 *	cmdChanHndl	Pointer to the command DMA Channel
 *	cmdBuff		Command buffer information (used in DMA only)
 *	regsPhysAddr	Physical address of internal registers (used in DMA
 *			mode only)
 *	dataPdmaIntMask Interrupt mask for PDMA data channel (used in DMA mode
 *			only).
 *	cmdPdmaIntMask	Interrupt mask for PDMA command channel (used in DMA
 *			mode only).
 */
typedef struct {
	uint32_t		flashIdx;
	uint32_t		cmdsetIdx;
	uint32_t 		flashWidth;
	uint32_t 		dfcWidth;
	int 	autoStatusRead;
	int		readyBypass;
	MV_NFC_IO_MODE 	ioMode;
	MV_NFC_ECC_MODE eccMode;
	MV_NFC_IF_MODE 	ifMode;
	MV_NFC_CHIP_SEL currCs;
#ifdef MV_INCLUDE_PDMA
	MV_PDMA_CHANNEL dataChanHndl;
	MV_PDMA_CHANNEL cmdChanHndl;
#endif
	MV_BUF_INFO	cmdBuff;
	MV_BUF_INFO	cmdDescBuff;
	MV_BUF_INFO	dataDescBuff;
	uint32_t		regsPhysAddr;
#ifdef MV_INCLUDE_PDMA
	uint32_t		dataPdmaIntMask;
	uint32_t		cmdPdmaIntMask;
#endif
} MV_NFC_CTRL;


enum nfc_page_size
{
	NFC_PAGE_512B = 0,
	NFC_PAGE_2KB,
	NFC_PAGE_4KB,
	NFC_PAGE_8KB,
	NFC_PAGE_16KB,
	NFC_PAGE_SIZE_MAX_CNT
};

struct nfc_platform_data {
	unsigned int		tclk;		/* Clock supplied to NFC */
	unsigned int		nfc_width;	/* Width of NFC 16/8 bits */
	unsigned int		num_devs;	/* Number of NAND devices 
						   (2 for ganged mode).   */
	unsigned int		num_cs;		/* Number of NAND devices 
						   chip-selects.	  */
	unsigned int		use_dma;	/* Enable/Disable DMA 1/0 */
	MV_NFC_ECC_MODE		ecc_type;
	struct mtd_partition *	parts;
	unsigned int		nr_parts;	
};
#endif /* __ASM_ARCH_ORION_NFC_H */
