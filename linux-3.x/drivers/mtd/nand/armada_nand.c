/*
 * armada_nand.c
 *
 * Copyright (c) 2005 Intel Corporation
 * Copyright (c) 2006 Marvell International Ltd.
 *
 * This driver is based on the PXA drivers/mtd/nand/pxa3xx_nand.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


/*
  mappings:

  MV_STATUS => int
  MV_OK => 0
  MV_U32 => uint32_t
  MV_BOOL => int
  mvOsUDelay() => udelay()
  mvOsDelay() => mdelay()
  BITx => (1<<x)

 */






#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#if 0
#include <linux/clk.h>
#endif
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/io.h>
#include <linux/of_address.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <asm/dma.h>

//#include "mvCommon.h"
//#include "mvOs.h"

#include "armada_nand.h"

#define	DRIVER_NAME	"armada-nand"

#define NFC_DPRINT(x) 		//printk x
#define PRINT_LVL		KERN_DEBUG

#define	CHIP_DELAY_TIMEOUT		(20 * HZ/10)
#define NFC_MAX_NUM_OF_DESCR	(33) /* worst case in 8K ganaged */
#define NFC_8BIT1K_ECC_SPARE	(32)

#define NFC_SR_MASK		(0xfff)
#define NFC_SR_BBD_MASK		(NFC_SR_CS0_BBD_MASK | NFC_SR_CS1_BBD_MASK)


char *cmd_text[]= {
	"MV_NFC_CMD_READ_ID",
	"MV_NFC_CMD_READ_STATUS",
	"MV_NFC_CMD_ERASE",
	"MV_NFC_CMD_MULTIPLANE_ERASE",
	"MV_NFC_CMD_RESET",

	"MV_NFC_CMD_CACHE_READ_SEQ",
	"MV_NFC_CMD_CACHE_READ_RAND",
	"MV_NFC_CMD_EXIT_CACHE_READ",
	"MV_NFC_CMD_CACHE_READ_START",
	"MV_NFC_CMD_READ_MONOLITHIC",
	"MV_NFC_CMD_READ_MULTIPLE",
	"MV_NFC_CMD_READ_NAKED",
	"MV_NFC_CMD_READ_LAST_NAKED",
	"MV_NFC_CMD_READ_DISPATCH",

	"MV_NFC_CMD_WRITE_MONOLITHIC",
	"MV_NFC_CMD_WRITE_MULTIPLE",
	"MV_NFC_CMD_WRITE_NAKED",
	"MV_NFC_CMD_WRITE_LAST_NAKED",
	"MV_NFC_CMD_WRITE_DISPATCH",
	"MV_NFC_CMD_WRITE_DISPATCH_START",
	"MV_NFC_CMD_WRITE_DISPATCH_END",

	"MV_NFC_CMD_COUNT"	/* This should be the last enum */

};

uint32_t pg_sz[NFC_PAGE_SIZE_MAX_CNT] = {512, 2048, 4096, 8192, 16384};

/* error code and state */
enum {
	ERR_NONE	= 0,
	ERR_DMABUSERR	= -1,
	ERR_CMD_TO	= -2,
	ERR_DATA_TO	= -3,
	ERR_DBERR	= -4,
	ERR_BBD		= -5,
};

enum {
	STATE_READY	= 0,
	STATE_CMD_HANDLE,
	STATE_DMA_READING,
	STATE_DMA_WRITING,
	STATE_DMA_DONE,
	STATE_PIO_READING,
	STATE_PIO_WRITING,
};

struct orion_nfc_info {
	struct platform_device	 *pdev;
#if 0
	struct clk		*clk;
#endif
	void __iomem		*mmio_base;
	//unsigned int		mmio_phys_base;

	unsigned int 		buf_start;
	unsigned int		buf_count;

	unsigned char		*data_buff;
	dma_addr_t 		data_buff_phys;
	size_t			data_buff_size;

	/* saved column/page_addr during CMD_SEQIN */
	int			seqin_column;
	int			seqin_page_addr;

	/* relate to the command */
	unsigned int		state;
	unsigned int		use_dma;	/* use DMA ? */

	/* flash information */
	unsigned int		tclk;		/* Clock supplied to NFC */
	unsigned int		nfc_width;	/* Width of NFC 16/8 bits */
	unsigned int		num_devs;	/* Number of NAND devices 
						   (2 for ganged mode).   */
	unsigned int		num_cs;		/* Number of NAND devices 
						   chip-selects.	  */
	MV_NFC_ECC_MODE		ecc_type;
	enum nfc_page_size	page_size;
	uint32_t 		page_per_block;	/* Pages per block (PG_PER_BLK) */	
	uint32_t 		flash_width;	/* Width of Flash memory (DWIDTH_M) */
	size_t	 		read_id_bytes;

	size_t			data_size;	/* data size in FIFO */
	size_t			read_size;
	int 			retcode;
	uint32_t		dscr;		/* IRQ events - status */
	struct completion 	cmd_complete;

	int			chained_cmd;
	uint32_t		column;
	uint32_t		page_addr;
	MV_NFC_CMD_TYPE		cmd;
	MV_NFC_CTRL		nfcCtrl;

	/* RW buffer chunks config */
	uint32_t			sgBuffAddr[MV_NFC_RW_MAX_BUFF_NUM];
	uint32_t			sgBuffSize[MV_NFC_RW_MAX_BUFF_NUM];
	uint32_t			sgNumBuffs;

	/* suspend / resume data */
	uint32_t			nfcUnitData[128];
	uint32_t			nfcDataLen;
	uint32_t			pdmaUnitData[128];
	uint32_t			pdmaDataLen;
};

/*
 * ECC Layout
 */

static struct nand_ecclayout ecc_latout_512B_hamming = {
	.eccbytes = 6,
	.eccpos = {8, 9, 10, 11, 12, 13 },
	.oobfree = { {2, 6} }
};

static struct nand_ecclayout ecc_layout_2KB_hamming = {
	.eccbytes = 24,
	.eccpos = {
		40, 41, 42, 43, 44, 45, 46, 47,
		48, 49, 50, 51, 52, 53, 54, 55,
		56, 57, 58, 59, 60, 61, 62, 63},
	.oobfree = { {2, 38} }
};

static struct nand_ecclayout ecc_layout_2KB_bch4bit = {
	.eccbytes = 32,
	.eccpos = {
		32, 33, 34, 35, 36, 37, 38, 39,
		40, 41, 42, 43, 44, 45, 46, 47,
		48, 49, 50, 51, 52, 53, 54, 55,
		56, 57, 58, 59, 60, 61, 62, 63},
	.oobfree = { {2, 30} }
};

static struct nand_ecclayout ecc_layout_4KB_bch4bit = {
	.eccbytes = 64,
	.eccpos = {
		32,  33,  34,  35,  36,  37,  38,  39,
		40,  41,  42,  43,  44,  45,  46,  47,
		48,  49,  50,  51,  52,  53,  54,  55,
		56,  57,  58,  59,  60,  61,  62,  63,
		96,  97,  98,  99,  100, 101, 102, 103,
		104, 105, 106, 107, 108, 109, 110, 111,
		112, 113, 114, 115, 116, 117, 118, 119,
		120, 121, 122, 123, 124, 125, 126, 127},
	/* Bootrom looks in bytes 0 & 5 for bad blocks */
	.oobfree = { {1, 4}, {6, 26}, { 64, 32} }
};

static struct nand_ecclayout ecc_layout_8KB_bch4bit = {
	.eccbytes = 128,
	.eccpos = {
		32,  33,  34,  35,  36,  37,  38,  39,
		40,  41,  42,  43,  44,  45,  46,  47,
		48,  49,  50,  51,  52,  53,  54,  55,
		56,  57,  58,  59,  60,  61,  62,  63,

		96,  97,  98,  99,  100, 101, 102, 103,
		104, 105, 106, 107, 108, 109, 110, 111,
		112, 113, 114, 115, 116, 117, 118, 119,
		120, 121, 122, 123, 124, 125, 126, 127,

		160, 161, 162, 163, 164, 165, 166, 167,
		168, 169, 170, 171, 172, 173, 174, 175,
		176, 177, 178, 179, 180, 181, 182, 183,
		184, 185, 186, 187, 188, 189, 190, 191,

		224, 225, 226, 227, 228, 229, 230, 231,
		232, 233, 234, 235, 236, 237, 238, 239,
		240, 241, 242, 243, 244, 245, 246, 247,
		248, 249, 250, 251, 252, 253, 254, 255},

	/* Bootrom looks in bytes 0 & 5 for bad blocks */
	.oobfree = { {1, 4}, {6, 26}, { 64, 32}, {128, 32}, {192, 32} }
};

static struct nand_ecclayout ecc_layout_4KB_bch8bit = {
	.eccbytes = 64,
	.eccpos = {
		32,  33,  34,  35,  36,  37,  38,  39,
		40,  41,  42,  43,  44,  45,  46,  47,
		48,  49,  50,  51,  52,  53,  54,  55,
		56,  57,  58,  59,  60,  61,  62,  63},
	/* Bootrom looks in bytes 0 & 5 for bad blocks */
	.oobfree = { {1, 4}, {6, 26},  }
};

static struct nand_ecclayout ecc_layout_8KB_bch8bit = {
	.eccbytes = 160,
	.eccpos = {
		128, 129, 130, 131, 132, 133, 134, 135,
		136, 137, 138, 139, 140, 141, 142, 143,
		144, 145, 146, 147, 148, 149, 150, 151,
		152, 153, 154, 155, 156, 157, 158, 159},
	/* Bootrom looks in bytes 0 & 5 for bad blocks */
	.oobfree = { {1, 4}, {6, 122},  }
};

static struct nand_ecclayout ecc_layout_8KB_bch12bit = {
	.eccbytes = 0,
	.eccpos = { },
	/* Bootrom looks in bytes 0 & 5 for bad blocks */
	.oobfree = { {1, 4}, {6, 58}, }
};

static struct nand_ecclayout ecc_layout_16KB_bch12bit = {
	.eccbytes = 0,
	.eccpos = { },
	/* Bootrom looks in bytes 0 & 5 for bad blocks */
	.oobfree = { {1, 4}, {6, 122},  }
};

/*
 * Define bad block scan pattern when scanning a device for factory 
 * marked blocks.
 */
static uint8_t mv_scan_pattern[] = { 0xff, 0xff };

static struct nand_bbt_descr mv_sp_bb = {
	.options = NAND_BBT_SCANMVCUSTOM,
	.offs = 5,
	.len = 1,
	.pattern = mv_scan_pattern
};

static struct nand_bbt_descr mv_lp_bb = {
	.options = NAND_BBT_SCANMVCUSTOM,
	.offs = 0,
	.len = 2,
	.pattern = mv_scan_pattern
};

/*
 * Lookup Tables
 */

struct orion_nfc_naked_info {
	
	struct nand_ecclayout* 	ecc_layout;
	struct nand_bbt_descr*	bb_info;
	uint32_t		bb_bytepos;
	uint32_t		chunk_size;
	uint32_t		chunk_spare;
	uint32_t		chunk_cnt;
	uint32_t		last_chunk_size;
	uint32_t		last_chunk_spare;
};

			                     /* PageSize*/          /* ECc Type */
static struct orion_nfc_naked_info orion_nfc_naked_info_lkup[NFC_PAGE_SIZE_MAX_CNT][MV_NFC_ECC_MAX_CNT] = {
	/* 512B Pages */
	{{    	/* Hamming */
		&ecc_latout_512B_hamming, &mv_sp_bb, 512, 512, 16, 1, 0, 0
	}, { 	/* BCH 4bit */
		NULL, NULL, 0, 0, 0, 0, 0, 0
	}, { 	/* BCH 8bit */
		NULL, NULL, 0, 0, 0, 0, 0, 0
	}, { 	/* BCH 12bit */
		NULL, NULL, 0, 0, 0, 0, 0, 0
	}, { 	/* BCH 16bit */
		NULL, NULL, 0, 0, 0, 0, 0, 0
	}, { 	/* No ECC */
		NULL, NULL, 0, 0, 0, 0, 0, 0
	}},
	/* 2KB Pages */
	{{	/* Hamming */
		&ecc_layout_2KB_hamming, &mv_lp_bb, 2048, 2048, 40, 1, 0, 0
	}, { 	/* BCH 4bit */
		&ecc_layout_2KB_bch4bit, &mv_lp_bb, 2048, 2048, 32, 1, 0, 0
	}, { 	/* BCH 8bit */
		NULL, NULL, 2018, 1024, 0, 1, 1024, 32
	}, { 	/* BCH 12bit */
		NULL, NULL, 1988, 704, 0, 2, 640, 0
	}, { 	/* BCH 16bit */
		NULL, NULL, 1958, 512, 0, 4, 0, 32
	}, { 	/* No ECC */
		NULL, NULL, 0, 0, 0, 0, 0, 0
	}},
	/* 4KB Pages */
	{{	/* Hamming */
		NULL, 0, 0, 0, 0, 0, 0, 0
	}, { 	/* BCH 4bit */
		&ecc_layout_4KB_bch4bit, &mv_lp_bb, 4034, 2048, 32, 2, 0, 0
	}, { 	/* BCH 8bit */
		&ecc_layout_4KB_bch8bit, &mv_lp_bb, 4006, 1024, 0, 4, 0, 64
	}, { 	/* BCH 12bit */
		NULL, NULL, 3946, 704,  0, 5, 576, 32
	}, { 	/* BCH 16bit */
		NULL, NULL, 3886, 512, 0, 8, 0, 32
	}, { 	/* No ECC */
		NULL, NULL, 0, 0, 0, 0, 0, 0
	}},
	/* 8KB Pages */
	{{	/* Hamming */
		NULL, 0, 0, 0, 0, 0, 0, 0
	}, { 	/* BCH 4bit */
		&ecc_layout_8KB_bch4bit, &mv_lp_bb, 8102, 2048, 32, 4, 0, 0
	}, { 	/* BCH 8bit */
		&ecc_layout_8KB_bch8bit, &mv_lp_bb, 7982, 1024, 0, 8, 0, 160
	}, { 	/* BCH 12bit */
		&ecc_layout_8KB_bch12bit, &mv_lp_bb,7862, 704, 0, 11, 448, 64
	}, { 	/* BCH 16bit */
		NULL, NULL, 7742, 512, 0, 16, 0, 32
	}, { 	/* No ECC */
		NULL, NULL, 0, 0, 0, 0, 0, 0
	}},
	/* 16KB Pages */
	{{	/* Hamming */
		NULL, NULL, 0, 0, 0, 0, 0, 0
	}, { 	/* BCH 4bit */
		NULL, NULL, 15914, 2048, 32, 8, 0, 0
	}, { 	/* BCH 8bit */
		NULL, NULL, 15930, 1024, 0, 16, 0, 352
	}, { 	/* BCH 12bit */
		&ecc_layout_16KB_bch12bit, &mv_lp_bb, 15724, 704, 0, 23, 192, 128
	}, { 	/* BCH 16bit */
		NULL, NULL, 15484, 512, 0, 32, 0, 32
	}, { 	/* No ECC */
		NULL, NULL, 0, 0, 0, 0, 0, 0
	}}};
		

#define ECC_LAYOUT	(orion_nfc_naked_info_lkup[info->page_size][info->ecc_type].ecc_layout)
#define BB_INFO		(orion_nfc_naked_info_lkup[info->page_size][info->ecc_type].bb_info)
#define	BB_BYTE_POS	(orion_nfc_naked_info_lkup[info->page_size][info->ecc_type].bb_bytepos)
#define CHUNK_CNT	(orion_nfc_naked_info_lkup[info->page_size][info->ecc_type].chunk_cnt)
#define CHUNK_SZ	(orion_nfc_naked_info_lkup[info->page_size][info->ecc_type].chunk_size)
#define CHUNK_SPR	(orion_nfc_naked_info_lkup[info->page_size][info->ecc_type].chunk_spare)
#define LST_CHUNK_SZ	(orion_nfc_naked_info_lkup[info->page_size][info->ecc_type].last_chunk_size)
#define LST_CHUNK_SPR	(orion_nfc_naked_info_lkup[info->page_size][info->ecc_type].last_chunk_spare)

struct orion_nfc_cmd_info {
	
	uint32_t		events_p1;	/* post command events */
	uint32_t		events_p2;	/* post data events */
	MV_NFC_PIO_RW_MODE	rw;
};

static struct orion_nfc_cmd_info orion_nfc_cmd_info_lkup[MV_NFC_CMD_COUNT] = {
	/* Phase 1 interrupts */			/* Phase 2 interrupts */			/* Read/Write */  /* MV_NFC_CMD_xxxxxx */
	{(NFC_SR_RDDREQ_MASK), 				(0),						MV_NFC_PIO_READ}, /* READ_ID */
	{(NFC_SR_RDDREQ_MASK), 				(0),						MV_NFC_PIO_READ}, /* READ_STATUS */
	{(0), 						(MV_NFC_STATUS_RDY | MV_NFC_STATUS_BBD),	MV_NFC_PIO_NONE}, /* ERASE */
	{(0), 						(0), 						MV_NFC_PIO_NONE}, /* MULTIPLANE_ERASE */
	{(0), 						(MV_NFC_STATUS_RDY), 				MV_NFC_PIO_NONE}, /* RESET */
	{(0), 						(0), 						MV_NFC_PIO_READ}, /* CACHE_READ_SEQ */
	{(0), 						(0), 						MV_NFC_PIO_READ}, /* CACHE_READ_RAND */
	{(0), 						(0), 						MV_NFC_PIO_NONE}, /* EXIT_CACHE_READ */
	{(0), 						(0), 						MV_NFC_PIO_READ}, /* CACHE_READ_START */
	{(NFC_SR_RDDREQ_MASK | NFC_SR_UNCERR_MASK), 	(0), 						MV_NFC_PIO_READ}, /* READ_MONOLITHIC */
	{(0), 						(0),						MV_NFC_PIO_READ}, /* READ_MULTIPLE */
	{(NFC_SR_RDDREQ_MASK | NFC_SR_UNCERR_MASK), 	(0), 						MV_NFC_PIO_READ}, /* READ_NAKED */
	{(NFC_SR_RDDREQ_MASK | NFC_SR_UNCERR_MASK), 	(0), 						MV_NFC_PIO_READ}, /* READ_LAST_NAKED */
	{(0), 						(0), 						MV_NFC_PIO_NONE}, /* READ_DISPATCH */
	{(MV_NFC_STATUS_WRD_REQ), 			(MV_NFC_STATUS_RDY | MV_NFC_STATUS_BBD),	MV_NFC_PIO_WRITE},/* WRITE_MONOLITHIC */
	{(0), 						(0), 						MV_NFC_PIO_WRITE},/* WRITE_MULTIPLE */
	{(MV_NFC_STATUS_WRD_REQ),			(MV_NFC_STATUS_PAGED),				MV_NFC_PIO_WRITE},/* WRITE_NAKED */
	{(0), 						(0), 						MV_NFC_PIO_WRITE},/* WRITE_LAST_NAKED */
	{(0), 						(0), 						MV_NFC_PIO_NONE}, /* WRITE_DISPATCH */
	{(MV_NFC_STATUS_CMDD),				(0),						MV_NFC_PIO_NONE}, /* WRITE_DISPATCH_START */
	{(0),						(MV_NFC_STATUS_RDY | MV_NFC_STATUS_BBD), 	MV_NFC_PIO_NONE}, /* WRITE_DISPATCH_END */
};


/****** arch/arm/plat-armada/common/mvTypes.h ************/

#define MV_STATUS int
#define MV_U32 uint32_t
#define MV_BOOL int
#define MV_VOID void

#define MV_8 int8_t
#define MV_U8 uint8_t

#define MV_32 int32_t
#define MV_U32 uint32_t

#define MV_16 int16_t
#define MV_U16 uint16_t

/* The following is a list of Marvell status    */
#define MV_ERROR		    (-1)
#define MV_OK			    (0)	/* Operation succeeded                   */
#define MV_FAIL			    (1)	/* Operation failed                      */
#define MV_BAD_VALUE        (2)	/* Illegal value (general)               */
#define MV_OUT_OF_RANGE     (3)	/* The value is out of range             */
#define MV_BAD_PARAM        (4)	/* Illegal parameter in function called  */
#define MV_BAD_PTR          (5)	/* Illegal pointer value                 */
#define MV_BAD_SIZE         (6)	/* Illegal size                          */
#define MV_BAD_STATE        (7)	/* Illegal state of state machine        */
#define MV_SET_ERROR        (8)	/* Set operation failed                  */
#define MV_GET_ERROR        (9)	/* Get operation failed                  */
#define MV_CREATE_ERROR     (10)	/* Fail while creating an item           */
#define MV_NOT_FOUND        (11)	/* Item not found                        */
#define MV_NO_MORE          (12)	/* No more items found                   */
#define MV_NO_SUCH          (13)	/* No such item                          */
#define MV_TIMEOUT          (14)	/* Time Out                              */
#define MV_NO_CHANGE        (15)	/* Parameter(s) is already in this value */
#define MV_NOT_SUPPORTED    (16)	/* This request is not support           */
#define MV_NOT_IMPLEMENTED  (17)	/* Request supported but not implemented */
#define MV_NOT_INITIALIZED  (18)	/* The item is not initialized           */
#define MV_NO_RESOURCE      (19)	/* Resource not available (memory ...)   */
#define MV_FULL             (20)	/* Item is full (Queue or table etc...)  */
#define MV_EMPTY            (21)	/* Item is empty (Queue or table etc...) */
#define MV_INIT_ERROR       (22)	/* Error occured while INIT process      */
#define MV_HW_ERROR         (23)	/* Hardware error                        */
#define MV_TX_ERROR         (24)	/* Transmit operation not succeeded      */
#define MV_RX_ERROR         (25)	/* Recieve operation not succeeded       */
#define MV_NOT_READY	    (26)	/* The other side is not ready yet       */
#define MV_ALREADY_EXIST    (27)	/* Tried to create existing item         */
#define MV_OUT_OF_CPU_MEM   (28)	/* Cpu memory allocation failed.         */
#define MV_NOT_STARTED      (29)	/* Not started yet                       */
#define MV_BUSY             (30)	/* Item is busy.                         */
#define MV_TERMINATE        (31)	/* Item terminates it's work.            */
#define MV_NOT_ALIGNED      (32)	/* Wrong alignment                       */
#define MV_NOT_ALLOWED      (33)	/* Operation NOT allowed                 */
#define MV_WRITE_PROTECT    (34)	/* Write protected                       */
#define MV_DROPPED          (35)	/* Packet dropped                        */
#define MV_STOLEN           (36)	/* Packet stolen */
#define MV_CONTINUE         (37)        /* Continue */
#define MV_RETRY		    (38)	/* Operation failed need retry           */

#define MV_INVALID  (int)(-1)

#define MV_FALSE	0
#define MV_TRUE     (!(MV_FALSE))

/****** arch/arm/plat-armada/mv_hal/nfc/mvNfc.c ************/

#define NFC_NATIVE_READ_ID_CMD		0x0090
#define NFC_READ_ID_ADDR_LEN		1
#define NFC_ERASE_ADDR_LEN		3
#define NFC_SP_READ_ADDR_LEN		3
#define NFC_SP_BIG_READ_ADDR_LEN	4
#define NFC_LP_READ_ADDR_LEN		5
#define NFC_BLOCK_ADDR_BITS		0xFFFFFF
#define NFC_SP_COL_OFFS			0
#define NFC_SP_COL_MASK			(0xFF << NFC_SP_COL_OFFS)
#define NFC_LP_COL_OFFS			0
#define NFC_LP_COL_MASK			(0xFFFF << NFC_SP_COL_OFFS)
#define NFC_SP_PG_OFFS			8
#define NFC_SP_PG_MASK			(0xFFFFFF << NFC_SP_PG_OFFS)
#define NFC_LP_PG_OFFS			16
#define NFC_LP_PG_MASK			(0xFFFF << NFC_LP_PG_OFFS)
#define NFC_PG_CNT_OFFS			8
#define NFC_PG_CNT_MASK			(0xFF << NFC_PG_CNT_OFFS)

/* NAND special features bitmask definition.	*/
#define NFC_FLAGS_NONE			0x0
#define NFC_FLAGS_ONFI_MODE_3_SET	0x1
#define NFC_CLOCK_UPSCALE_200M		0x2

/* End of NAND special features definitions.	*/

#define NFC_READ_ID_PDMA_DATA_LEN	32
#define NFC_READ_STATUS_PDMA_DATA_LEN	32
#define NFC_READ_ID_PIO_DATA_LEN	8
#define NFC_READ_STATUS_PIO_DATA_LEN	8
#define NFC_RW_SP_PDMA_DATA_LEN		544
#define NFC_RW_SP_NO_ECC_DATA_LEN	528
#define NFC_RW_SP_HMNG_ECC_DATA_LEN	520
#define NFC_RW_SP_G_NO_ECC_DATA_LEN	528
#define NFC_RW_SP_G_HMNG_ECC_DATA_LEN	526

#define NFC_RW_LP_PDMA_DATA_LEN		2112

#define NFC_RW_LP_NO_ECC_DATA_LEN	2112
#define NFC_RW_LP_HMNG_ECC_DATA_LEN	2088
#define NFC_RW_LP_BCH_ECC_DATA_LEN	2080

#define NFC_RW_LP_G_NO_ECC_DATA_LEN	2112
#define NFC_RW_LP_G_HMNG_ECC_DATA_LEN	2088
#define NFC_RW_LP_G_BCH_ECC_DATA_LEN	2080

#define NFC_RW_LP_BCH1K_ECC_DATA_LEN	1024
#define NFC_RW_LP_BCH704B_ECC_DATA_LEN	704
#define NFC_RW_LP_BCH512B_ECC_DATA_LEN	512

#define NFC_CMD_STRUCT_SIZE		(sizeof(MV_NFC_CMD))
#define NFC_CMD_BUFF_SIZE(cmdb_0)	((cmdb_0 & NFC_CB0_LEN_OVRD_MASK) ? 16 : 12)
#define NFC_CMD_BUFF_ADDR		(NFC_COMMAND_BUFF_0_REG_4PDMA)
#define NFC_DATA_BUFF_ADDR		(NFC_DATA_BUFF_REG_4PDMA)

/**********/
/* Macros */
/**********/
#define ns_clk(ns, ns2clk)	((ns % ns2clk) ? (MV_U32)((ns/ns2clk)+1) : (MV_U32)(ns/ns2clk))
#define maxx(a, b)		((a > b) ? a : b)
#define check_limit(val, pwr)	((val > ((1 << pwr)-1)) ? ((1 << pwr)-1) : val)

/***********/
/* Typedef */
/***********/

/* Flash Timing Parameters */
typedef struct {
	/* Flash Timing */
	MV_U32 tADL;		/* Address to write data delay */
	MV_U32 tCH;		/* Enable signal hold time */
	MV_U32 tCS;		/* Enable signal setup time */
	MV_U32 tWC;		/* ND_nWS cycle duration */
	MV_U32 tWH;		/* ND_nWE high duration */
	MV_U32 tWP;		/* ND_nWE pulse time */
	MV_U32 tRC;		/* ND_nRE cycle duration */
	MV_U32 tRH;		/* ND_nRE high duration */
	MV_U32 tRP;		/* ND_nRE pulse width */
	MV_U32 tR;		/* ND_nWE high to ND_nRE low for read */
	MV_U32 tWHR;		/* ND_nWE high to ND_nRE low for status read */
	MV_U32 tAR;		/* ND_ALE low to ND_nRE low delay */
	MV_U32 tRHW;		/* ND_nRE high to ND_nWE low delay */
	/* Physical Layout */
	MV_U32 pgPrBlk;		/* Pages per block */
	MV_U32 pgSz;		/* Page size */
	MV_U32 oobSz;		/* Page size */
	MV_U32 blkNum;		/* Number of blocks per device */
	MV_U32 id;		/* Manufacturer and device IDs */
	MV_U32 seqDis;		/* Enable/Disable sequential multipage read */
	MV_8 *model;		/* Flash Model string */
	MV_U32 bb_page;		/* Page containing bad block marking */
	MV_U32 flags;		/* Special features configuration.	*/
} MV_NFC_FLASH_INFO;

/* Flash command set */
typedef struct {
	MV_U16 read1;
	MV_U16 exitCacheRead;
	MV_U16 cacheReadRand;
	MV_U16 cacheReadSeq;
	MV_U16 read2;
	MV_U16 program;
	MV_U16 readStatus;
	MV_U16 readId;
	MV_U16 erase;
	MV_U16 multiplaneErase;
	MV_U16 reset;
	MV_U16 lock;
	MV_U16 unlock;
	MV_U16 lockStatus;
} MV_NFC_FLASH_CMD_SET;

/* ONFI Mode type */
typedef enum {
	MV_NFC_ONFI_MODE_0,
	MV_NFC_ONFI_MODE_1,
	MV_NFC_ONFI_MODE_2,
	MV_NFC_ONFI_MODE_3,
	MV_NFC_ONFI_MODE_4,
	MV_NFC_ONFI_MODE_5
} MV_NFC_ONFI_MODE;

/********/
/* Data */
/********/

/* Defined Flash Types */
MV_NFC_FLASH_INFO flashDeviceInfo[] = {
	{			/* ST 1Gb */
	.tADL = 100,		/* tADL, Address to write data delay */
	.tCH = 5,		/* tCH, Enable signal hold time */
	.tCS = 20,		/* tCS, Enable signal setup time */
	.tWC = 30,		/* tWC, ND_nWE cycle duration */
	.tWH = 10,		/* tWH, ND_nWE high duration */
	.tWP = 15,		/* tWP, ND_nWE pulse time */
	.tRC = 25,		/* tWC, ND_nRE cycle duration */
	.tRH = 10,		/* tRH, ND_nRE high duration */
	.tRP = 15,		/* tRP, ND_nRE pulse width */
	.tR = 25000, 		/* tR = tR+tRR+tWB+1, ND_nWE high to ND_nRE low for read - 25000+20+100+1 */
	.tWHR = 60,		/* tWHR, ND_nWE high to ND_nRE low delay for status read */
	.tAR = 10,		/* tAR, ND_ALE low to ND_nRE low delay */
	.tRHW = 30,		/* tRHW, ND_nRE high to ND_nWE low delay */
	.pgPrBlk = 64,		/* Pages per block - detected */
	.pgSz = 2048,		/* Page size */
	.oobSz = 64,		/* Spare size */
	.blkNum = 1024,		/* Number of blocks/sectors in the flash */
	.id = 0xF120,		/* Device ID 0xDevice,Vendor */
	.model = "ST 1Gb 8bit",
	.bb_page = 63,		/* Manufacturer Bad block marking page in block */
	.flags = NFC_CLOCK_UPSCALE_200M
	},

	{			/* ST 8Gb */
	 .tADL = 0,		/* tADL, Address to write data delay */
	 .tCH = 5,		/* tCH, Enable signal hold time */
	 .tCS = 20,		/* tCS, Enable signal setup time */
	 .tWC = 24,		/* tWC, ND_nWE cycle duration */
	 .tWH = 12,		/* tWH, ND_nWE high duration */
	 .tWP = 12,		/* tWP, ND_nWE pulse time */
	 .tRC = 24,		/* tWC, ND_nRE cycle duration */
	 .tRH = 12,		/* tRH, ND_nRE high duration */
	 .tRP = 12,		/* tRP, ND_nRE pulse width */
	 .tR = 25121,		/* tR = tR+tRR+tWB+1, ND_nWE high to ND_nRE low for read - 25000+20+100+1 */
	 .tWHR = 60,		/* tWHR, ND_nWE high to ND_nRE low delay for status read */
	 .tAR = 10,		/* tAR, ND_ALE low to ND_nRE low delay */
	 .tRHW = 48,		/* tRHW, ND_nRE high to ND_nWE low delay */
	 .pgPrBlk = 64,		/* Pages per block - detected */
	 .pgSz = 2048,		/* Page size */
	 .oobSz = 64,		/* Spare size */
	 .blkNum = 2048,	/* Number of blocks/sectors in the flash */
	 .id = 0xD320,		/* Device ID 0xDevice,Vendor */
	 .model = "ST 8Gb 8bit",
	 .bb_page = 63,		/* Manufacturer Bad block marking page in block */
	 .flags = NFC_CLOCK_UPSCALE_200M
	 },
	{			/* ST 4Gb */
	 .tADL = 70,		/* tADL, Address to write data delay */
	 .tCH = 5,		/* tCH, Enable signal hold time */
	 .tCS = 20,		/* tCS, Enable signal setup time */
	 .tWC = 22,		/* tWC, ND_nWE cycle duration */
	 .tWH = 10,		/* tWH, ND_nWE high duration */
	 .tWP = 12,		/* tWP, ND_nWE pulse time */
	 .tRC = 24,		/* tWC, ND_nRE cycle duration */
	 .tRH = 12,		/* tRH, ND_nRE high duration */
	 .tRP = 12,		/* tRP, ND_nRE pulse width */
	 .tR = 25121,		/* tR = tR+tRR+tWB+1, ND_nWE high to ND_nRE low for read - 25000+20+100+1 */
	 .tWHR = 60,		/* tWHR, ND_nWE high to ND_nRE low delay for status read */
	 .tAR = 10,		/* tAR, ND_ALE low to ND_nRE low delay */
	 .tRHW = 100,		/* tRHW, ND_nRE high to ND_nWE low delay */
	 .pgPrBlk = 64,		/* Pages per block - detected */
	 .pgSz = 2048,		/* Page size */
	 .oobSz = 64,		/* Spare size */
	 .blkNum = 2048,	/* Number of blocks/sectors in the flash */
	 .id = 0xDC20,		/* Device ID 0xDevice,Vendor */
	 .model = "NM 4Gb 8bit",
	 .bb_page = 0,		/* Manufacturer Bad block marking page in block */
	 .flags = NFC_CLOCK_UPSCALE_200M
	 },
	{			/* ST 32Gb */
	 .tADL = 0,		/* tADL, Address to write data delay */
	 .tCH = 5,		/* tCH, Enable signal hold time */
	 .tCS = 20,		/* tCS, Enable signal setup time */
	 .tWC = 22,		/* tWC, ND_nWE cycle duration */
	 .tWH = 10,		/* tWH, ND_nWE high duration */
	 .tWP = 12,		/* tWP, ND_nWE pulse time */
	 .tRC = 22,		/* tWC, ND_nRE cycle duration */
	 .tRH = 10,		/* tRH, ND_nRE high duration */
	 .tRP = 12,		/* tRP, ND_nRE pulse width */
	 .tR = 25121,		/* tR = tR+tRR+tWB+1, ND_nWE high to ND_nRE low for read - 25000+20+100+1 */
	 .tWHR = 80,		/* tWHR, ND_nWE high to ND_nRE low delay for status read */
	 .tAR = 10,		/* tAR, ND_ALE low to ND_nRE low delay */
	 .tRHW = 48,		/* tRHW, ND_nRE high to ND_nWE low delay */
	 .pgPrBlk = 64,		/* Pages per block - detected */
	 .pgSz = 4096,		/* Page size */
	 .oobSz = 128,		/* Spare size */
	 .blkNum = 16384,	/* Number of blocks/sectors in the flash */
	 .id = 0xD520,		/* Device ID 0xVendor,device */
	 .model = "ST 32Gb 8bit",
	 .bb_page = 63,		/* Manufacturer Bad block marking page in block */
	 .flags = NFC_CLOCK_UPSCALE_200M
	 },

	{			/* Samsung 16Gb */
	 .tADL = 90,		/* tADL, Address to write data delay */
	 .tCH = 0,		/* tCH, Enable signal hold time */
	 .tCS = 5,		/* tCS, Enable signal setup time */
	 .tWC = 22,		/* tWC, ND_nWE cycle duration */
	 .tWH = 10,		/* tWH, ND_nWE high duration */
	 .tWP = 12,		/* tWP, ND_nWE pulse time */
	 .tRC = 24,		/* tWC, ND_nRE cycle duration */
	 .tRH = 12,		/* tRH, ND_nRE high duration */
	 .tRP = 12,		/* tRP, ND_nRE pulse width */
	 .tR = 49146,		/* tR = data transfer from cell to register, maximum 60,000ns */
	 .tWHR = 66,		/* tWHR, ND_nWE high to ND_nRE low delay for status read */
	 .tAR = 66,		/* tAR, ND_ALE low to ND_nRE low delay */
	 .tRHW = 32,		/* tRHW, ND_nRE high to ND_nWE low delay 32 clocks */
	 .pgPrBlk = 128,	/* Pages per block - detected */
	 .pgSz = 2048,		/* Page size */
	 .oobSz = 64,		/* Spare size */
	 .blkNum = 8192,	/* Number of blocks/sectors in the flash */
	 .id = 0xD5EC,		/* Device ID 0xDevice,Vendor */
	 .model = "Samsung 16Gb 8bit",
	 .bb_page = 127,	/* Manufacturer Bad block marking page in block */
	 .flags = NFC_CLOCK_UPSCALE_200M
	 },

	{			/* Samsung 2Gb */
	.tADL = 90,		/* tADL, Address to write data delay */
	.tCH = 10,		/* tCH, Enable signal hold time */
	.tCS = 0,		/* tCS, Enable signal setup time */
	.tWC = 40,		/* tWC, ND_nWE cycle duration */
	.tWH = 15,		/* tWH, ND_nWE high duration */
	.tWP = 25,		/* tWP, ND_nWE pulse time */
	.tRC = 40,		/* tWC, ND_nRE cycle duration */
	.tRH = 15,		/* tRH, ND_nRE high duration */
	.tRP = 25,		/* tRP, ND_nRE pulse width */
	.tR = 25000, 		/* tR = data transfer from cell to register, maximum 60,000ns */
	.tWHR = 60,		/* tWHR, ND_nWE high to ND_nRE low delay for status read */
	.tAR = 10,		/* tAR, ND_ALE low to ND_nRE low delay */
	.tRHW = 30,		/* tRHW, ND_nRE high to ND_nWE low delay 32 clocks */
	.pgPrBlk = 128,		/* Pages per block - detected */
	.pgSz = 2048,		/* Page size */
	.oobSz = 64,		/* Spare size */
	.blkNum = 1024,		/* Number of blocks/sectors in the flash */
	.id = 0xDAEC,		/* Device ID 0xDevice,Vendor */ /* 0x9AA8 when run through JTAG */
	.model = "Samsung 2Gb 8bit",
	.bb_page = 0,		/* Manufacturer Bad block marking page in block */
	 .flags = NFC_CLOCK_UPSCALE_200M
	},

 	{			/* Samsung 8Gb */
	.tADL = 100,		/* tADL, Address to write data delay */
	.tCH = 5,		/* tCH, Enable signal hold time */
	.tCS = 20,		/* tCS, Enable signal setup time */
	.tWC = 22,		/* tWC, ND_nWE cycle duration */
	.tWH = 10,		/* tWH, ND_nWE high duration */
	.tWP = 12,		/* tWP, ND_nWE pulse time */
	.tRC = 22,		/* tWC, ND_nRE cycle duration */
	.tRH = 10,		/* tRH, ND_nRE high duration */
	.tRP = 12,		/* tRP, ND_nRE pulse width */
	.tR = 25000, 		/* tR = data transfer from cell to register, maximum 60,000ns */
	.tWHR = 60,		/* tWHR, ND_nWE high to ND_nRE low delay for status read */
	.tAR = 10,		/* tAR, ND_ALE low to ND_nRE low delay */
	.tRHW = 100,		/* tRHW, ND_nRE high to ND_nWE low delay 32 clocks */
	.pgPrBlk = 64,		/* Pages per block - detected */
	.pgSz = 4096,		/* Page size */
	.oobSz = 128,		/* Spare size */
	.blkNum = 4096,		/* Number of blocks/sectors in the flash */
	.id = 0xD3EC,		/* Device ID 0xDevice,Vendor */ /* 0x9AA8 when run through JTAG */
	.model = "Samsung 8Gb 8bit",
	.bb_page = 0,		/* Manufacturer Bad block marking page in block */
	 .flags = NFC_CLOCK_UPSCALE_200M
	},

 	{			/* Samsung 4Gb */
	.tADL = 70,		/* tADL, Address to write data delay */
	.tCH = 5,		/* tCH, Enable signal hold time */
	.tCS = 20,		/* tCS, Enable signal setup time */
	.tWC = 22,		/* tWC, ND_nWE cycle duration */
	.tWH = 10,		/* tWH, ND_nWE high duration */
	.tWP = 12,		/* tWP, ND_nWE pulse time */
	.tRC = 22,		/* tWC, ND_nRE cycle duration */
	.tRH = 10,		/* tRH, ND_nRE high duration */
	.tRP = 12,		/* tRP, ND_nRE pulse width */
	.tR = 25000, 		/* tR = data transfer from cell to register, maximum 60,000ns */
	.tWHR = 60,		/* tWHR, ND_nWE high to ND_nRE low delay for status read */
	.tAR = 10,		/* tAR, ND_ALE low to ND_nRE low delay */
	.tRHW = 100,		/* tRHW, ND_nRE high to ND_nWE low delay 32 clocks */
	.pgPrBlk = 64,		/* Pages per block - detected */
	.pgSz = 2048,		/* Page size */
	.oobSz = 64,		/* Spare size */
	.blkNum = 2048,		/* Number of blocks/sectors in the flash */
	.id = 0xDCEC,		/* Device ID 0xDevice,Vendor */ /* 0x9AA8 when run through JTAG */
	.model = "Samsung 4Gb 8bit",
	.bb_page = 0,		/* Manufacturer Bad block marking page in block */
	.flags = NFC_CLOCK_UPSCALE_200M
	},

	{			/* Samsung 32Gb */
	 .tADL = 0,		/* tADL, Address to write data delay */
	 .tCH = 5,		/* tCH, Enable signal hold time */
	 .tCS = 20,		/* tCS, Enable signal setup time */
	 .tWC = 25,		/* tWC, ND_nWE cycle duration */
	 .tWH = 10,		/* tWH, ND_nWE high duration */
	 .tWP = 15,		/* tWP, ND_nWE pulse time */
	 .tRC = 30,		/* tWC, ND_nRE cycle duration */
	 .tRH = 15,		/* tRH, ND_nRE high duration */
	 .tRP = 15,		/* tRP, ND_nRE pulse width */
	 .tR = 60000,		/* tR = data transfer from cell to register, maximum 60,000ns */
	 .tWHR = 60,		/* tWHR, ND_nWE high to ND_nRE low delay for status read */
	 .tAR = 10,		/* tAR, ND_ALE low to ND_nRE low delay */
	 .tRHW = 48,		/* tRHW, ND_nRE high to ND_nWE low delay */
	 .pgPrBlk = 128,	/* Pages per block - detected */
	 .pgSz = 4096,		/* Page size */
	 .oobSz = 128,		/* Spare size */
	 .blkNum = 8192,	/* Number of blocks/sectors in the flash */
	 .id = 0xD7EC,		/* Device ID 0xDevice,Vendor */
	 .model = "Samsung 32Gb 8bit",
	 .bb_page = 127,	/* Manufacturer Bad block marking page in block */
	 .flags = NFC_CLOCK_UPSCALE_200M
	 },
	{			/* Micron 64Gb */
	 .tADL = 0,		/* tADL, Address to write data delay */
	 .tCH = 20,		/* tCH, Enable signal hold time */
	 .tCS = 20,		/* tCS, Enable signal setup time */
	 .tWC = 90,		/* tWC, ND_nWE cycle duration */
	 .tWH = 45,		/* tWH, ND_nWE high duration */
	 .tWP = 45,		/* tWP, ND_nWE pulse time */
	 .tRC = 90,		/* tWC, ND_nRE cycle duration */
	 .tRH = 45,		/* tRH, ND_nRE high duration */
	 .tRP = 45,		/* tRP, ND_nRE pulse width */
	 .tR = 0,		/* tR = data transfer from cell to register */
	 .tWHR = 90,		/* tWHR, ND_nWE high to ND_nRE low delay for status read */
	 .tAR = 65,		/* tAR, ND_ALE low to ND_nRE low delay */
	 .tRHW = 32,		/* tRHW, ND_nRE high to ND_nWE low delay */
	 .pgPrBlk = 256,	/* Pages per block - detected */
	 .pgSz = 8192,		/* Page size */
	 .oobSz = 448,		/* Spare size */
	 .blkNum = 4096,	/* Number of blocks/sectors in the flash */
	 .id = 0x882C,		/* Device ID 0xDevice,Vendor */
	 .model = "Micron 64Gb 8bit",
	 .bb_page = 0,		/* Manufacturer Bad block marking page in block */
	 .flags = NFC_CLOCK_UPSCALE_200M
	 },
	{			/* Hinyx 8Gb */
	.tADL = 0,		/* tADL, Address to write data delay */
	.tCH = 5,		/* tCH, Enable signal hold time */
	.tCS = 20,		/* tCS, Enable signal setup time */
	.tWC = 22,		/* tWC, ND_nWE cycle duration */
	.tWH = 10,		/* tWH, ND_nWE high duration */
	.tWP = 12,		/* tWP, ND_nWE pulse time */
	.tRC = 22,		/* tWC, ND_nRE cycle duration */
	.tRH = 10,		/* tRH, ND_nRE high duration */
	.tRP = 12,		/* tRP, ND_nRE pulse width */
	.tR = 25,		/* tR = data transfer from cell to register */
	.tWHR = 80,		/* tWHR, ND_nWE high to ND_nRE low delay for status read */
	.tAR = 10,		/* tAR, ND_ALE low to ND_nRE low delay */
	.tRHW = 100,		/* tRHW, ND_nRE high to ND_nWE low delay */
	.pgPrBlk = 64,		/* Pages per block - detected */
	.pgSz = 2048,		/* Page size */
	.oobSz = 64,		/* Spare size */
	.blkNum = 8192,		/* Number of blocks/sectors in the flash */
	.id = 0xDCAD,		/* Device ID 0xDevice,Vendor */
	.model = "Hynix 8Gb 8bit",
	.bb_page = 0,		/* Manufacturer Bad block marking page in block */
	 .flags = NFC_CLOCK_UPSCALE_200M
	},
	/* Timing used is ONFI Mode 2 (28Mhz) */
	{			/* Micron 8Gb */
	.tADL = 100,		/* tADL, Address to write data delay */
	.tCH = 10,		/* tCH, Enable signal hold time */
	.tCS = 25,		/* tCS, Enable signal setup time */
	.tWC = 35,		/* tWC, ND_nWE cycle duration */
	.tWH = 15,		/* tWH, ND_nWE high duration */
	.tWP = 20,		/* tWP, ND_nWE pulse time */
	.tRC = 35,		/* tWC, ND_nRE cycle duration */
	.tRH = 15,		/* tRH, ND_nRE high duration */
	.tRP = 17,		/* tRP, ND_nRE pulse width */
	.tR = 25241,		/* tR = data transfer from cell to register tR = tR+tRR+tWB+1 */
	.tWHR = 60,		/* tWHR, ND_nWE high to ND_nRE low delay for status read */
	.tAR = 10,		/* tAR, ND_ALE low to ND_nRE low delay */
	.tRHW = 100,		/* tRHW, ND_nRE high to ND_nWE low delay */
	.pgPrBlk = 128,		/* Pages per block - detected */
	.pgSz = 4096,		/* Page size */
	.oobSz = 224,		/* Spare size */
	.blkNum = 2048,		/* Number of blocks/sectors in the flash */
	.id = 0x382C,		/* Device ID 0xDevice,Vendor */
		.model = "Micron 8Gb 8bit",
	.bb_page = 0,		/* Manufacturer Bad block marking page in block */
	.flags = (NFC_CLOCK_UPSCALE_200M | NFC_FLAGS_ONFI_MODE_3_SET)
	}
};

/* Defined Command set */
#define 	MV_NFC_FLASH_SP_CMD_SET_IDX		0
#define		MV_NFC_FLASH_LP_CMD_SET_IDX		1
static MV_NFC_FLASH_CMD_SET flashCmdSet[] = {
	{
	 .read1 = 0x0000,
	 .read2 = 0x0050,
	 .program = 0x1080,
	 .readStatus = 0x0070,
	 .readId = 0x0090,
	 .erase = 0xD060,
	 .multiplaneErase = 0xD160,
	 .reset = 0x00FF,
	 .lock = 0x002A,
	 .unlock = 0x2423,
	 .lockStatus = 0x007A,
	 },
	{
	 .read1 = 0x3000,
	 .exitCacheRead = 0x003f,
	 .cacheReadRand = 0x3100,
	 .cacheReadSeq = 0x0031,
	 .read2 = 0x0050,
	 .program = 0x1080,
	 .readStatus = 0x0070,
	 .readId = 0x0090,
	 .erase = 0xD060,
	 .multiplaneErase = 0xD160,
	 .reset = 0x00FF,
	 .lock = 0x002A,
	 .unlock = 0x2423,
	 .lockStatus = 0x007A,
	 }
};

#if 0
static MV_U32 MV_REG_READ(MV_U32 offset)
{
	MV_U32 v;
	v = readl_relaxed((void *)(INTER_REGS_BASE | (offset)));
	printk("MV_REG_READ(0x%08x)=0x%08x\n", ((unsigned int) (INTER_REGS_BASE | (offset))), v);
	return v;
}

static void MV_REG_WRITE(MV_U32 offset, MV_U32 val)
{
	printk("MV_REG_WRITE(0x%08x,0x%08x)\n", ((unsigned int) (INTER_REGS_BASE | (offset))), val);
	writel_relaxed((val), (void *)(INTER_REGS_BASE | (offset)));
}
#endif

static MV_STATUS mvDfcWait4Complete(MV_U32 statMask, MV_U32 usec)
{
	MV_U32 i, sts;

	for (i = 0; i < usec; i++) {
		sts = (MV_REG_READ(NFC_STATUS_REG) & statMask);
		if (sts) {
			MV_REG_WRITE(NFC_STATUS_REG, sts);
			return MV_OK;
		}
		udelay(1);
	}

	return MV_TIMEOUT;
}


MV_STATUS mvNfcTransferDataLength(MV_NFC_CTRL *nfcCtrl, MV_NFC_CMD_TYPE cmd, MV_U32 *data_len)
{
	switch (cmd) {
	case MV_NFC_CMD_READ_ID:
		if (nfcCtrl->ioMode == MV_NFC_PDMA_ACCESS)
			*data_len = NFC_READ_ID_PDMA_DATA_LEN;
		else
			*data_len = NFC_READ_ID_PIO_DATA_LEN;
		break;

	case MV_NFC_CMD_READ_STATUS:
		if (nfcCtrl->ioMode == MV_NFC_PDMA_ACCESS)
			*data_len = NFC_READ_STATUS_PDMA_DATA_LEN;
		else
			*data_len = NFC_READ_STATUS_PIO_DATA_LEN;
		break;

	case MV_NFC_CMD_READ_MONOLITHIC:	/* Read a single 512B or 2KB page */
	case MV_NFC_CMD_READ_MULTIPLE:
	case MV_NFC_CMD_READ_NAKED:
	case MV_NFC_CMD_READ_LAST_NAKED:
	case MV_NFC_CMD_READ_DISPATCH:
	case MV_NFC_CMD_WRITE_MONOLITHIC:	/* Program a single page of 512B or 2KB */
	case MV_NFC_CMD_WRITE_MULTIPLE:
	case MV_NFC_CMD_WRITE_NAKED:
	case MV_NFC_CMD_WRITE_LAST_NAKED:
	case MV_NFC_CMD_WRITE_DISPATCH:
	case MV_NFC_CMD_EXIT_CACHE_READ:
	case MV_NFC_CMD_CACHE_READ_SEQ:
	case MV_NFC_CMD_CACHE_READ_START:
		if (nfcCtrl->ioMode == MV_NFC_PDMA_ACCESS) {
			/* Decide read data size based on page size */
			if (flashDeviceInfo[nfcCtrl->flashIdx].pgSz < MV_NFC_2KB_PAGE) {	/* Small Page */
				*data_len = NFC_RW_SP_PDMA_DATA_LEN;
			} else {	/* Large Page */

				if (nfcCtrl->eccMode == MV_NFC_ECC_BCH_2K)
					*data_len = NFC_RW_LP_BCH_ECC_DATA_LEN;
				else if (nfcCtrl->eccMode == MV_NFC_ECC_BCH_1K)
					*data_len = NFC_RW_LP_BCH1K_ECC_DATA_LEN;
				else if (nfcCtrl->eccMode == MV_NFC_ECC_BCH_704B)
					*data_len = NFC_RW_LP_BCH704B_ECC_DATA_LEN;
				else if (nfcCtrl->eccMode == MV_NFC_ECC_BCH_512B)
					*data_len = NFC_RW_LP_BCH512B_ECC_DATA_LEN;
				else	/* Hamming and No-Ecc */
					*data_len = NFC_RW_LP_PDMA_DATA_LEN;
			}
		} else {	/* PIO mode */

			/* Decide read data size based on page size */
			if (flashDeviceInfo[nfcCtrl->flashIdx].pgSz < MV_NFC_2KB_PAGE) {	/* Small Page */
				if (nfcCtrl->ifMode == MV_NFC_IF_2X8) {
					if (nfcCtrl->eccMode == MV_NFC_ECC_HAMMING)
						*data_len = NFC_RW_SP_G_HMNG_ECC_DATA_LEN;
					else	/* No ECC */
						*data_len = NFC_RW_SP_G_NO_ECC_DATA_LEN;
				} else {
					if (nfcCtrl->eccMode == MV_NFC_ECC_HAMMING)
						*data_len = NFC_RW_SP_HMNG_ECC_DATA_LEN;
					else	/* No ECC */
						*data_len = NFC_RW_SP_NO_ECC_DATA_LEN;
				}
			} else {	/* Large Page */

				if (nfcCtrl->ifMode == MV_NFC_IF_2X8) {
					if (nfcCtrl->eccMode == MV_NFC_ECC_BCH_2K)
						*data_len = NFC_RW_LP_G_BCH_ECC_DATA_LEN;
					else if (nfcCtrl->eccMode == MV_NFC_ECC_BCH_1K)
						*data_len = NFC_RW_LP_BCH1K_ECC_DATA_LEN;
					else if (nfcCtrl->eccMode == MV_NFC_ECC_BCH_704B)
						*data_len = NFC_RW_LP_BCH704B_ECC_DATA_LEN;
					else if (nfcCtrl->eccMode == MV_NFC_ECC_BCH_512B)
						*data_len = NFC_RW_LP_BCH512B_ECC_DATA_LEN;
					else if (nfcCtrl->eccMode == MV_NFC_ECC_HAMMING)
						*data_len = NFC_RW_LP_G_HMNG_ECC_DATA_LEN;
					else	/* No ECC */
						*data_len = NFC_RW_LP_G_NO_ECC_DATA_LEN;
				} else {
					if (nfcCtrl->eccMode == MV_NFC_ECC_BCH_2K)
						*data_len = NFC_RW_LP_BCH_ECC_DATA_LEN;
					else if (nfcCtrl->eccMode == MV_NFC_ECC_BCH_1K)
						*data_len = NFC_RW_LP_BCH1K_ECC_DATA_LEN;
					else if (nfcCtrl->eccMode == MV_NFC_ECC_BCH_704B)
						*data_len = NFC_RW_LP_BCH704B_ECC_DATA_LEN;
					else if (nfcCtrl->eccMode == MV_NFC_ECC_BCH_512B)
						*data_len = NFC_RW_LP_BCH512B_ECC_DATA_LEN;
					else if (nfcCtrl->eccMode == MV_NFC_ECC_HAMMING)
						*data_len = NFC_RW_LP_HMNG_ECC_DATA_LEN;
					else	/* No ECC */
						*data_len = NFC_RW_LP_NO_ECC_DATA_LEN;
				}
			}
		}
		break;

	case MV_NFC_CMD_ERASE:
	case MV_NFC_CMD_MULTIPLANE_ERASE:
	case MV_NFC_CMD_RESET:
	case MV_NFC_CMD_WRITE_DISPATCH_START:
	case MV_NFC_CMD_WRITE_DISPATCH_END:
		return MV_BAD_PARAM;

	default:
		return MV_BAD_PARAM;

	};

	return MV_OK;
}

MV_STATUS mvNfcFlashPageSizeGet(MV_NFC_CTRL *nfcCtrl, uint32_t *size, uint32_t *totalSize)
{
	if (nfcCtrl->flashIdx >= (sizeof(flashDeviceInfo) / sizeof(MV_NFC_FLASH_INFO)))
		return MV_NOT_FOUND;
	if (size == NULL)
		return MV_BAD_PTR;

	if (nfcCtrl->ifMode == MV_NFC_IF_2X8)
		*size = flashDeviceInfo[nfcCtrl->flashIdx].pgSz << 1;
	else
		*size = flashDeviceInfo[nfcCtrl->flashIdx].pgSz;

	if (totalSize) {
		mvNfcTransferDataLength(nfcCtrl, MV_NFC_CMD_READ_MONOLITHIC, totalSize);
		if (nfcCtrl->ifMode == MV_NFC_IF_2X8)
			*totalSize = (*totalSize) << 1;
		if (flashDeviceInfo[nfcCtrl->flashIdx].pgSz > MV_NFC_2KB_PAGE)
			*totalSize = (*totalSize) << 1;
	}
	return MV_OK;
}

MV_STATUS mvNfcIntrSet(MV_NFC_CTRL *nfcCtrl, MV_U32 intMask, MV_BOOL enable)
{
	MV_U32 reg;
	MV_U32 msk = (intMask & (NFC_SR_WRCMDREQ_MASK | NFC_SR_RDDREQ_MASK | NFC_SR_WRDREQ_MASK |
				 NFC_SR_CORERR_MASK | NFC_SR_UNCERR_MASK));

	if ((nfcCtrl->currCs == MV_NFC_CS_0) || (nfcCtrl->currCs == MV_NFC_CS_2)) {
		if (intMask & MV_NFC_STATUS_BBD)
			msk |= NFC_SR_CS0_BBD_MASK;
		if (intMask & MV_NFC_STATUS_CMDD)
			msk |= NFC_SR_CS0_CMDD_MASK;
		if (intMask & MV_NFC_STATUS_PAGED)
			msk |= NFC_SR_CS0_PAGED_MASK;
		if (intMask & MV_NFC_STATUS_RDY)
			msk |= NFC_SR_RDY0_MASK;
	} else if ((nfcCtrl->currCs == MV_NFC_CS_1) || (nfcCtrl->currCs == MV_NFC_CS_3)) {
		if (intMask & MV_NFC_STATUS_BBD)
			msk |= NFC_SR_CS1_BBD_MASK;
		if (intMask & MV_NFC_STATUS_CMDD)
			msk |= NFC_SR_CS1_CMDD_MASK;
		if (intMask & MV_NFC_STATUS_PAGED)
			msk |= NFC_SR_CS1_PAGED_MASK;
		if (intMask & MV_NFC_STATUS_RDY)
			msk |= NFC_SR_RDY0_MASK;
	}

	reg = MV_REG_READ(NFC_CONTROL_REG);
	if (enable)
		reg &= ~msk;
	else
		reg |= msk;

	MV_REG_WRITE(NFC_CONTROL_REG, reg);

	return MV_OK;
}

static MV_STATUS mvNfcBuildCommand(MV_NFC_CTRL *nfcCtrl, MV_NFC_MULTI_CMD *descInfo, MV_U32 *cmdb)
{
	cmdb[0] = 0;
	cmdb[1] = 0;
	cmdb[2] = 0;
	cmdb[3] = 0;
	if (nfcCtrl->autoStatusRead)
		cmdb[0] |= NFC_CB0_AUTO_RS_MASK;

	if ((nfcCtrl->currCs == MV_NFC_CS_1) || (nfcCtrl->currCs == MV_NFC_CS_3))
		cmdb[0] |= NFC_CB0_CSEL_MASK;

	if ((nfcCtrl->currCs == MV_NFC_CS_2) || (nfcCtrl->currCs == MV_NFC_CS_3))
		cmdb[2] |= NFC_CB2_CS_2_3_SELECT_MASK;

	if (nfcCtrl->readyBypass)
		cmdb[0] |= NFC_CB0_RDY_BYP_MASK;

	switch (descInfo->cmd) {
	case MV_NFC_CMD_READ_ID:
		cmdb[0] |= (flashCmdSet[nfcCtrl->cmdsetIdx].readId & (NFC_CB0_CMD1_MASK | NFC_CB0_CMD2_MASK));
		cmdb[0] |= ((NFC_READ_ID_ADDR_LEN << NFC_CB0_ADDR_CYC_OFFS) & NFC_CB0_ADDR_CYC_MASK);
		cmdb[0] |= NFC_CB0_CMD_TYPE_READ_ID;
		break;

	case MV_NFC_CMD_READ_STATUS:
		cmdb[0] |= (flashCmdSet[nfcCtrl->cmdsetIdx].readStatus & (NFC_CB0_CMD1_MASK | NFC_CB0_CMD2_MASK));
		cmdb[0] |= NFC_CB0_CMD_TYPE_STATUS;
		break;

	case MV_NFC_CMD_ERASE:
	case MV_NFC_CMD_MULTIPLANE_ERASE:

		if (descInfo->cmd == MV_NFC_CMD_ERASE)
			cmdb[0] |= (flashCmdSet[nfcCtrl->cmdsetIdx].erase & (NFC_CB0_CMD1_MASK | NFC_CB0_CMD2_MASK));
		if (descInfo->cmd == MV_NFC_CMD_MULTIPLANE_ERASE)
			cmdb[0] |=
			    (flashCmdSet[nfcCtrl->cmdsetIdx].multiplaneErase & (NFC_CB0_CMD1_MASK | NFC_CB0_CMD2_MASK));

		cmdb[0] |= ((NFC_ERASE_ADDR_LEN << NFC_CB0_ADDR_CYC_OFFS) & NFC_CB0_ADDR_CYC_MASK);
		cmdb[0] |= NFC_CB0_DBC_MASK;
		cmdb[0] |= NFC_CB0_CMD_TYPE_ERASE;
		cmdb[1] |= (descInfo->pageAddr & NFC_BLOCK_ADDR_BITS);
		break;

	case MV_NFC_CMD_RESET:
		cmdb[0] |= (flashCmdSet[nfcCtrl->cmdsetIdx].reset & (NFC_CB0_CMD1_MASK | NFC_CB0_CMD2_MASK));
		cmdb[0] |= NFC_CB0_CMD_TYPE_RESET;
		break;

	case MV_NFC_CMD_CACHE_READ_SEQ:
		cmdb[0] = (flashCmdSet[nfcCtrl->cmdsetIdx].cacheReadSeq & (NFC_CB0_CMD1_MASK | NFC_CB0_CMD2_MASK));
		break;

	case MV_NFC_CMD_CACHE_READ_RAND:
		cmdb[0] = (flashCmdSet[nfcCtrl->cmdsetIdx].cacheReadRand & (NFC_CB0_CMD1_MASK | NFC_CB0_CMD2_MASK));
		if (flashDeviceInfo[nfcCtrl->flashIdx].pgSz < MV_NFC_2KB_PAGE) {
			cmdb[1] |= ((descInfo->pageAddr << NFC_SP_PG_OFFS) & NFC_SP_PG_MASK);
			if (descInfo->pageAddr & ~NFC_SP_PG_MASK)
				cmdb[0] |=
				    ((NFC_SP_BIG_READ_ADDR_LEN << NFC_CB0_ADDR_CYC_OFFS) & NFC_CB0_ADDR_CYC_MASK);
			else
				cmdb[0] |= ((NFC_SP_READ_ADDR_LEN << NFC_CB0_ADDR_CYC_OFFS) & NFC_CB0_ADDR_CYC_MASK);
		} else {
			cmdb[0] |= ((NFC_LP_READ_ADDR_LEN << NFC_CB0_ADDR_CYC_OFFS) & NFC_CB0_ADDR_CYC_MASK);
			cmdb[0] |= NFC_CB0_DBC_MASK;
			cmdb[1] |= ((descInfo->pageAddr << NFC_LP_PG_OFFS) & NFC_LP_PG_MASK);
			cmdb[2] |= (descInfo->pageAddr >> (32 - NFC_LP_PG_OFFS));
		}
		cmdb[0] |= NFC_CB0_CMD_TYPE_READ;
		break;

	case MV_NFC_CMD_EXIT_CACHE_READ:
		cmdb[0] |= (flashCmdSet[nfcCtrl->cmdsetIdx].exitCacheRead & (NFC_CB0_CMD1_MASK | NFC_CB0_CMD2_MASK));
		break;

	case MV_NFC_CMD_CACHE_READ_START:
		cmdb[0] |= (flashCmdSet[nfcCtrl->cmdsetIdx].read1 & (NFC_CB0_CMD1_MASK | NFC_CB0_CMD2_MASK));
		if (flashDeviceInfo[nfcCtrl->flashIdx].pgSz < MV_NFC_2KB_PAGE) {
			cmdb[1] |= ((descInfo->pageAddr << NFC_SP_PG_OFFS) & NFC_SP_PG_MASK);
			if (descInfo->pageAddr & ~NFC_SP_PG_MASK)
				cmdb[0] |=
				    ((NFC_SP_BIG_READ_ADDR_LEN << NFC_CB0_ADDR_CYC_OFFS) & NFC_CB0_ADDR_CYC_MASK);
			else
				cmdb[0] |= ((NFC_SP_READ_ADDR_LEN << NFC_CB0_ADDR_CYC_OFFS) & NFC_CB0_ADDR_CYC_MASK);
		} else {
			cmdb[0] |= ((NFC_LP_READ_ADDR_LEN << NFC_CB0_ADDR_CYC_OFFS) & NFC_CB0_ADDR_CYC_MASK);
			cmdb[0] |= NFC_CB0_DBC_MASK;
			cmdb[1] |= ((descInfo->pageAddr << NFC_LP_PG_OFFS) & NFC_LP_PG_MASK);
			cmdb[2] |= (descInfo->pageAddr >> (32 - NFC_LP_PG_OFFS));
		}
		cmdb[0] |= NFC_CB0_CMD_TYPE_READ;
		cmdb[0] |= NFC_CB0_LEN_OVRD_MASK;
		break;

	case MV_NFC_CMD_READ_MONOLITHIC:	/* Read a single 512B or 2KB page */
	case MV_NFC_CMD_READ_MULTIPLE:
	case MV_NFC_CMD_READ_NAKED:
	case MV_NFC_CMD_READ_LAST_NAKED:
	case MV_NFC_CMD_READ_DISPATCH:
		cmdb[0] |= (flashCmdSet[nfcCtrl->cmdsetIdx].read1 & (NFC_CB0_CMD1_MASK | NFC_CB0_CMD2_MASK));
		if (flashDeviceInfo[nfcCtrl->flashIdx].pgSz < MV_NFC_2KB_PAGE) {
			cmdb[1] |= ((descInfo->pageAddr << NFC_SP_PG_OFFS) & NFC_SP_PG_MASK);
			if (descInfo->pageAddr & ~NFC_SP_PG_MASK)
				cmdb[0] |=
				    ((NFC_SP_BIG_READ_ADDR_LEN << NFC_CB0_ADDR_CYC_OFFS) & NFC_CB0_ADDR_CYC_MASK);
			else
				cmdb[0] |= ((NFC_SP_READ_ADDR_LEN << NFC_CB0_ADDR_CYC_OFFS) & NFC_CB0_ADDR_CYC_MASK);
		} else {
			cmdb[0] |= ((NFC_LP_READ_ADDR_LEN << NFC_CB0_ADDR_CYC_OFFS) & NFC_CB0_ADDR_CYC_MASK);
			cmdb[0] |= NFC_CB0_DBC_MASK;
			cmdb[1] |= ((descInfo->pageAddr << NFC_LP_PG_OFFS) & NFC_LP_PG_MASK);
			cmdb[2] |= (descInfo->pageAddr >> (32 - NFC_LP_PG_OFFS));
		}
		cmdb[0] |= NFC_CB0_CMD_TYPE_READ;

		if (descInfo->length) {
			cmdb[0] |= NFC_CB0_LEN_OVRD_MASK;
			cmdb[3] |= (descInfo->length & 0xFFFF);
		}

		/* Check for extended command syntax */
		switch (descInfo->cmd) {
		case MV_NFC_CMD_READ_MULTIPLE:
			cmdb[0] |= NFC_CB0_CMD_XTYPE_MULTIPLE;
			break;
		case MV_NFC_CMD_READ_NAKED:
			cmdb[0] |= NFC_CB0_CMD_XTYPE_NAKED;
			break;
		case MV_NFC_CMD_READ_LAST_NAKED:
			cmdb[0] |= NFC_CB0_CMD_XTYPE_LAST_NAKED;
			break;
		case MV_NFC_CMD_READ_DISPATCH:
			cmdb[0] |= NFC_CB0_CMD_XTYPE_DISPATCH;
			break;
		default:
			break;
		};
		break;

	case MV_NFC_CMD_WRITE_MONOLITHIC:	/* Program a single page of 512B or 2KB */
	case MV_NFC_CMD_WRITE_MULTIPLE:
		/*case MV_NFC_CMD_WRITE_NAKED: */
	case MV_NFC_CMD_WRITE_LAST_NAKED:
	case MV_NFC_CMD_WRITE_DISPATCH:
		cmdb[0] |= (flashCmdSet[nfcCtrl->cmdsetIdx].program & (NFC_CB0_CMD1_MASK | NFC_CB0_CMD2_MASK));
		if (flashDeviceInfo[nfcCtrl->flashIdx].pgSz < MV_NFC_2KB_PAGE) {
			if (descInfo->pageAddr & ~NFC_SP_PG_MASK)
				cmdb[0] |=
				    ((NFC_SP_BIG_READ_ADDR_LEN << NFC_CB0_ADDR_CYC_OFFS) & NFC_CB0_ADDR_CYC_MASK);
			else
				cmdb[0] |= ((NFC_SP_READ_ADDR_LEN << NFC_CB0_ADDR_CYC_OFFS) & NFC_CB0_ADDR_CYC_MASK);
			cmdb[1] |= ((descInfo->pageAddr << NFC_SP_PG_OFFS) & NFC_SP_PG_MASK);
		} else {
			cmdb[0] |= ((NFC_LP_READ_ADDR_LEN << NFC_CB0_ADDR_CYC_OFFS) & NFC_CB0_ADDR_CYC_MASK);
			cmdb[1] |= ((descInfo->pageAddr << NFC_LP_PG_OFFS) & NFC_LP_PG_MASK);
			cmdb[2] |= (descInfo->pageAddr >> (32 - NFC_LP_PG_OFFS));
		}
		cmdb[0] |= NFC_CB0_DBC_MASK;
		cmdb[0] |= NFC_CB0_CMD_TYPE_WRITE;

		/* Check for extended syntax */
		switch (descInfo->cmd) {
		case MV_NFC_CMD_WRITE_MULTIPLE:
			cmdb[0] |= NFC_CB0_CMD_XTYPE_MULTIPLE;
			break;
		case MV_NFC_CMD_WRITE_NAKED:
			cmdb[0] |= NFC_CB0_CMD_XTYPE_NAKED;
			break;
		case MV_NFC_CMD_WRITE_LAST_NAKED:
			cmdb[0] |= NFC_CB0_CMD_XTYPE_LAST_NAKED;
			break;
		case MV_NFC_CMD_WRITE_DISPATCH:
			cmdb[0] |= NFC_CB0_CMD_XTYPE_DISPATCH;
			break;
		default:
			break;
		};
		break;

	case MV_NFC_CMD_WRITE_DISPATCH_START:
		cmdb[0] |= (flashCmdSet[nfcCtrl->cmdsetIdx].program & NFC_CB0_CMD1_MASK);
		if (flashDeviceInfo[nfcCtrl->flashIdx].pgSz < MV_NFC_2KB_PAGE) {
			if (descInfo->pageAddr & ~NFC_SP_PG_MASK)
				cmdb[0] |=
				    ((NFC_SP_BIG_READ_ADDR_LEN << NFC_CB0_ADDR_CYC_OFFS) & NFC_CB0_ADDR_CYC_MASK);
			else
				cmdb[0] |= ((NFC_SP_READ_ADDR_LEN << NFC_CB0_ADDR_CYC_OFFS) & NFC_CB0_ADDR_CYC_MASK);
			cmdb[1] |= ((descInfo->pageAddr << NFC_SP_PG_OFFS) & NFC_SP_PG_MASK);
		} else {
			cmdb[0] |= ((NFC_LP_READ_ADDR_LEN << NFC_CB0_ADDR_CYC_OFFS) & NFC_CB0_ADDR_CYC_MASK);
			cmdb[1] |= ((descInfo->pageAddr << NFC_LP_PG_OFFS) & NFC_LP_PG_MASK);
			cmdb[2] |= (descInfo->pageAddr >> (32 - NFC_LP_PG_OFFS));
		}
		cmdb[0] |= NFC_CB0_CMD_TYPE_WRITE;
		cmdb[0] |= NFC_CB0_CMD_XTYPE_DISPATCH;
		break;

	case MV_NFC_CMD_WRITE_NAKED:
		cmdb[0] |= NFC_CB0_CMD_TYPE_WRITE;
		cmdb[0] |= NFC_CB0_CMD_XTYPE_NAKED;
		if (descInfo->length) {
			cmdb[0] |= NFC_CB0_LEN_OVRD_MASK;
			cmdb[3] |= (descInfo->length & 0xFFFF);
		}
		break;

	case MV_NFC_CMD_WRITE_DISPATCH_END:
		cmdb[0] |= ((flashCmdSet[nfcCtrl->cmdsetIdx].program >> 8) & NFC_CB0_CMD1_MASK);
		cmdb[0] |= NFC_CB0_CMD_TYPE_WRITE;
		cmdb[0] |= NFC_CB0_CMD_XTYPE_DISPATCH;
		break;

	default:
		return MV_BAD_PARAM;
	}

	/* update page count */
	cmdb[2] |= (((descInfo->pageCount - 1) << NFC_PG_CNT_OFFS) & NFC_PG_CNT_MASK);

	return MV_OK;
}

MV_STATUS mvNfcCommandPio(MV_NFC_CTRL *nfcCtrl, MV_NFC_MULTI_CMD *cmd_desc, MV_BOOL next)
{
	MV_U32 reg;
	MV_U32 errCode = MV_OK;
	MV_U32 cmdb_pio[4];
	MV_U32 *cmdb;
	MV_U32 timeout = 10000;
	MV_STATUS ret;

	/* Check that a chip was selected */
	if (nfcCtrl->currCs == MV_NFC_CS_NONE)
		return MV_FAIL;

	/* Clear all old events on the status register */
	reg = MV_REG_READ(NFC_STATUS_REG);
	MV_REG_WRITE(NFC_STATUS_REG, reg);

	/* Setting ND_RUN bit to start the new transaction - verify that controller in idle state */
	while (timeout > 0) {
		reg = MV_REG_READ(NFC_CONTROL_REG);
		if (!(reg & NFC_CTRL_ND_RUN_MASK))
			break;
		timeout--;
	}

	if (timeout == 0)
		return MV_BAD_STATE;

	reg |= NFC_CTRL_ND_RUN_MASK;
	MV_REG_WRITE(NFC_CONTROL_REG, reg);

	/* Wait for Command WRITE request */
	mvDfcWait4Complete(NFC_SR_WRCMDREQ_MASK, 1);
	if (errCode != MV_OK)
		return errCode;

	/* Build 12 byte Command */
	if (nfcCtrl->ioMode == MV_NFC_PDMA_ACCESS)
		cmdb = (MV_U32 *) nfcCtrl->cmdBuff.bufVirtPtr;
	else			/* PIO mode */
		cmdb = cmdb_pio;

	if (nfcCtrl->eccMode != MV_NFC_ECC_DISABLE) {
		switch (cmd_desc->cmd) {
		case MV_NFC_CMD_READ_MONOLITHIC:
		case MV_NFC_CMD_READ_MULTIPLE:
		case MV_NFC_CMD_READ_NAKED:
		case MV_NFC_CMD_READ_LAST_NAKED:
		case MV_NFC_CMD_WRITE_MONOLITHIC:
		case MV_NFC_CMD_WRITE_MULTIPLE:
		case MV_NFC_CMD_WRITE_NAKED:
		case MV_NFC_CMD_WRITE_LAST_NAKED:
			if (nfcCtrl->eccMode != MV_NFC_ECC_DISABLE) {
				MV_REG_BIT_SET(NFC_CONTROL_REG, NFC_CTRL_ECC_EN_MASK);
				if (nfcCtrl->eccMode != MV_NFC_ECC_HAMMING)
					MV_REG_BIT_SET(NFC_ECC_CONTROL_REG, NFC_ECC_BCH_EN_MASK);
			}
			break;

		default:
			/* disable ECC for non-data commands */
			MV_REG_BIT_RESET(NFC_CONTROL_REG, NFC_CTRL_ECC_EN_MASK);
			MV_REG_BIT_RESET(NFC_ECC_CONTROL_REG, NFC_ECC_BCH_EN_MASK);
			break;
		};
	}

	/* Build the command buffer */
	ret = mvNfcBuildCommand(nfcCtrl, cmd_desc, cmdb);
	if (ret != MV_OK)
		return ret;

	/* If next command, link to it */
	if (next)
		cmdb[0] |= NFC_CB0_NEXT_CMD_MASK;

	/* issue command */
	MV_REG_WRITE(NFC_COMMAND_BUFF_0_REG, cmdb[0]);
	MV_REG_WRITE(NFC_COMMAND_BUFF_0_REG, cmdb[1]);
	MV_REG_WRITE(NFC_COMMAND_BUFF_0_REG, cmdb[2]);
	MV_REG_WRITE(NFC_COMMAND_BUFF_0_REG, cmdb[3]);

	return MV_OK;
}

MV_VOID mvNfcReadWritePio(MV_NFC_CTRL *nfcCtrl, MV_U32 *buff, MV_U32 data_len, MV_NFC_PIO_RW_MODE mode)
{
	MV_U32 i;

	switch (mode) {
	case MV_NFC_PIO_READ:
		for (i = 0; i < data_len; i += 4) {
			*buff = MV_REG_READ(NFC_DATA_BUFF_REG);
			buff++;
		}
		break;

	case MV_NFC_PIO_WRITE:	/* Program a single page of 512B or 2KB */
		for (i = 0; i < data_len; i += 4) {
			MV_REG_WRITE(NFC_DATA_BUFF_REG, *buff);
			buff++;
		}
		break;

	default:
		/* nothing to do */
		break;
	};
}

MV_STATUS mvNfcReset(void)
{
	MV_U32 reg;
	MV_U32 errCode = MV_OK;
	MV_U32 timeout = 10000;

	/* Clear all old events on the status register */
	reg = MV_REG_READ(NFC_STATUS_REG);
	MV_REG_WRITE(NFC_STATUS_REG, reg);

	/* Setting ND_RUN bit to start the new transaction */
	reg = MV_REG_READ(NFC_CONTROL_REG);
	reg |= NFC_CTRL_ND_RUN_MASK;
	MV_REG_WRITE(NFC_CONTROL_REG, reg);

	/* Wait for Command WRITE request */
	errCode = mvDfcWait4Complete(NFC_SR_WRCMDREQ_MASK, 1);
	if (errCode != MV_OK)
		goto Error;

	/* Send Command */
	MV_REG_WRITE(NFC_COMMAND_BUFF_0_REG, 0x00A000FF);	/* DFC_NDCB0_RESET */
	MV_REG_WRITE(NFC_COMMAND_BUFF_0_REG, 0x0);
	MV_REG_WRITE(NFC_COMMAND_BUFF_0_REG, 0x0);

	/* Wait for Command completion */
	errCode = mvDfcWait4Complete(NFC_SR_RDY0_MASK, 1000);
	if (errCode != MV_OK)
		goto Error;

	/* Wait for ND_RUN bit to get cleared. */
	while (timeout > 0) {
		reg = MV_REG_READ(NFC_CONTROL_REG);
		if (!(reg & NFC_CTRL_ND_RUN_MASK))
			break;
		timeout--;
	}
	if (timeout == 0)
		return MV_BAD_STATE;

Error:
	return errCode;
}

MV_STATUS mvNfcSelectChip(MV_NFC_CTRL *nfcCtrl, MV_NFC_CHIP_SEL chip)
{
	nfcCtrl->currCs = chip;
	return MV_OK;
}

MV_U32 mvNfcBadBlockPageNumber(MV_NFC_CTRL *nfcCtrl)
{
	return flashDeviceInfo[nfcCtrl->flashIdx].bb_page;
}

MV_VOID setNANDClock(MV_U32 nClock)
{
    /* Set the division ratio of ECC Clock 0x00018748[13:8] (by default it's double of core clock) */
    MV_U32 nVal = MV_REG_READ(0x18748);

    //printk(KERN_INFO "%s:%4d:%s nVal=%08x\n", __FILE__, __LINE__, __FUNCTION__, nVal);

    nVal = nVal & ~0x3F00;
    nVal = nVal | (nClock<<8);
    MV_REG_WRITE(0x18748, nVal);

    {
	    nVal = MV_REG_READ(0x18748);
	    //printk(KERN_INFO "%s:%4d:%s nVal=%08x\n", __FILE__, __LINE__, __FUNCTION__, nVal);
    }

    /* Set reload force of ECC clock 0x00018740[7:0] to 0x2 (meaning you will force only the ECC clock) */
    nVal = MV_REG_READ(0x18740);
    //printk(KERN_INFO "%s:%4d:%s nVal=%08x\n", __FILE__, __LINE__, __FUNCTION__, nVal);

    nVal = nVal & ~(0xff);
    nVal = nVal | 0x2;
    MV_REG_WRITE(0x18740, nVal);

    {
	    nVal = MV_REG_READ(0x18740);
	    //printk(KERN_INFO "%s:%4d:%s nVal=%08x\n", __FILE__, __LINE__, __FUNCTION__, nVal);
    }

    /* Set reload ratio bit 0x00018740[8] to 1'b1 */
    MV_REG_BIT_SET(0x18740, 0x100);
    mdelay(1);
    /* Set reload ratio bit 0x00018740[8] to 1'b1 */
    MV_REG_BIT_RESET(0x18740, 0x100);
}

static MV_STATUS mvNfcReadIdNative(MV_NFC_CHIP_SEL cs, MV_U16 *id)
{
	MV_U32 reg, cmdb0 = 0, cmdb2 = 0;
	MV_U32 errCode = MV_OK;

	/* Clear all old events on the status register */
	reg = MV_REG_READ(NFC_STATUS_REG);
	MV_REG_WRITE(NFC_STATUS_REG, reg);

	/* Setting ND_RUN bit to start the new transaction */
	reg = MV_REG_READ(NFC_CONTROL_REG);
	reg |= NFC_CTRL_ND_RUN_MASK;
	MV_REG_WRITE(NFC_CONTROL_REG, reg);

	/* Wait for Command WRITE request */
	errCode = mvDfcWait4Complete(NFC_SR_WRCMDREQ_MASK, 1);
	if (errCode != MV_OK)
		return errCode;

	/* Send Command */
	reg = NFC_NATIVE_READ_ID_CMD;
	reg |= (0x1 << NFC_CB0_ADDR_CYC_OFFS);
	reg |= NFC_CB0_CMD_TYPE_READ_ID;
	cmdb0 = reg;
	if ((cs == MV_NFC_CS_1) || (cs == MV_NFC_CS_3))
		cmdb0 |= NFC_CB0_CSEL_MASK;

	if ((cs == MV_NFC_CS_2) || (cs == MV_NFC_CS_3))
		cmdb2 |= NFC_CB2_CS_2_3_SELECT_MASK;

	MV_REG_WRITE(NFC_COMMAND_BUFF_0_REG, cmdb0);
	MV_REG_WRITE(NFC_COMMAND_BUFF_0_REG, 0x0);
	MV_REG_WRITE(NFC_COMMAND_BUFF_0_REG, cmdb2);

	/* Wait for Data READ request */
	mvDfcWait4Complete(NFC_SR_RDDREQ_MASK, 10);
	if (errCode != MV_OK)
		return errCode;

	/*  Read the read ID bytes. + read 4 bogus bytes */
	*id = (MV_U16) (MV_REG_READ(NFC_DATA_BUFF_REG) & 0xFFFF);
	reg = MV_REG_READ(NFC_DATA_BUFF_REG);	/* dummy read to complete 8 bytes */

	reg = MV_REG_READ(NFC_CONTROL_REG);
	if (reg & NFC_CTRL_ND_RUN_MASK) {
		MV_REG_WRITE(NFC_CONTROL_REG, (reg & ~NFC_CTRL_ND_RUN_MASK));
		return MV_BAD_STATE;
	}

	return MV_OK;
}

static MV_STATUS mvNfcDeviceFeatureSet(MV_NFC_CTRL *nfcCtrl, MV_U8 cmd, MV_U8 addr, MV_U32 data0, MV_U32 data1)
{
	MV_U32 reg;
	MV_U32 errCode = MV_OK;
	MV_U32 timeout = 10000;

	/* Clear all old events on the status register */
	reg = MV_REG_READ(NFC_STATUS_REG);
	MV_REG_WRITE(NFC_STATUS_REG, reg);

	/* Setting ND_RUN bit to start the new transaction */
	reg = MV_REG_READ(NFC_CONTROL_REG);
	reg |= NFC_CTRL_ND_RUN_MASK;
	MV_REG_WRITE(NFC_CONTROL_REG, reg);

	/* Wait for Command WRITE request */
	errCode = mvDfcWait4Complete(NFC_SR_WRCMDREQ_MASK, 1);
	if (errCode != MV_OK)
		goto Error;

	reg = MV_REG_READ(NFC_STATUS_REG);
	MV_REG_WRITE(NFC_STATUS_REG, reg);

	/* Send Naked Command Dispatch Command*/
	reg = cmd;
	reg |= (0x1 << NFC_CB0_ADDR_CYC_OFFS);
	reg |= NFC_CB0_CMD_XTYPE_MULTIPLE;
	reg |= NFC_CB0_CMD_TYPE_WRITE;
	reg |= NFC_CB0_LEN_OVRD_MASK;

	MV_REG_WRITE(NFC_COMMAND_BUFF_0_REG, reg);
	MV_REG_WRITE(NFC_COMMAND_BUFF_0_REG, addr);
	MV_REG_WRITE(NFC_COMMAND_BUFF_0_REG, 0x0);
	MV_REG_WRITE(NFC_COMMAND_BUFF_0_REG, 0x8);

	/* Wait for Data READ request */
	errCode = mvDfcWait4Complete(NFC_SR_WRDREQ_MASK, 10);
	if (errCode != MV_OK)
		return errCode;

	udelay(100);

	MV_REG_WRITE(NFC_DATA_BUFF_REG, data0);
	MV_REG_WRITE(NFC_DATA_BUFF_REG, data1);

	/* Wait for Data READ request */
	errCode = mvDfcWait4Complete(NFC_SR_RDY0_MASK, 10);
	if (errCode != MV_OK)
		return errCode;

	/* Wait for ND_RUN bit to get cleared. */
	while (timeout > 0) {
		reg = MV_REG_READ(NFC_CONTROL_REG);
		if (!(reg & NFC_CTRL_ND_RUN_MASK))
			break;
		timeout--;
	}
	if (timeout == 0)
		return MV_BAD_STATE;

Error:
	return errCode;
}

static MV_STATUS mvNfcDeviceFeatureGet(MV_NFC_CTRL *nfcCtrl, MV_U8 cmd, MV_U8 addr, MV_U32 *data0, MV_U32 *data1)
{
	MV_U32 reg;
	MV_U32 errCode = MV_OK;
	MV_U32 timeout = 10000;

	/* Clear all old events on the status register */
	reg = MV_REG_READ(NFC_STATUS_REG);
	MV_REG_WRITE(NFC_STATUS_REG, reg);

	/* Setting ND_RUN bit to start the new transaction */
	reg = MV_REG_READ(NFC_CONTROL_REG);
	reg |= NFC_CTRL_ND_RUN_MASK;
	MV_REG_WRITE(NFC_CONTROL_REG, reg);

	/* Wait for Command WRITE request */
	errCode = mvDfcWait4Complete(NFC_SR_WRCMDREQ_MASK, 1);
	if (errCode != MV_OK)
		goto Error;

	reg = MV_REG_READ(NFC_STATUS_REG);
	MV_REG_WRITE(NFC_STATUS_REG, reg);

	/* Send Read Command */
	reg = cmd;
	reg |= (0x1 << NFC_CB0_ADDR_CYC_OFFS);
	reg |= NFC_CB0_CMD_XTYPE_MULTIPLE;
	reg |= NFC_CB0_CMD_TYPE_READ;
	reg |= NFC_CB0_LEN_OVRD_MASK;

	MV_REG_WRITE(NFC_COMMAND_BUFF_0_REG, reg);
	MV_REG_WRITE(NFC_COMMAND_BUFF_0_REG, addr);
	MV_REG_WRITE(NFC_COMMAND_BUFF_0_REG, 0x0);
	MV_REG_WRITE(NFC_COMMAND_BUFF_0_REG, 0x8);

	/* Wait for READY */
	errCode = mvDfcWait4Complete(NFC_SR_RDY0_MASK, 10);
	if (errCode != MV_OK)
		return errCode;

	udelay(100);
	/* Send Last-Naked Read Command */
	reg = 0x0;
	reg |= NFC_CB0_CMD_XTYPE_LAST_NAKED;
	reg |= NFC_CB0_CMD_TYPE_READ;
	reg |= NFC_CB0_LEN_OVRD_MASK;

	MV_REG_WRITE(NFC_COMMAND_BUFF_0_REG, reg);
	MV_REG_WRITE(NFC_COMMAND_BUFF_0_REG, 0x0);
	MV_REG_WRITE(NFC_COMMAND_BUFF_0_REG, 0x0);
	MV_REG_WRITE(NFC_COMMAND_BUFF_0_REG, 0x8);

	/* Wait for Data READ request */
	errCode = mvDfcWait4Complete(NFC_SR_RDDREQ_MASK, 100);
	if (errCode != MV_OK)
		return errCode;

	/*  Read the data + read 4 bogus bytes */
	*data0 = MV_REG_READ(NFC_DATA_BUFF_REG);
	*data1 = MV_REG_READ(NFC_DATA_BUFF_REG);

	/* Wait for ND_RUN bit to get cleared. */
	while (timeout > 0) {
		reg = MV_REG_READ(NFC_CONTROL_REG);
		if (!(reg & NFC_CTRL_ND_RUN_MASK))
			break;
		timeout--;
	}
	if (timeout == 0)
		return MV_BAD_STATE;
Error:
	return errCode;
}

static MV_STATUS mvNfcDeviceModeSet(MV_NFC_CTRL *nfcCtrl, MV_NFC_ONFI_MODE mode)
{
	MV_STATUS	ret;
	MV_U32		d0 = 0, d1 = 0;

	if (mode == MV_NFC_ONFI_MODE_3) {
		/* Switch to faster mode */
		ret = mvNfcDeviceFeatureSet(nfcCtrl, 0xEF, 0x01, 0x03, 0);
		if (ret != MV_OK)
			return ret;

		/* Verify mode setting */
		mvNfcDeviceFeatureGet(nfcCtrl, 0xEE, 0x01, &d0, &d1);
		if (d0 != 3)
			return MV_BAD_VALUE;
	} else
		return MV_FAIL;

	return MV_OK;
}

static MV_STATUS mvNfcTimingSet(MV_U32 tclk, MV_NFC_FLASH_INFO *flInfo)
{
	MV_U32 reg, i;
	MV_U32 clk2ns;
	MV_U32 trc, trp, trh, twc, twp, twh;
	MV_U32 tadl_nfc, tch_nfc, tcs_nfc, twh_nfc, twp_nfc, trh_nfc, trp_nfc;
	MV_U32 tr_nfc, trhw_nfc, twhr_nfc, tar_nfc;
	MV_U32 tr_pre_nfc = 0;
	MV_U32 ret = MV_OK;

	switch (tclk) {
	case 125000000:
		clk2ns = 8;
		break;
	case 166666667:
		clk2ns = 6;
		break;
	case 200000000:
		clk2ns = 5;
		break;
	case 250000000:
		clk2ns = 4;
		break;
	default:
		return MV_FAIL;
	};

	/* Calculate legal read timing */
	trc = ns_clk(flInfo->tRC, clk2ns);
	trp = ns_clk(flInfo->tRP, clk2ns);
	trh = ns_clk(flInfo->tRH, clk2ns);
	if (trc > (trp + trh))
		trh = (trc - trp);

	/* Calculate legal write timing */
	twc = ns_clk(flInfo->tWC, clk2ns);
	twp = ns_clk(flInfo->tWP, clk2ns);
	twh = ns_clk(flInfo->tWH, clk2ns);
	if (twc > (twp + twh))
		twh = (twc - twp);

	/* Calculate the timing configurations for register0 */
	tadl_nfc = (ns_clk(flInfo->tADL, clk2ns) - maxx(ns_clk(flInfo->tCH, clk2ns), twh));
	tch_nfc = (ns_clk(flInfo->tCH, clk2ns) - 1);
	tcs_nfc = (ns_clk(flInfo->tCS, clk2ns) - twp - 1);
	twh_nfc = (twh - 1);
	twp_nfc = (twp - 1);
	trh_nfc = (trh - 1);
	trp_nfc = (trp - 1);

	if (check_limit(tadl_nfc, 5) != tadl_nfc) {
		ret = MV_OUT_OF_RANGE;
		tadl_nfc = check_limit(tadl_nfc, 5);
	}

	if (check_limit(tch_nfc, 3) != tch_nfc) {
		ret = MV_OUT_OF_RANGE;
		tch_nfc = check_limit(tch_nfc, 3);
	}

	if (check_limit(tcs_nfc, 3) != tcs_nfc) {
		ret = MV_OUT_OF_RANGE;
		tcs_nfc = check_limit(tcs_nfc, 3);
	}

	if (check_limit(twh_nfc, 3) != twh_nfc) {
		ret = MV_OUT_OF_RANGE;
		twh_nfc = check_limit(twh_nfc, 3);
	}

	if (check_limit(twp_nfc, 3) != twp_nfc) {
		ret = MV_OUT_OF_RANGE;
		twp_nfc = check_limit(twp_nfc, 3);
	}

	if (check_limit(trh_nfc, 3) != trh_nfc) {
		ret = MV_OUT_OF_RANGE;
		trh_nfc = check_limit(trh_nfc, 3);
	}

	if (check_limit(trp_nfc, 4) != trp_nfc) {
		ret = MV_OUT_OF_RANGE;
		trp_nfc = check_limit(trp_nfc, 4);
	}

	reg =  ((tadl_nfc << NFC_TMNG0_TADL_OFFS) | \
		(0x1 << NFC_TMNG0_SEL_CNTR_OFFS) | \
		(0x4 << NFC_TMNG0_RD_CNT_DEL_OFFS) | \
		(tch_nfc << NFC_TMNG0_TCH_OFFS) | \
		(tcs_nfc << NFC_TMNG0_TCS_OFFS) | \
		(twh_nfc << NFC_TMNG0_TWH_OFFS) | \
		(twp_nfc << NFC_TMNG0_TWP_OFFS) | \
		(0x0 << NFC_TMNG0_SEL_NRE_EDGE_OFFS) | \
		((trp_nfc >> 3) << NFC_TMNG0_ETRP_OFFS) | \
		(trh_nfc << NFC_TMNG0_TRH_OFFS) | \
		((trp_nfc & 0x7) << NFC_TMNG0_TRP_OFFS));
	MV_REG_WRITE(NFC_TIMING_0_REG, reg);

	/* Calculate the timing configurations for register1 */
	tr_nfc = (ns_clk(flInfo->tR, clk2ns) - tch_nfc - 3);
	trhw_nfc = (ns_clk(flInfo->tRHW, clk2ns) % 16) ? ((ns_clk(flInfo->tRHW, clk2ns) / 16) + 1) : (ns_clk(flInfo->tRHW, clk2ns) / 16);

	/*
	 * For simplicity Assuming that tar == twhr
	 * loop over all 16 possible values of tWHR_NFC and find smallest possible value (if possible!!!)
	 */
	twhr_nfc = 17; /* big number */
	for (i=0; i<16; i++) {
		if ( (maxx(twh_nfc, tch_nfc) + maxx (i, maxx(0, i - maxx(twh_nfc, tch_nfc))) + 2) >= ns_clk(flInfo->tWHR, clk2ns))
			if (twhr_nfc > i)
				twhr_nfc = i;
	}

	if (twhr_nfc >= 16) {
		twhr_nfc = 15; /* worst case - best we can do */
		ret = MV_OUT_OF_RANGE;
	}

	tar_nfc = twhr_nfc; /* our initial assumption */

	if (tr_nfc > 0xFFFF) {
		tr_pre_nfc = 1;
		tr_nfc = ((tr_nfc % 16) ? ((tr_nfc/16) + 1) : (tr_nfc/16));
	}

#ifndef MTD_NAND_NFC_NEGLECT_RNB
	/* If RnBx signal is used, then override tR to a very small and legal value */
	tr_nfc = 0xFF;
	tr_pre_nfc = 0;
#endif

	if (check_limit(tr_nfc, 16) != tr_nfc) {
		ret = MV_OUT_OF_RANGE;
		tr_nfc = check_limit(tr_nfc, 16);
	}

	if (check_limit(trhw_nfc, 2) != trhw_nfc) {
		ret = MV_OUT_OF_RANGE;
		trhw_nfc = check_limit(trhw_nfc, 2);
	}

	if (check_limit(twhr_nfc, 4) != twhr_nfc) {
		ret = MV_OUT_OF_RANGE;
		twhr_nfc = check_limit(twhr_nfc, 4);
	}

	if (check_limit(tar_nfc, 4) != tar_nfc) {
		ret = MV_OUT_OF_RANGE;
		tar_nfc = check_limit(tar_nfc, 5);
	}

	reg = ((tr_nfc << NFC_TMNG1_TR_OFFS) | \
		(tr_pre_nfc << NFC_TMNG1_PRESCALE_OFFS) | \
		(trhw_nfc << NFC_TMNG1_TRHW_OFFS) | \
		(twhr_nfc << NFC_TMNG1_TWHR_OFFS) | \
		(tar_nfc << NFC_TMNG1_TAR_OFFS));
#ifndef MTD_NAND_NFC_NEGLECT_RNB
	reg |= (0x1 << NFC_TMNG1_WAIT_MODE_OFFS);
#endif
	MV_REG_WRITE(NFC_TIMING_1_REG, reg);

	return MV_OK;
}

MV_STATUS mvNfcInit(MV_NFC_INFO *nfcInfo, MV_NFC_CTRL *nfcCtrl)
{
	MV_U32 ctrl_reg;
	MV_STATUS ret;
	MV_U16 read_id = 0;
	MV_U32 i;
	MV_U32 nand_clock;
	/* Initial register values */
	ctrl_reg = 0;

	/*
	 Reduce NAND clock for supporting slower flashes for initialization
	 ECC engine clock = (2Ghz / divider)
	 NFC clock = ECC clock / 2
	 */
	setNANDClock(8); /* Go down to 125MHz */
	nand_clock = 125000000;

	/* Relax Timing configurations to avoid timing violations after flash reset */
	MV_REG_WRITE(NFC_TIMING_0_REG, 0xFC3F3F7F);
	MV_REG_WRITE(NFC_TIMING_1_REG, 0x00FF83FF);

	/* make sure ECC is disabled at this point - will be enabled only when issuing certain commands */
	MV_REG_BIT_RESET(NFC_CONTROL_REG, NFC_CTRL_ECC_EN_MASK);
	if (nfcInfo->eccMode != MV_NFC_ECC_HAMMING)
		MV_REG_BIT_RESET(NFC_ECC_CONTROL_REG, NFC_ECC_BCH_EN_MASK);

	if ((nfcInfo->eccMode == MV_NFC_ECC_BCH_1K) ||
	    (nfcInfo->eccMode == MV_NFC_ECC_BCH_704B) || (nfcInfo->eccMode == MV_NFC_ECC_BCH_512B))
		/* Disable spare */
		ctrl_reg &= ~NFC_CTRL_SPARE_EN_MASK;
	else
		/* Enable spare */
		ctrl_reg |= NFC_CTRL_SPARE_EN_MASK;

	ctrl_reg &= ~NFC_CTRL_ECC_EN_MASK;

	/* Configure flash interface */
	if (nfcInfo->ifMode == MV_NFC_IF_1X16) {
		nfcCtrl->flashWidth = 16;
		nfcCtrl->dfcWidth = 16;
		ctrl_reg |= (NFC_CTRL_DWIDTH_M_MASK | NFC_CTRL_DWIDTH_C_MASK);
	} else if (nfcInfo->ifMode == MV_NFC_IF_2X8) {
		nfcCtrl->flashWidth = 8;
		nfcCtrl->dfcWidth = 16;
		ctrl_reg |= NFC_CTRL_DWIDTH_C_MASK;
	} else {
		nfcCtrl->flashWidth = 8;
		nfcCtrl->dfcWidth = 8;
	}

	/* Configure initial READ-ID byte count */
	ctrl_reg |= (0x2 << NFC_CTRL_RD_ID_CNT_OFFS);

	/* Configure the Arbiter */
	ctrl_reg |= NFC_CTRL_ND_ARB_EN_MASK;

	/* Write registers before device detection */
	MV_REG_WRITE(NFC_CONTROL_REG, ctrl_reg);

#ifdef MTD_NAND_NFC_INIT_RESET
	/* reset the device */
	ret = mvNfcReset();
	if (ret != MV_OK)
		return ret;
#endif

	/* Read the device ID */
	ret = mvNfcReadIdNative(nfcCtrl->currCs, &read_id);
	if (ret != MV_OK)
		return ret;

	/* Look for device ID in knwon device table */
	for (i = 0; i < (sizeof(flashDeviceInfo) / sizeof(MV_NFC_FLASH_INFO)); i++) {
		if (flashDeviceInfo[i].id == read_id)
			break;
	}
	if (i == (sizeof(flashDeviceInfo) / sizeof(MV_NFC_FLASH_INFO)))
		return MV_NOT_SUPPORTED;
	else
		nfcCtrl->flashIdx = i;

	/* In case of ONFI Mode set needed */
	if (flashDeviceInfo[i].flags & NFC_FLAGS_ONFI_MODE_3_SET) {
		ret = mvNfcDeviceModeSet(nfcCtrl, MV_NFC_ONFI_MODE_3);
		if (ret != MV_OK)
			return ret;
	}

#if 0
	/* Critical Initialization done. Raise NFC clock if needed */
	if (flashDeviceInfo[i].flags & NFC_CLOCK_UPSCALE_200M) {
		setNANDClock(5);
		nand_clock = 200000000;
	}
#endif

	/* Configure the command set based on page size */
	if (flashDeviceInfo[i].pgSz < MV_NFC_2KB_PAGE)
		nfcCtrl->cmdsetIdx = MV_NFC_FLASH_SP_CMD_SET_IDX;
	else
		nfcCtrl->cmdsetIdx = MV_NFC_FLASH_LP_CMD_SET_IDX;

#if 0
	/* calculate Timing parameters */
	ret = mvNfcTimingSet(nand_clock, &flashDeviceInfo[i]);
	if (ret != MV_OK)
		return ret;
#endif

	/* Configure the control register based on the device detected */
	ctrl_reg = MV_REG_READ(NFC_CONTROL_REG);

	/* Configure DMA */
	if (nfcInfo->ioMode == MV_NFC_PDMA_ACCESS)
		ctrl_reg |= NFC_CTRL_DMA_EN_MASK;
	else
		ctrl_reg &= ~NFC_CTRL_DMA_EN_MASK;

	/* Configure Page size */
	ctrl_reg &= ~NFC_CTRL_PAGE_SZ_MASK;
	switch (flashDeviceInfo[i].pgSz) {
	case MV_NFC_512B_PAGE:
		ctrl_reg |= NFC_CTRL_PAGE_SZ_512B;
		break;

	case MV_NFC_2KB_PAGE:
	case MV_NFC_4KB_PAGE:
	case MV_NFC_8KB_PAGE:
		ctrl_reg |= NFC_CTRL_PAGE_SZ_2KB;
		break;

	default:
		return MV_BAD_PARAM;
	}

	/* Disable sequential read if indicated */
	if (flashDeviceInfo[i].seqDis)
		ctrl_reg |= NFC_CTRL_SEQ_DIS_MASK;
	else
		ctrl_reg &= ~NFC_CTRL_SEQ_DIS_MASK;

	/* Configure the READ-ID count and row address start based on page size */
	ctrl_reg &= ~(NFC_CTRL_RD_ID_CNT_MASK | NFC_CTRL_RA_START_MASK);
	if (flashDeviceInfo[i].pgSz >= MV_NFC_2KB_PAGE) {
		ctrl_reg |= NFC_CTRL_RD_ID_CNT_LP;
		ctrl_reg |= NFC_CTRL_RA_START_MASK;
	} else {
		ctrl_reg |= NFC_CTRL_RD_ID_CNT_SP;
	}

	/* Confiugre pages per block */
	ctrl_reg &= ~NFC_CTRL_PG_PER_BLK_MASK;
	switch (flashDeviceInfo[i].pgPrBlk) {
	case 32:
		ctrl_reg |= NFC_CTRL_PG_PER_BLK_32;
		break;

	case 64:
		ctrl_reg |= NFC_CTRL_PG_PER_BLK_64;
		break;

	case 128:
		ctrl_reg |= NFC_CTRL_PG_PER_BLK_128;
		break;

	case 256:
		ctrl_reg |= NFC_CTRL_PG_PER_BLK_256;
		break;

	default:
		return MV_BAD_PARAM;
	}

	/* Write the updated control register */
	MV_REG_WRITE(NFC_CONTROL_REG, ctrl_reg);

#ifdef MV_INCLUDE_PDMA
	/* DMA resource allocation */
	if (nfcInfo->ioMode == MV_NFC_PDMA_ACCESS) {
		/* Allocate command buffer */
		nfcCtrl->cmdBuff.bufVirtPtr =
		     mvOsIoUncachedMalloc(nfcInfo->osHandle, (NFC_CMD_STRUCT_SIZE * MV_NFC_MAX_DESC_CHAIN),
					  &nfcCtrl->cmdBuff.bufPhysAddr, &nfcCtrl->cmdBuff.memHandle);
		if (nfcCtrl->cmdBuff.bufVirtPtr == NULL)
			return MV_OUT_OF_CPU_MEM;
		nfcCtrl->cmdBuff.bufSize = (NFC_CMD_STRUCT_SIZE * MV_NFC_MAX_DESC_CHAIN);
		nfcCtrl->cmdBuff.dataSize = (NFC_CMD_STRUCT_SIZE * MV_NFC_MAX_DESC_CHAIN);

		/* Allocate command DMA descriptors */
		nfcCtrl->cmdDescBuff.bufVirtPtr =
		     mvOsIoUncachedMalloc(nfcInfo->osHandle, (MV_PDMA_DESC_SIZE * (MV_NFC_MAX_DESC_CHAIN + 1)),
					  &nfcCtrl->cmdDescBuff.bufPhysAddr, &nfcCtrl->cmdDescBuff.memHandle);
		if (nfcCtrl->cmdDescBuff.bufVirtPtr == NULL)
			return MV_OUT_OF_CPU_MEM;
		/* verify allignment to 128bits */
		if ((MV_U32) nfcCtrl->cmdDescBuff.bufVirtPtr & 0xF) {
			nfcCtrl->cmdDescBuff.bufVirtPtr =
			    (MV_U8 *) (((MV_U32) nfcCtrl->cmdDescBuff.bufVirtPtr & ~0xF) + MV_PDMA_DESC_SIZE);
			nfcCtrl->cmdDescBuff.bufPhysAddr =
			    ((nfcCtrl->cmdDescBuff.bufPhysAddr & ~0xF) + MV_PDMA_DESC_SIZE);
		}
		nfcCtrl->cmdDescBuff.bufSize = (MV_PDMA_DESC_SIZE * MV_NFC_MAX_DESC_CHAIN);
		nfcCtrl->cmdDescBuff.dataSize = (MV_PDMA_DESC_SIZE * MV_NFC_MAX_DESC_CHAIN);

		/* Allocate data DMA descriptors */
		nfcCtrl->dataDescBuff.bufVirtPtr =
		     mvOsIoUncachedMalloc(nfcInfo->osHandle, (MV_PDMA_DESC_SIZE * (MV_NFC_MAX_DESC_CHAIN + 1)),
					  &nfcCtrl->dataDescBuff.bufPhysAddr,
					  &nfcCtrl->dataDescBuff.memHandle);
		if (nfcCtrl->dataDescBuff.bufVirtPtr == NULL)
			return MV_OUT_OF_CPU_MEM;
		/* verify allignment to 128bits */
		if ((MV_U32) nfcCtrl->dataDescBuff.bufVirtPtr & 0xF) {
			nfcCtrl->dataDescBuff.bufVirtPtr =
			    (MV_U8 *) (((MV_U32) nfcCtrl->dataDescBuff.bufVirtPtr & ~0xF) + MV_PDMA_DESC_SIZE);
			nfcCtrl->dataDescBuff.bufPhysAddr =
			    ((nfcCtrl->dataDescBuff.bufPhysAddr & ~0xF) + MV_PDMA_DESC_SIZE);
		}
		nfcCtrl->dataDescBuff.bufSize = (MV_PDMA_DESC_SIZE * MV_NFC_MAX_DESC_CHAIN);
		nfcCtrl->dataDescBuff.dataSize = (MV_PDMA_DESC_SIZE * MV_NFC_MAX_DESC_CHAIN);

		/* Allocate Data DMA channel */
		if (mvPdmaChanAlloc(MV_PDMA_NAND_DATA, nfcInfo->dataPdmaIntMask, &nfcCtrl->dataChanHndl) != MV_OK)
			return MV_NO_RESOURCE;

		/* Allocate Command DMA channel */
		if (mvPdmaChanAlloc(MV_PDMA_NAND_COMMAND, nfcInfo->cmdPdmaIntMask, &nfcCtrl->cmdChanHndl) != MV_OK)
			return MV_NO_RESOURCE;
	}
#endif

	/* Initialize remaining fields in the CTRL structure */
	nfcCtrl->autoStatusRead = nfcInfo->autoStatusRead;
	nfcCtrl->readyBypass = nfcInfo->readyBypass;
	nfcCtrl->ioMode = nfcInfo->ioMode;
	nfcCtrl->eccMode = nfcInfo->eccMode;
	nfcCtrl->ifMode = nfcInfo->ifMode;
	nfcCtrl->currCs = MV_NFC_CS_NONE;
	nfcCtrl->regsPhysAddr = nfcInfo->regsPhysAddr;
#ifdef MV_INCLUDE_PDMA
	nfcCtrl->dataPdmaIntMask = nfcInfo->dataPdmaIntMask;
	nfcCtrl->cmdPdmaIntMask = nfcInfo->cmdPdmaIntMask;
#endif

	return MV_OK;
}


/***********************************************************/

static int prepare_read_prog_cmd(struct orion_nfc_info *info,
			int column, int page_addr)
{
	uint32_t size;

	if (mvNfcFlashPageSizeGet(&info->nfcCtrl, &size, &info->data_size) != 0)
		return -EINVAL;

	return 0;
}
int orion_nfc_wait_for_completion_timeout(struct orion_nfc_info *info, int timeout)
{
	return wait_for_completion_timeout(&info->cmd_complete, timeout);

}

#ifdef CONFIG_MV_INCLUDE_PDMA
static void orion_nfc_data_dma_irq(int irq, void *data)
{
	struct orion_nfc_info *info = data;
	uint32_t dcsr, intr;
	int channel = info->nfcCtrl.dataChanHndl.chanNumber;

	intr = MV_REG_READ(PDMA_INTR_CAUSE_REG);
	dcsr = MV_REG_READ(PDMA_CTRL_STATUS_REG(channel));
	MV_REG_WRITE(PDMA_CTRL_STATUS_REG(channel), dcsr);

	NFC_DPRINT((PRINT_LVL "orion_nfc_data_dma_irq(0x%x, 0x%x) - 1.\n", dcsr, intr));

	if(info->chained_cmd) {
		if (dcsr & DCSR_BUSERRINTR) {
			info->retcode = ERR_DMABUSERR;
			complete(&info->cmd_complete);
		}
		if ((info->state == STATE_DMA_READING) && (dcsr & DCSR_ENDINTR)) {
			info->state = STATE_READY;
			complete(&info->cmd_complete);
		}
		return;
	}

	if (dcsr & DCSR_BUSERRINTR) {
		info->retcode = ERR_DMABUSERR;
		complete(&info->cmd_complete);
	}

	if (info->state == STATE_DMA_WRITING) {
		info->state = STATE_DMA_DONE;
		mvNfcIntrSet(&info->nfcCtrl,  MV_NFC_STATUS_BBD | MV_NFC_STATUS_RDY , true);
	} else {
		info->state = STATE_READY;
		complete(&info->cmd_complete);
	}

	return;
}
#endif

static irqreturn_t orion_nfc_irq_pio(int irq, void *devid)
{
	struct orion_nfc_info *info = devid;

	/* Disable all interrupts */
	mvNfcIntrSet(&info->nfcCtrl, 0xFFF, false);

	/* Clear the interrupt and pass the status UP */
	info->dscr = MV_REG_READ(NFC_STATUS_REG);
	NFC_DPRINT((PRINT_LVL ">>> orion_nfc_irq_pio(0x%x)\n", info->dscr));
	MV_REG_WRITE(NFC_STATUS_REG, info->dscr);
	complete(&info->cmd_complete);

	return IRQ_HANDLED;
}
	
#ifdef CONFIG_MV_INCLUDE_PDMA
static irqreturn_t orion_nfc_irq_dma(int irq, void *devid)
{
	struct orion_nfc_info *info = devid;
	unsigned int status;

	status = MV_REG_READ(NFC_STATUS_REG);

	NFC_DPRINT((PRINT_LVL "orion_nfc_irq_dma(0x%x) - 1.\n", status));

	if(!info->chained_cmd) {
		if (status & (NFC_SR_RDDREQ_MASK | NFC_SR_UNCERR_MASK)) {
			if (status & NFC_SR_UNCERR_MASK)
				info->retcode = ERR_DBERR;
			mvNfcIntrSet(&info->nfcCtrl, NFC_SR_RDDREQ_MASK | NFC_SR_UNCERR_MASK, false);
			if (info->use_dma) {
				info->state = STATE_DMA_READING;
				mvNfcReadWrite(&info->nfcCtrl, info->cmd, (uint32_t*)info->data_buff, info->data_buff_phys);
			} else {
				info->state = STATE_PIO_READING;
				complete(&info->cmd_complete);
			}
		} else if (status & NFC_SR_WRDREQ_MASK) {
			mvNfcIntrSet(&info->nfcCtrl, NFC_SR_WRDREQ_MASK, false);
			if (info->use_dma) {
				info->state = STATE_DMA_WRITING;
				NFC_DPRINT((PRINT_LVL "Calling mvNfcReadWrite().\n"));
				if (mvNfcReadWrite(&info->nfcCtrl, info->cmd,
						   (uint32_t *)info->data_buff,
						   info->data_buff_phys) != 0)
					printk(KERN_ERR "mvNfcReadWrite() failed.\n");
			} else {
				info->state = STATE_PIO_WRITING;
				complete(&info->cmd_complete);
			}
		} else if (status & (NFC_SR_BBD_MASK | MV_NFC_CS0_CMD_DONE_INT |
				     NFC_SR_RDY0_MASK | MV_NFC_CS1_CMD_DONE_INT |
				     NFC_SR_RDY1_MASK)) {
			if (status & NFC_SR_BBD_MASK)
				info->retcode = ERR_BBD;
			mvNfcIntrSet(&info->nfcCtrl,  MV_NFC_STATUS_BBD |
					MV_NFC_STATUS_CMDD | MV_NFC_STATUS_RDY,
					false);
			info->state = STATE_READY;
			complete(&info->cmd_complete);
		}
	} else if (status & (NFC_SR_BBD_MASK | NFC_SR_RDY0_MASK |
				NFC_SR_RDY1_MASK | NFC_SR_UNCERR_MASK)) {
		if (status & (NFC_SR_BBD_MASK | NFC_SR_UNCERR_MASK))
			info->retcode = ERR_DBERR;
		mvNfcIntrSet(&info->nfcCtrl, MV_NFC_STATUS_BBD |
				MV_NFC_STATUS_RDY | MV_NFC_STATUS_CMDD,
				false);
		if ((info->state != STATE_DMA_READING) ||
		    (info->retcode == ERR_DBERR)) {
			info->state = STATE_READY;
			complete(&info->cmd_complete);
		}
	}
	MV_REG_WRITE(NFC_STATUS_REG, status);
	return IRQ_HANDLED;
}
#endif

static int orion_nfc_cmd_prepare(struct orion_nfc_info *info,
		MV_NFC_MULTI_CMD *descInfo, u32 *numCmds)
{
	uint32_t	i;
	MV_NFC_MULTI_CMD *currDesc;	

	currDesc = descInfo;
	if (info->cmd == MV_NFC_CMD_READ_MONOLITHIC) {
		/* Main Chunks */
		for (i=0; i<CHUNK_CNT; i++)
		{
			if (i == 0)
				currDesc->cmd = MV_NFC_CMD_READ_MONOLITHIC;
			else if ((i == (CHUNK_CNT-1)) && (LST_CHUNK_SZ == 0) && (LST_CHUNK_SPR == 0))
				currDesc->cmd = MV_NFC_CMD_READ_LAST_NAKED;
			else
				currDesc->cmd = MV_NFC_CMD_READ_NAKED;

			currDesc->pageAddr = info->page_addr;
			currDesc->pageCount = 1;
			currDesc->virtAddr = (uint32_t *)(info->data_buff + (i * CHUNK_SZ));
			currDesc->physAddr = info->data_buff_phys + (i * CHUNK_SZ);
			currDesc->length = (CHUNK_SZ + CHUNK_SPR);

			if (CHUNK_SPR == 0)
				currDesc->numSgBuffs = 1;
			else
			{
				currDesc->numSgBuffs = 2;
				currDesc->sgBuffAddr[0] = (info->data_buff_phys + (i * CHUNK_SZ));
				currDesc->sgBuffAddrVirt[0] = (uint32_t *)(info->data_buff + (i * CHUNK_SZ));
				currDesc->sgBuffSize[0] = CHUNK_SZ;
				currDesc->sgBuffAddr[1] = (info->data_buff_phys + (CHUNK_SZ * CHUNK_CNT) + LST_CHUNK_SZ + (i * CHUNK_SPR));
				currDesc->sgBuffAddrVirt[1] = (uint32_t *)(info->data_buff + (CHUNK_SZ * CHUNK_CNT) + LST_CHUNK_SZ + (i * CHUNK_SPR));
				currDesc->sgBuffSize[1] = CHUNK_SPR;
			}

			currDesc++;
		}
		
		/* Last chunk if existing */
		if ((LST_CHUNK_SZ != 0) || (LST_CHUNK_SPR != 0))
		{
			currDesc->cmd = MV_NFC_CMD_READ_LAST_NAKED;
			currDesc->pageAddr = info->page_addr;
			currDesc->pageCount = 1;				
			currDesc->length = (LST_CHUNK_SPR + LST_CHUNK_SZ);

			if ((LST_CHUNK_SZ == 0) && (LST_CHUNK_SPR != 0))	/* Spare only */
			{
				currDesc->virtAddr = (uint32_t *)(info->data_buff + (CHUNK_SZ * CHUNK_CNT) + LST_CHUNK_SZ + (CHUNK_SPR * CHUNK_CNT));
				currDesc->physAddr = info->data_buff_phys + (CHUNK_SZ * CHUNK_CNT) + LST_CHUNK_SZ + (CHUNK_SPR * CHUNK_CNT);
				currDesc->numSgBuffs = 1;
				currDesc->length = LST_CHUNK_SPR;
			}
			else if ((LST_CHUNK_SZ != 0) && (LST_CHUNK_SPR == 0))	/* Data only */
			{
				currDesc->virtAddr = (uint32_t *)(info->data_buff + (CHUNK_SZ * CHUNK_CNT));
				currDesc->physAddr = info->data_buff_phys + (CHUNK_SZ * CHUNK_CNT);
				currDesc->numSgBuffs = 1;
				currDesc->length = LST_CHUNK_SZ;
			}
			else /* Both spare and data */
			{
				currDesc->numSgBuffs = 2;
				currDesc->sgBuffAddr[0] = (info->data_buff_phys + (CHUNK_SZ * CHUNK_CNT));
				currDesc->sgBuffAddrVirt[0] = (uint32_t *)(info->data_buff + (CHUNK_SZ * CHUNK_CNT));
				currDesc->sgBuffSize[0] = LST_CHUNK_SZ;
				currDesc->sgBuffAddr[1] = (info->data_buff_phys + (CHUNK_SZ * CHUNK_CNT) + LST_CHUNK_SZ + (CHUNK_SPR * CHUNK_CNT));
				currDesc->sgBuffAddrVirt[1] =  (uint32_t *)(info->data_buff + (CHUNK_SZ * CHUNK_CNT) + LST_CHUNK_SZ + (CHUNK_SPR * CHUNK_CNT));
				currDesc->sgBuffSize[1] = LST_CHUNK_SPR;
			}
			currDesc++;
		}

		*numCmds = CHUNK_CNT + (((LST_CHUNK_SZ) || (LST_CHUNK_SPR)) ? 1 : 0);
	} else if (info->cmd == MV_NFC_CMD_WRITE_MONOLITHIC) {
		/* Write Dispatch */
		currDesc->cmd = MV_NFC_CMD_WRITE_DISPATCH_START;
		currDesc->pageAddr = info->page_addr;
		currDesc->pageCount = 1;
		currDesc->numSgBuffs = 1;
		currDesc->length = 0;
		currDesc++;

		/* Main Chunks */
		for (i=0; i<CHUNK_CNT; i++)
		{
			currDesc->cmd = MV_NFC_CMD_WRITE_NAKED;
			currDesc->pageAddr = info->page_addr;
			currDesc->pageCount = 1;
			currDesc->virtAddr = (uint32_t *)(info->data_buff + (i * CHUNK_SZ));
			currDesc->physAddr = info->data_buff_phys + (i * CHUNK_SZ);
			currDesc->length = (CHUNK_SZ + CHUNK_SPR);

			if (CHUNK_SPR == 0)
				currDesc->numSgBuffs = 1;
			else
			{
				currDesc->numSgBuffs = 2;
				currDesc->sgBuffAddr[0] = (info->data_buff_phys + (i * CHUNK_SZ));
				currDesc->sgBuffAddrVirt[0] = (uint32_t *)(info->data_buff + (i * CHUNK_SZ));
				currDesc->sgBuffSize[0] = CHUNK_SZ;
				currDesc->sgBuffAddr[1] = (info->data_buff_phys + (CHUNK_SZ * CHUNK_CNT) + LST_CHUNK_SZ + (i * CHUNK_SPR));
				currDesc->sgBuffAddrVirt[1] = (uint32_t *)(info->data_buff + (CHUNK_SZ * CHUNK_CNT) + LST_CHUNK_SZ + (i * CHUNK_SPR));
				currDesc->sgBuffSize[1] = CHUNK_SPR;
			}

			currDesc++;
		}
		
		/* Last chunk if existing */
		if ((LST_CHUNK_SZ != 0) || (LST_CHUNK_SPR != 0))
		{
			currDesc->cmd = MV_NFC_CMD_WRITE_NAKED;
			currDesc->pageAddr = info->page_addr;
			currDesc->pageCount = 1;
			currDesc->length = (LST_CHUNK_SZ + LST_CHUNK_SPR);

			if ((LST_CHUNK_SZ == 0) && (LST_CHUNK_SPR != 0))	/* Spare only */
			{
				currDesc->virtAddr = (uint32_t *)(info->data_buff + (CHUNK_SZ * CHUNK_CNT) + LST_CHUNK_SZ + (CHUNK_SPR * CHUNK_CNT));
				currDesc->physAddr = info->data_buff_phys + (CHUNK_SZ * CHUNK_CNT) + LST_CHUNK_SZ + (CHUNK_SPR * CHUNK_CNT);
				currDesc->numSgBuffs = 1;
			}
			else if ((LST_CHUNK_SZ != 0) && (LST_CHUNK_SPR == 0))	/* Data only */
			{
				currDesc->virtAddr = (uint32_t *)(info->data_buff + (CHUNK_SZ * CHUNK_CNT));
				currDesc->physAddr = info->data_buff_phys + (CHUNK_SZ * CHUNK_CNT);
				currDesc->numSgBuffs = 1;
			}
			else /* Both spare and data */
			{
				currDesc->numSgBuffs = 2;
				currDesc->sgBuffAddr[0] = (info->data_buff_phys + (CHUNK_SZ * CHUNK_CNT));
				currDesc->sgBuffAddrVirt[0] = (uint32_t *)(info->data_buff + (CHUNK_SZ * CHUNK_CNT));
				currDesc->sgBuffSize[0] = LST_CHUNK_SZ;
				currDesc->sgBuffAddr[1] = (info->data_buff_phys + (CHUNK_SZ * CHUNK_CNT) + LST_CHUNK_SZ + (CHUNK_SPR * CHUNK_CNT));
				currDesc->sgBuffAddrVirt[1] = (uint32_t *)(info->data_buff + (CHUNK_SZ * CHUNK_CNT) + LST_CHUNK_SZ + (CHUNK_SPR * CHUNK_CNT));
				currDesc->sgBuffSize[1] = LST_CHUNK_SPR;
			}
			currDesc++;
		}

		/* Write Dispatch END */
		currDesc->cmd = MV_NFC_CMD_WRITE_DISPATCH_END;
		currDesc->pageAddr = info->page_addr;
		currDesc->pageCount = 1;
		currDesc->numSgBuffs = 1;
		currDesc->length = 0;

		*numCmds = CHUNK_CNT + (((LST_CHUNK_SZ) || (LST_CHUNK_SPR)) ? 1 : 0) + 2;
	} else {
		descInfo[0].cmd = info->cmd;
		descInfo[0].pageAddr = info->page_addr;
		descInfo[0].pageCount = 1;
		descInfo[0].virtAddr = (uint32_t *)info->data_buff;
		descInfo[0].physAddr = info->data_buff_phys;
		descInfo[0].numSgBuffs = 1;
		descInfo[0].length = info->data_size;
		*numCmds = 1;
	}

	return 0;
}

#ifdef CONFIG_MV_INCLUDE_PDM
static int orion_nfc_do_cmd_dma(struct orion_nfc_info *info,
		uint32_t event)
{
	uint32_t ndcr;
	int ret, timeout = CHIP_DELAY_TIMEOUT;
	int status;
	uint32_t	numCmds;

	/* static allocation to avoid stack overflow*/
	static MV_NFC_MULTI_CMD descInfo[NFC_MAX_NUM_OF_DESCR];

	/* Clear all status bits. */
	MV_REG_WRITE(NFC_STATUS_REG, NFC_SR_MASK);

	mvNfcIntrSet(&info->nfcCtrl, event, true);

	NFC_DPRINT((PRINT_LVL "\nAbout to issue dma cmd %d (cs %d) - 0x%x.\n",
				info->cmd, info->nfcCtrl.currCs,
				MV_REG_READ(NFC_CONTROL_REG)));
	if ((info->cmd == MV_NFC_CMD_READ_MONOLITHIC) ||
	    (info->cmd == MV_NFC_CMD_READ_ID) ||
	    (info->cmd == MV_NFC_CMD_READ_STATUS))
		info->state = STATE_DMA_READING;
	else
		info->state = STATE_CMD_HANDLE;
	info->chained_cmd = 1;

	orion_nfc_cmd_prepare(info, descInfo, &numCmds);

	status = mvNfcCommandMultiple(&info->nfcCtrl,descInfo, numCmds);
	if (status != 0) {
		printk(KERN_ERR "nfcCmdMultiple() failed for cmd %d (%d).\n",
				info->cmd, status);
		goto fail;
	}

	NFC_DPRINT((PRINT_LVL "After issue command %d - 0x%x.\n",
				info->cmd, MV_REG_READ(NFC_STATUS_REG)));

	ret = orion_nfc_wait_for_completion_timeout(info, timeout);
	if (!ret) {
		printk(KERN_ERR "Cmd %d execution timed out (0x%x) - cs %d.\n",
				info->cmd, MV_REG_READ(NFC_STATUS_REG),
				info->nfcCtrl.currCs);
		info->retcode = ERR_CMD_TO;
		goto fail_stop;
	}

	mvNfcIntrSet(&info->nfcCtrl, event | MV_NFC_STATUS_CMDD, false);

	while (MV_PDMA_CHANNEL_STOPPED !=
			mvPdmaChannelStateGet(&info->nfcCtrl.dataChanHndl)) {
		if (info->retcode == ERR_NONE)
			BUG();

	}

	return 0;

fail_stop:
	ndcr = MV_REG_READ(NFC_CONTROL_REG);
	MV_REG_WRITE(NFC_CONTROL_REG, ndcr & ~NFC_CTRL_ND_RUN_MASK);
	udelay(10);
fail:
	return -ETIMEDOUT;
}
#endif

static int orion_nfc_error_check(struct orion_nfc_info *info)
{
	switch (info->cmd) {
		case MV_NFC_CMD_ERASE:
		case MV_NFC_CMD_MULTIPLANE_ERASE:
		case MV_NFC_CMD_WRITE_MONOLITHIC:
		case MV_NFC_CMD_WRITE_MULTIPLE:
		case MV_NFC_CMD_WRITE_NAKED:
		case MV_NFC_CMD_WRITE_LAST_NAKED:
		case MV_NFC_CMD_WRITE_DISPATCH:
		case MV_NFC_CMD_WRITE_DISPATCH_START:
		case MV_NFC_CMD_WRITE_DISPATCH_END:
			if (info->dscr & (MV_NFC_CS0_BAD_BLK_DETECT_INT | MV_NFC_CS1_BAD_BLK_DETECT_INT)) {
				info->retcode = ERR_BBD;
				return 1;
			}
			break;
		
		case MV_NFC_CMD_CACHE_READ_SEQ:
		case MV_NFC_CMD_CACHE_READ_RAND:
		case MV_NFC_CMD_EXIT_CACHE_READ:
		case MV_NFC_CMD_CACHE_READ_START:
		case MV_NFC_CMD_READ_MONOLITHIC:
		case MV_NFC_CMD_READ_MULTIPLE:
		case MV_NFC_CMD_READ_NAKED:
		case MV_NFC_CMD_READ_LAST_NAKED:
		case MV_NFC_CMD_READ_DISPATCH:
			if (info->dscr & MV_NFC_UNCORR_ERR_INT) {
				info->dscr = ERR_DBERR;
				return 1;
			}
			break;

		default:
			break;
	}

	info->retcode = ERR_NONE;
	return 0;
}

/* ==================================================================================================
 *           STEP  1		|   STEP  2   |   STEP  3   |   STEP  4   |   STEP  5   |   STEP 6
 *           COMMAND		|   WAIT FOR  |   CHK ERRS  |     PIO     |   WAIT FOR  |   CHK ERRS
 * =========================|=============|=============|=============|=============|============
 *   READ MONOLITHIC		|   RDDREQ    |   UNCERR    |    READ     |     NONE    |    NONE
 *   READ NAKED				|   RDDREQ    |   UNCERR    |    READ     |     NONE    |    NONE
 *   READ LAST NAKED		|   RDDREQ    |   UNCERR    |    READ     |     NONE    |    NONE
 *   WRITE MONOLITHIC		|   WRDREQ    |    NONE     |    WRITE    |     RDY     |    BBD
 *   WRITE DISPATCH START	|   CMDD      |    NONE     |    NONE     |     NONE    |    NONE
 *   WRITE NAKED			|   WRDREQ    |    NONE     |    WRITE    |     PAGED   |    NONE
 *   WRITE DISPATCH END		|   NONE      |    NONE     |    NONE     |     RDY     |    BBD
 *   ERASE					|   NONE      |    NONE     |    NONE     |     RDY     |    BBD
 *   READ ID				|   RDDREQ    |    NONE     |    READ     |     NONE    |    NONE
 *   READ STAT				|   RDDREQ    |    NONE     |    READ     |     NONE    |    NONE
 *   RESET					|   NONE      |    NONE     |    NONE     |     RDY     |    NONE
 */
static int orion_nfc_do_cmd_pio(struct orion_nfc_info *info)
{
	int timeout = CHIP_DELAY_TIMEOUT;
	int status;
	uint32_t	i, j, numCmds;
	uint32_t ndcr;

	/* static allocation to avoid stack overflow */
	static MV_NFC_MULTI_CMD descInfo[NFC_MAX_NUM_OF_DESCR];

	/* Clear all status bits */
	MV_REG_WRITE(NFC_STATUS_REG, NFC_SR_MASK);	
	
	NFC_DPRINT((PRINT_LVL "\nStarting PIO command %d (cs %d) - NDCR=0x%08x\n",
				info->cmd, info->nfcCtrl.currCs, MV_REG_READ(NFC_CONTROL_REG)));

	/* Build the chain of commands */
	orion_nfc_cmd_prepare(info, descInfo, &numCmds);
	NFC_DPRINT((PRINT_LVL "Prepared %d commands in sequence\n", numCmds));

	/* Execute the commands */
	for (i=0; i < numCmds; i++) {
		/* Verify that command is supported in PIO mode */
		if ((orion_nfc_cmd_info_lkup[descInfo[i].cmd].events_p1 == 0) &&
		    (orion_nfc_cmd_info_lkup[descInfo[i].cmd].events_p2 == 0)) {
			goto fail_stop;
		}
		
		/* clear the return code */
		info->dscr = 0;

		/* STEP1: Initiate the command */
		NFC_DPRINT((PRINT_LVL "About to issue Descriptor #%d (command %d, pageaddr 0x%x, length %d).\n", 
			    i, descInfo[i].cmd, descInfo[i].pageAddr, descInfo[i].length));
		if ((status = mvNfcCommandPio(&info->nfcCtrl, &descInfo[i], false)) != 0) {
			printk(KERN_ERR "mvNfcCommandPio() failed for command %d (%d).\n", descInfo[i].cmd, status);
			goto fail_stop;
		}
		NFC_DPRINT((PRINT_LVL "After issue command %d (NDSR=0x%x)\n", descInfo[i].cmd, MV_REG_READ(NFC_STATUS_REG)));
	
		/* Check if command phase interrupts events are needed */
		if (orion_nfc_cmd_info_lkup[descInfo[i].cmd].events_p1) {
			/* Enable necessary interrupts for command phase */
			NFC_DPRINT((PRINT_LVL "Enabling part1 interrupts (IRQs 0x%x)\n", orion_nfc_cmd_info_lkup[descInfo[i].cmd].events_p1));
			mvNfcIntrSet(&info->nfcCtrl, orion_nfc_cmd_info_lkup[descInfo[i].cmd].events_p1, true);	
			
			/* STEP2: wait for interrupt */
			if (!orion_nfc_wait_for_completion_timeout(info, timeout)) {
				printk(KERN_ERR "command %d execution timed out (CS %d, NDCR=0x%x, NDSR=0x%x).\n",
				       descInfo[i].cmd, info->nfcCtrl.currCs, MV_REG_READ(NFC_CONTROL_REG), MV_REG_READ(NFC_STATUS_REG));
				info->retcode = ERR_CMD_TO;
				goto fail_stop;
			}
		
			/* STEP3: Check for errors */
			if (orion_nfc_error_check(info)) {
				NFC_DPRINT((PRINT_LVL "Command level errors (DSCR=%08x, retcode=%d)\n", info->dscr, info->retcode));
				goto fail_stop;
			}
		}		
				
		/* STEP4: PIO Read/Write data if needed */
		if (descInfo[i].numSgBuffs > 1)
		{
			for (j=0; j< descInfo[i].numSgBuffs; j++) {
				NFC_DPRINT((PRINT_LVL "Starting SG#%d PIO Read/Write (%d bytes, R/W mode %d)\n", j, 
					    descInfo[i].sgBuffSize[j], orion_nfc_cmd_info_lkup[descInfo[i].cmd].rw));
				mvNfcReadWritePio(&info->nfcCtrl, descInfo[i].sgBuffAddrVirt[j], 
						  descInfo[i].sgBuffSize[j], orion_nfc_cmd_info_lkup[descInfo[i].cmd].rw);
			}
		}
		else {
			NFC_DPRINT((PRINT_LVL "Starting nonSG PIO Read/Write (%d bytes, R/W mode %d)\n", 
				    descInfo[i].length, orion_nfc_cmd_info_lkup[descInfo[i].cmd].rw));
			mvNfcReadWritePio(&info->nfcCtrl, descInfo[i].virtAddr, 
					  descInfo[i].length, orion_nfc_cmd_info_lkup[descInfo[i].cmd].rw);
		}

		/* check if data phase events are needed */
		if (orion_nfc_cmd_info_lkup[descInfo[i].cmd].events_p2) {
			/* Enable the RDY interrupt to close the transaction */
			NFC_DPRINT((PRINT_LVL "Enabling part2 interrupts (IRQs 0x%x)\n", orion_nfc_cmd_info_lkup[descInfo[i].cmd].events_p2));
			mvNfcIntrSet(&info->nfcCtrl, orion_nfc_cmd_info_lkup[descInfo[i].cmd].events_p2, true);			

			/* STEP5: Wait for transaction to finish */
			if (!orion_nfc_wait_for_completion_timeout(info, timeout)) {
				printk(KERN_ERR "command %d execution timed out (NDCR=0x%08x, NDSR=0x%08x, NDECCCTRL=0x%08x)\n", descInfo[i].cmd, 
						MV_REG_READ(NFC_CONTROL_REG), MV_REG_READ(NFC_STATUS_REG), MV_REG_READ(NFC_ECC_CONTROL_REG));
				info->retcode = ERR_DATA_TO;
				goto fail_stop;
			}
		
			/* STEP6: Check for errors BB errors (in erase) */
			if (orion_nfc_error_check(info)) {
				NFC_DPRINT((PRINT_LVL "Data level errors (DSCR=0x%08x, retcode=%d)\n", info->dscr, info->retcode));
				goto fail_stop;
			}
		}
	
		/* Fallback - in case the NFC did not reach the idle state */
		ndcr = MV_REG_READ(NFC_CONTROL_REG);
		if (ndcr & NFC_CTRL_ND_RUN_MASK) {
			printk(KERN_DEBUG "WRONG NFC STAUS: command %d, NDCR=0x%08x, NDSR=0x%08x, NDECCCTRL=0x%08x)\n", 
		       	info->cmd, MV_REG_READ(NFC_CONTROL_REG), MV_REG_READ(NFC_STATUS_REG), MV_REG_READ(NFC_ECC_CONTROL_REG));
			MV_REG_WRITE(NFC_CONTROL_REG, (ndcr & ~NFC_CTRL_ND_RUN_MASK));
		}
	}

	NFC_DPRINT((PRINT_LVL "Command done (NDCR=0x%08x, NDSR=0x%08x)\n", MV_REG_READ(NFC_CONTROL_REG), MV_REG_READ(NFC_STATUS_REG)));
	info->retcode = ERR_NONE;	
	
	return 0;

fail_stop:
	ndcr = MV_REG_READ(NFC_CONTROL_REG);
	if (ndcr & NFC_CTRL_ND_RUN_MASK) {
		printk(KERN_ERR "WRONG NFC STAUS: command %d, NDCR=0x%08x, NDSR=0x%08x, NDECCCTRL=0x%08x)\n", 
		       info->cmd, MV_REG_READ(NFC_CONTROL_REG), MV_REG_READ(NFC_STATUS_REG), MV_REG_READ(NFC_ECC_CONTROL_REG));
		MV_REG_WRITE(NFC_CONTROL_REG, (ndcr & ~NFC_CTRL_ND_RUN_MASK));
	}
	mvNfcIntrSet(&info->nfcCtrl, 0xFFF, false);
	udelay(10);
	return -ETIMEDOUT;
}

static int orion_nfc_dev_ready(struct mtd_info *mtd)
{
	//printk(KERN_INFO "%s:%4d:%s mtd=%p stsreg=%08x rdy=%08x\n", __FILE__, __LINE__, __FUNCTION__, mtd,
	//MV_REG_READ(NFC_STATUS_REG),
	//(MV_REG_READ(NFC_STATUS_REG) & (NFC_SR_RDY0_MASK | NFC_SR_RDY1_MASK))
	//);
	return (MV_REG_READ(NFC_STATUS_REG) & (NFC_SR_RDY0_MASK | NFC_SR_RDY1_MASK)) ? 1 : 0;
}

static inline int is_buf_blank(uint8_t *buf, size_t len)
{
	for (; len > 0; len--)
		if (*buf++ != 0xff)
			return 0;
	return 1;
}

static void orion_nfc_cmdfunc(struct mtd_info *mtd, unsigned command,
				int column, int page_addr)
{
	struct orion_nfc_info *info = (struct orion_nfc_info *)((struct nand_chip *)mtd->priv)->priv;
#if 0
	int ret = 0;
#endif

	//printk(KERN_INFO "%s:%4d:%s mtd=%p cmd=%d, col=%d, addr=%08x\n", __FILE__, __LINE__, __FUNCTION__, mtd, command, column, page_addr);
	info->data_size = 0;
	info->state = STATE_READY;
	info->chained_cmd = 0;
	info->retcode = ERR_NONE;

	init_completion(&info->cmd_complete);

	switch (command) {
	case NAND_CMD_READOOB:
		info->buf_count = mtd->writesize + mtd->oobsize;
		info->buf_start = mtd->writesize + column;
		info->cmd = MV_NFC_CMD_READ_MONOLITHIC;
		info->column = column;
		info->page_addr = page_addr;
		if (prepare_read_prog_cmd(info, column, page_addr))
			break;

		if (info->use_dma)
#ifdef CONFIG_MV_INCLUDE_PDM
			orion_nfc_do_cmd_dma(info, MV_NFC_STATUS_RDY | NFC_SR_UNCERR_MASK);
#else
			printk(KERN_ERR "DMA mode not supported!\n");
#endif
		else
			orion_nfc_do_cmd_pio(info);

		/* We only are OOB, so if the data has error, does not matter */
		if (info->retcode == ERR_DBERR)
			info->retcode = ERR_NONE;
		break;

	case NAND_CMD_READ0:
		info->buf_start = column;
		info->buf_count = mtd->writesize + mtd->oobsize;
		memset(info->data_buff, 0xff, info->buf_count);
		info->cmd = MV_NFC_CMD_READ_MONOLITHIC;
		info->column = column;
		info->page_addr = page_addr;

		if (prepare_read_prog_cmd(info, column, page_addr))
			break;

		if (info->use_dma)
#ifdef CONFIG_MV_INCLUDE_PDM
			orion_nfc_do_cmd_dma(info, MV_NFC_STATUS_RDY | NFC_SR_UNCERR_MASK);
#else
			printk(KERN_ERR "DMA mode not supported!\n");
#endif
		else
			orion_nfc_do_cmd_pio(info);

		if (info->retcode == ERR_DBERR) {
			/* for blank page (all 0xff), HW will calculate its ECC as
			 * 0, which is different from the ECC information within
			 * OOB, ignore such double bit errors
			 */
			if (is_buf_blank(info->data_buff, mtd->writesize))
				info->retcode = ERR_NONE;
			else
				printk(PRINT_LVL "%s: retCode == ERR_DBERR\n", __FUNCTION__);
		}
		break;
	case NAND_CMD_SEQIN:
		info->buf_start = column;
		info->buf_count = mtd->writesize + mtd->oobsize;
		memset(info->data_buff + mtd->writesize, 0xff, mtd->oobsize);

		/* save column/page_addr for next CMD_PAGEPROG */
		info->seqin_column = column;
		info->seqin_page_addr = page_addr;
		break;
	case NAND_CMD_PAGEPROG:
		info->column = info->seqin_column;
		info->page_addr = info->seqin_page_addr;
		info->cmd = MV_NFC_CMD_WRITE_MONOLITHIC;
		if (prepare_read_prog_cmd(info,
				info->seqin_column, info->seqin_page_addr)) {
			printk(KERN_ERR "prepare_read_prog_cmd() failed.\n");
			break;
		}
	
		if (info->use_dma)
#ifdef CONFIG_MV_INCLUDE_PDM
			orion_nfc_do_cmd_dma(info, MV_NFC_STATUS_RDY);
#else
			printk(KERN_ERR "DMA mode not supported!\n");
#endif
		else
			orion_nfc_do_cmd_pio(info);

		break;
	case NAND_CMD_ERASE1:
		info->column = 0;
		info->page_addr = page_addr;
		info->cmd = MV_NFC_CMD_ERASE;

		if (info->use_dma)
#ifdef CONFIG_MV_INCLUDE_PDM
			orion_nfc_do_cmd_dma(info, MV_NFC_STATUS_BBD | MV_NFC_STATUS_RDY);
#else
			printk(KERN_ERR "DMA mode not supported!\n");
#endif
		else
			orion_nfc_do_cmd_pio(info);

		break;
	case NAND_CMD_ERASE2:
		break;
	case NAND_CMD_READID:
	case NAND_CMD_STATUS:
		info->buf_start = 0;
		info->buf_count = (command == NAND_CMD_READID) ?
				info->read_id_bytes : 1;
		info->data_size = 8;
		info->column = 0;
		info->page_addr = 0;
		info->cmd = (command == NAND_CMD_READID) ?
			MV_NFC_CMD_READ_ID : MV_NFC_CMD_READ_STATUS;

		if (info->use_dma)
#ifdef CONFIG_MV_INCLUDE_PDM
			orion_nfc_do_cmd_dma(info,MV_NFC_STATUS_RDY);
#else
			printk(KERN_ERR "DMA mode not supported!\n");
#endif
		else
			orion_nfc_do_cmd_pio(info);

		break;
	case NAND_CMD_RESET:
#if 0
		info->column = 0;
		info->page_addr = 0;
		info->cmd = MV_NFC_CMD_RESET;

		if (info->use_dma)
#ifdef CONFIG_MV_INCLUDE_PDM
			ret = orion_nfc_do_cmd_dma(info, MV_NFC_STATUS_CMDD);
#else
			printk(KERN_ERR "DMA mode not supported!\n");
#endif
		else
			ret = orion_nfc_do_cmd_pio(info);

		if (ret == 0) {
			int timeout = 2;
			uint32_t ndcr;

			while (timeout--) {
				if (MV_REG_READ(NFC_STATUS_REG) & (NFC_SR_RDY0_MASK | NFC_SR_RDY1_MASK))
					break;
				msleep(10);
			}

			ndcr = MV_REG_READ(NFC_CONTROL_REG);
			MV_REG_WRITE(NFC_CONTROL_REG, ndcr & ~NFC_CTRL_ND_RUN_MASK);
		}
#else
		if (mvNfcReset() != 0)
			printk(KERN_ERR "Device reset failed.\n");
#endif
		break;
	default:
		printk(KERN_ERR "non-supported command.\n");
		break;
	}

	if (info->retcode == ERR_DBERR) {
		printk(KERN_ERR "double bit error @ page %08x (%d)\n",
				page_addr, info->cmd);
		info->retcode = ERR_NONE;
	}
}

static uint8_t orion_nfc_read_byte(struct mtd_info *mtd)
{
	struct orion_nfc_info *info = (struct orion_nfc_info *)((struct nand_chip *)mtd->priv)->priv;
	char retval = 0xFF;

	if (info->buf_start < info->buf_count)
		/* Has just send a new command? */
		retval = info->data_buff[info->buf_start++];
	//printk(KERN_INFO "%s:%4d:%s mtd=%p data_buff=%p bf_start=%08x ret=%08x\n", __FILE__, __LINE__, __FUNCTION__, mtd, info->data_buff, info->buf_start, retval);
	return retval;
}

static u16 orion_nfc_read_word(struct mtd_info *mtd)
{
	struct orion_nfc_info *info = (struct orion_nfc_info *)((struct nand_chip *)mtd->priv)->priv;
	u16 retval = 0xFFFF;

	if (!(info->buf_start & 0x01) && info->buf_start < info->buf_count) {
		retval = *((u16 *)(info->data_buff+info->buf_start));
		info->buf_start += 2;
	}
	else
		printk(KERN_ERR "\n%s: returning 0xFFFF (%d, %d).\n",__FUNCTION__, info->buf_start,info->buf_count);

	//printk(KERN_INFO "%s:%4d:%s mtd=%p data_buff=%p bf_start=%08x ret=%08x\n", __FILE__, __LINE__, __FUNCTION__, mtd, info->data_buff, info->buf_start, retval);
	return retval;
}

static void orion_nfc_read_buf(struct mtd_info *mtd, uint8_t *buf, int len)
{
	struct orion_nfc_info *info = (struct orion_nfc_info *)((struct nand_chip *)mtd->priv)->priv;
	int real_len = min_t(size_t, len, info->buf_count - info->buf_start);

	//printk(KERN_INFO "%s:%4d:%s mtd=%p buf=%p len=%d\n", __FILE__, __LINE__, __FUNCTION__, mtd, buf, len);
	memcpy(buf, info->data_buff + info->buf_start, real_len);
	info->buf_start += real_len;
}

static void orion_nfc_write_buf(struct mtd_info *mtd,
		const uint8_t *buf, int len)
{
	struct orion_nfc_info *info = (struct orion_nfc_info *)((struct nand_chip *)mtd->priv)->priv;
	int real_len = min_t(size_t, len, info->buf_count - info->buf_start);

	//printk(KERN_INFO "%s:%4d:%s mtd=%p buf=%p len=%d\n", __FILE__, __LINE__, __FUNCTION__, mtd, buf, len);
	memcpy(info->data_buff + info->buf_start, buf, real_len);
	info->buf_start += real_len;
}

static void orion_nfc_select_chip(struct mtd_info *mtd, int chip)
{
	struct orion_nfc_info *info = (struct orion_nfc_info *)((struct nand_chip *)mtd->priv)->priv;

	//printk(KERN_INFO "%s:%4d:%s mtd=%p chip=%d info=%p\n", __FILE__, __LINE__, __FUNCTION__, mtd, chip, info);
	mvNfcSelectChip(&info->nfcCtrl, MV_NFC_CS_0 + chip);
	return;
}

static int orion_nfc_waitfunc(struct mtd_info *mtd, struct nand_chip *this)
{
	struct orion_nfc_info *info = (struct orion_nfc_info *)((struct nand_chip *)mtd->priv)->priv;

	//printk(KERN_INFO "%s:%4d:%s mtd=%p this->state=%d\n", __FILE__, __LINE__, __FUNCTION__, mtd, this->state);
	/* orion_nfc_send_command has waited for command complete */
	if (this->state == FL_WRITING || this->state == FL_ERASING) {
		if (info->retcode == ERR_NONE)
			return 0;
		else {
			/*
			 * any error make it return 0x01 which will tell
			 * the caller the erase and write fail
			 */
			return 0x01;
		}
	}

	return 0;
}

static void orion_nfc_ecc_hwctl(struct mtd_info *mtd, int mode)
{
	return;
}

static int orion_nfc_ecc_calculate(struct mtd_info *mtd,
		const uint8_t *dat, uint8_t *ecc_code)
{
	return 0;
}

static int orion_nfc_ecc_correct(struct mtd_info *mtd,
		uint8_t *dat, uint8_t *read_ecc, uint8_t *calc_ecc)
{
	struct orion_nfc_info *info = (struct orion_nfc_info *)((struct nand_chip *)mtd->priv)->priv;
	/*
	 * Any error include ERR_SEND_CMD, ERR_DBERR, ERR_BUSERR, we
	 * consider it as a ecc error which will tell the caller the
	 * read fail We have distinguish all the errors, but the
	 * nand_read_ecc only check this function return value
	 */
	if (info->retcode != ERR_NONE)
		return -1;

	return 0;
}

static int orion_nfc_detect_flash(struct orion_nfc_info *info)
{
	uint32_t my_page_size;

	mvNfcFlashPageSizeGet(&info->nfcCtrl, &my_page_size, NULL);

	/* Translate page size to enum */
	switch (my_page_size)
	{
		case 512:
			info->page_size = NFC_PAGE_512B;
			break;

		case 2048:
			info->page_size = NFC_PAGE_2KB;
			break;

		case 4096:
			info->page_size = NFC_PAGE_4KB;
			break;
		
		case 8192:
			info->page_size = NFC_PAGE_8KB;
			break;

		case 16384:
			info->page_size = NFC_PAGE_16KB;
			break;

		default:
			return -EINVAL;
	}

	info->flash_width = info->nfc_width;
	if (info->flash_width != 16 && info->flash_width != 8)
		return -EINVAL;

	/* calculate flash information */
	info->read_id_bytes = (pg_sz[info->page_size] >= 2048) ? 4 : 2;

	return 0;
}

/* the maximum possible buffer size for ganaged 8K page with OOB data
 * is: 2 * (8K + Spare) ==> to be alligned allocate 5 MMU (4K) pages
 */
#define MAX_BUFF_SIZE	(PAGE_SIZE * 5)

static int orion_nfc_init_buff(struct orion_nfc_info *info)
{
	struct platform_device *pdev = info->pdev;

	if (info->use_dma == 0) {
		info->data_buff = kmalloc(MAX_BUFF_SIZE, GFP_KERNEL);
		if (info->data_buff == NULL)
			return -ENOMEM;
		return 0;
	}

	info->data_buff = dma_alloc_coherent(&pdev->dev, MAX_BUFF_SIZE,
				&info->data_buff_phys, GFP_KERNEL);
	if (info->data_buff == NULL) {
		dev_err(&pdev->dev, "failed to allocate dma buffer\n");
		return -ENOMEM;
	}
	memset(info->data_buff, 0xff, MAX_BUFF_SIZE);

#ifdef CONFIG_MV_INCLUDE_PDM
	if (pxa_request_dma_intr ("nand-data", info->nfcCtrl.dataChanHndl.chanNumber,
			orion_nfc_data_dma_irq, info) < 0) {
		dev_err(&pdev->dev, "failed to request PDMA IRQ\n");
		return -ENOMEM;
	}
#endif
	return 0;
}

static uint8_t mv_bbt_pattern[] = {'M', 'V', 'B', 'b', 't', '0' };
static uint8_t mv_mirror_pattern[] = {'1', 't', 'b', 'B', 'V', 'M' };

static struct nand_bbt_descr mvbbt_main_descr = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
		| NAND_BBT_2BIT | NAND_BBT_VERSION,
	.offs =	8,
	.len = 6,
	.veroffs = 14,
	.maxblocks = 8,		/* Last 8 blocks in each chip */
	.pattern = mv_bbt_pattern
};

static struct nand_bbt_descr mvbbt_mirror_descr = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
		| NAND_BBT_2BIT | NAND_BBT_VERSION,
	.offs =	8,
	.len = 6,
	.veroffs = 14,
	.maxblocks = 8,		/* Last 8 blocks in each chip */
	.pattern = mv_mirror_pattern
};


static int orion_nfc_markbad(struct mtd_info *mtd, loff_t ofs)
{
	struct nand_chip *chip = mtd->priv;
	uint8_t buf[6] = {0, 0, 0, 0, 0, 0};
	int block, ret = 0;
	loff_t page_addr;

	//printk(KERN_INFO "%s:%4d:%s mtd=%p ofs=%08x\n", __FILE__, __LINE__, __FUNCTION__, mtd, (int)ofs);

	/* Get block number */
	block = (int)(ofs >> chip->bbt_erase_shift);
	if (chip->bbt)
		chip->bbt[block >> 2] |= 0x01 << ((block & 0x03) << 1);
	ret = nand_update_bbt(mtd, ofs);

	if (ret == 0) {
		/* Get address of the next block */
		ofs += mtd->erasesize;
		ofs &= ~(mtd->erasesize - 1);

		/* Get start of oob in last page */
		ofs -= mtd->oobsize;

		page_addr = ofs;
		do_div(page_addr, mtd->writesize);

		orion_nfc_cmdfunc(mtd, NAND_CMD_SEQIN, mtd->writesize,
				page_addr);
		orion_nfc_write_buf(mtd, buf, 6);
		orion_nfc_cmdfunc(mtd, NAND_CMD_PAGEPROG, 0, page_addr);
	}

	return ret;
}


static void orion_nfc_init_nand(struct nand_chip *nand, struct orion_nfc_info *info)
{
	
	nand->options 	= 0;
	if (info->nfc_width == 16)
		nand->options 	|= NAND_BUSWIDTH_16;
	nand->bbt_options 	= NAND_BBT_USE_FLASH;
#ifdef CONFIG_MTD_NAND_ARMADA_GANG_SUPPORT 
	nand->num_devs		= info->num_devs;
#endif
#ifdef CONFIG_MTD_NAND_ARMADA_MLC_SUPPORT
	nand->oobsize_ovrd	= ((CHUNK_SPR * CHUNK_CNT) + LST_CHUNK_SPR);
	nand->bb_location	= BB_BYTE_POS;
	nand->bb_page		= mvNfcBadBlockPageNumber(&info->nfcCtrl);
#endif
	nand->waitfunc		= orion_nfc_waitfunc;
	nand->select_chip	= orion_nfc_select_chip;
	nand->dev_ready		= orion_nfc_dev_ready;
	nand->cmdfunc		= orion_nfc_cmdfunc;
	nand->read_word		= orion_nfc_read_word;
	nand->read_byte		= orion_nfc_read_byte;
	nand->read_buf		= orion_nfc_read_buf;
	nand->write_buf		= orion_nfc_write_buf;
	nand->block_markbad	= orion_nfc_markbad;
	nand->ecc.mode		= NAND_ECC_HW;
	nand->ecc.hwctl		= orion_nfc_ecc_hwctl;
	nand->ecc.calculate	= orion_nfc_ecc_calculate;
	nand->ecc.correct	= orion_nfc_ecc_correct;
	nand->ecc.size		= pg_sz[info->page_size];
	nand->ecc.strength      = 1;
	nand->ecc.layout	= ECC_LAYOUT;
	nand->bbt_td 		= &mvbbt_main_descr;
	nand->bbt_md 		= &mvbbt_mirror_descr;
	nand->badblock_pattern	= BB_INFO;
	nand->chip_delay 	= 25;
}

static int orion_nfc_probe(struct platform_device *pdev)
{
	struct mtd_part_parser_data ppdata = {};
	struct nfc_platform_data *pdata;
	struct orion_nfc_info *info;
	struct nand_chip *nand;
	struct mtd_info *mtd;
	//struct resource *r;
	int ret = 0, irq;
	uint32_t val;
	char * stat[2] = {"Disabled", "Enabled"};
	char * ecc_stat[] = {"Hamming", "BCH 4bit", "BCH 8bit", "BCH 12bit", "BCH 16bit", "No"};
	MV_NFC_INFO nfcInfo;

	//printk(KERN_INFO "calling %s:%4d %s\n", __FILE__, __LINE__, __FUNCTION__);

	if (pdev->dev.of_node) {
		pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
		if (!pdata) {
			printk(KERN_ERR "armada_nand: failed to allocate pdata structure.\n");
			return -ENOMEM;
		}
		if (!of_property_read_u32(pdev->dev.of_node, "use_dma", &val))
			pdata->use_dma = val;

		if (!of_property_read_u32(pdev->dev.of_node, "ecc_type", &val))
			pdata->ecc_type = val;
		else
			pdata->ecc_type = MV_NFC_ECC_BCH_2K; /* MV_NFC_ECC_BCH_1K for 8bit-ecc, MV_NFC_ECC_BCH_704B for 12bit-ecc, MV_NFC_ECC_BCH_512B for 16bit-ecc */

		if (!of_property_read_u32(pdev->dev.of_node, "nfc_width", &val))
			pdata->nfc_width = val;
		else
			pdata->nfc_width = 8; /* or 16 in ganged mode */

		if (!of_property_read_u32(pdev->dev.of_node, "num_devs", &val))
			pdata->num_devs = val;
		else
			pdata->num_devs = 1; /* 2 in ganged mode */

		if (!of_property_read_u32(pdev->dev.of_node, "num_cs", &val))
			pdata->num_cs = val;
		else
			pdata->num_cs = 1;

		if (!of_property_read_u32(pdev->dev.of_node, "tclk", &val))
			pdata->tclk = val;
		else
			pdata->tclk = 200000000; /* 200 MHz tclk by default */
	} else
		pdata = dev_get_platdata(&pdev->dev);

	if (!pdata) {
		dev_err(&pdev->dev, "no platform data defined\n");
		return -ENODEV;
	}

	dev_info(&pdev->dev, "Initialize HAL based NFC in %dbit mode with DMA %s using %s ECC\n",
			  pdata->nfc_width, stat[pdata->use_dma], ecc_stat[pdata->ecc_type]);

	/* Allocate all data: mtd_info -> nand_chip -> orion_nfc_info */
	mtd = kzalloc(sizeof(struct mtd_info),	GFP_KERNEL);
	if (!mtd) {
		dev_err(&pdev->dev, "failed to allocate memory for mtd_info\n");
		return -ENOMEM;
	}

	info = kzalloc(sizeof(struct orion_nfc_info), GFP_KERNEL);
	if (!info) {
		dev_err(&pdev->dev, "failed to allocate memory for orion_nfc_info\n");
		return -ENOMEM;
	}

	nand = kzalloc(sizeof(struct nand_chip), GFP_KERNEL);
	if (!nand) {
		dev_err(&pdev->dev, "failed to allocate memory for nand_chip\n");
		return -ENOMEM;
	}

	/* Hookup pointers */
	info->pdev = pdev;
	nand->priv = info;
	mtd->priv = nand;
	mtd->name = DRIVER_NAME;
	mtd->owner = THIS_MODULE;

	/* Copy all necessary information from platform data */
	info->use_dma = pdata->use_dma;
	info->ecc_type = pdata->ecc_type;
	info->nfc_width = pdata->nfc_width;
	info->num_devs = pdata->num_devs;
	info->num_cs = pdata->num_cs;
	info->tclk = pdata->tclk;
#if 0
	/* Get the TCLK */
	info->clk = clk_get_sys("armada-nand", NULL);
	if (IS_ERR(info->clk)) {
		dev_err(&pdev->dev, "failed to get nand clock\n");
		ret = PTR_ERR(info->clk);
		goto fail_free_mtd;
	}
	clk_enable(info->clk);
#endif

	irq = irq_of_parse_and_map(pdev->dev.of_node, 0);
	//printk(KERN_INFO "%s:%4d:%s irq=%d\n", __FILE__, __LINE__, __FUNCTION__, irq);

	if (irq < 0) {
		irq = platform_get_irq(pdev, 0);
		//printk(KERN_INFO "%s:%4d:%s irq=%d\n", __FILE__, __LINE__, __FUNCTION__, irq);
	}
	//printk(KERN_INFO "%s:%4d:%s irq=%d\n", __FILE__, __LINE__, __FUNCTION__, irq);
	if (irq < 0) {
		dev_err(&pdev->dev, "no IRQ resource defined\n");
		ret = -ENXIO;
		goto fail_put_clk;
	}

	info->mmio_base = of_iomap(pdev->dev.of_node, 0);
	//printk(KERN_INFO "%s:%4d:%s mmio_base=%p\n", __FILE__, __LINE__, __FUNCTION__, info->mmio_base);
	if (info->mmio_base == NULL) {
		printk(KERN_INFO "of_iomap() returns NULL\n");
		ret = -ENOMEM;
		goto fail_put_clk;
	}
	//printk(KERN_INFO "%s:%4d:%s base=%08x\n", __FILE__, __LINE__, __FUNCTION__, (unsigned int) info->mmio_base);


	//r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	//if (r == NULL) {
	//	dev_err(&pdev->dev, "no IO memory resource defined\n");
	//	ret = -ENODEV;
	//	goto fail_put_clk;
	//}
	//
	//printk(KERN_INFO "%s:%4d:%s r=%p start=%08x end=%08x irq=%d\n", __FILE__, __LINE__, __FUNCTION__,
	//       r, r->start, r->end, irq);
	//r = devm_request_mem_region(&pdev->dev, r->start, r->end - r->start + 1,
	//			    pdev->name);
	//if (r == NULL) {
	//	dev_err(&pdev->dev, "failed to request memory resource\n");
	//	ret = -EBUSY;
	//	goto fail_put_clk;
	//}
	//printk(KERN_INFO "%s:%4d:%s r=%p start=%08x end=%08x\n", __FILE__, __LINE__, __FUNCTION__,
	//       r, r->start, r->end);
	//
	//info->mmio_base = devm_ioremap(&pdev->dev, r->start,
	//			       r->end - r->start + 1);
	//if (info->mmio_base == NULL) {
	//	dev_err(&pdev->dev, "ioremap() failed\n");
	//	ret = -ENODEV;
	//	goto fail_put_clk;
	//}
	//
	//printk(KERN_INFO "%s:%4d:%s r=%p start=%08x end=%08x\n", __FILE__, __LINE__, __FUNCTION__,
	//       r, r->start, r->end);
	//info->mmio_phys_base = r->start;

	/* we need this access for setNANDClock() */
	//r = devm_request_mem_region(&pdev->dev, INTER_REGS_BASE + /*0x18740*/0x18000, /*16*/4096, pdev->name);
	//if (r) {
	//	printk(KERN_INFO "%s:%4d:%s r=%p start=%p end=%p\n", __FILE__, __LINE__, __FUNCTION__,
	//	       r, r->start, r->end);
	//	r = devm_ioremap(&pdev->dev, r->start, r->end - r->start + 1);
	//}
	//else
	//	dev_err(&pdev->dev, "request_mem_region() failed for 16@0x18740, continuing\n");
	//
	//if (r) {
	//	printk(KERN_INFO "%s:%4d:%s r=%p start=%p end=%p\n", __FILE__, __LINE__, __FUNCTION__,
	//	       r, r->start, r->end);
	//}
	//else
	//	dev_err(&pdev->dev, "ioremap() failed for 16@0x18740, continuing\n");

#ifdef CONFIG_MV_INCLUDE_PDMA
	if (mvPdmaHalInit(MV_PDMA_MAX_CHANNELS_NUM) != 0) {
		dev_err(&pdev->dev, "mvPdmaHalInit() failed.\n");
		goto fail_put_clk;
	}
#endif
	/* Initialize NFC HAL */
	nfcInfo.ioMode = (info->use_dma ? MV_NFC_PDMA_ACCESS : MV_NFC_PIO_ACCESS);
	nfcInfo.eccMode = info->ecc_type;
		
	if(info->num_devs == 1)
		nfcInfo.ifMode = ((info->nfc_width == 8) ? MV_NFC_IF_1X8 : MV_NFC_IF_1X16);
	else
		nfcInfo.ifMode = MV_NFC_IF_2X8;
	nfcInfo.autoStatusRead = false;
	nfcInfo.tclk = info->tclk;
	nfcInfo.readyBypass = false;
	nfcInfo.osHandle = NULL;
	nfcInfo.regsPhysAddr = INTER_REGS_BASE;
#ifdef CONFIG_MV_INCLUDE_PDMA
	nfcInfo.dataPdmaIntMask = MV_PDMA_END_OF_RX_INTR_EN | MV_PDMA_END_INTR_EN;
	nfcInfo.cmdPdmaIntMask = 0x0;
#endif

	if (mvNfcInit(&nfcInfo, &info->nfcCtrl) != 0) {
		dev_err(&pdev->dev, "mvNfcInit() failed.\n");
		goto fail_put_clk;
	}

	mvNfcSelectChip(&info->nfcCtrl, MV_NFC_CS_0);
	mvNfcIntrSet(&info->nfcCtrl,  0xFFF, false);
	mvNfcSelectChip(&info->nfcCtrl, MV_NFC_CS_1);
	mvNfcIntrSet(&info->nfcCtrl,  0xFFF, false);
	mvNfcSelectChip(&info->nfcCtrl, MV_NFC_CS_NONE);

	ret = orion_nfc_init_buff(info);
	if (ret)
		goto fail_put_clk;

	/* Clear all old events on the status register */
	MV_REG_WRITE(NFC_STATUS_REG, MV_REG_READ(NFC_STATUS_REG));
	if (info->use_dma)
#ifdef CONFIG_MV_INCLUDE_PDMA
		ret = request_irq(irq/*IRQ_AURORA_NFC*/, orion_nfc_irq_dma, IRQF_DISABLED,
				pdev->name, info);
#else
		printk(KERN_ERR "DMA mode not supported!\n");
#endif
	else
		ret = request_irq(irq/*IRQ_AURORA_NFC*/, orion_nfc_irq_pio, IRQF_DISABLED,
				pdev->name, info);

	if (ret < 0) {
		dev_err(&pdev->dev, "failed to request IRQ\n");
		goto fail_free_buf;
	}	

	ret = orion_nfc_detect_flash(info);
	if (ret) {
		dev_err(&pdev->dev, "failed to detect flash\n");
		ret = -ENODEV;
		goto fail_free_irq;
	}

	orion_nfc_init_nand(nand, info);

	if (nand->ecc.layout == NULL) {
		dev_err(&pdev->dev, "Undefined ECC layout for selected nand device\n");
		ret = -ENXIO;
		goto fail_free_irq;
	}

	platform_set_drvdata(pdev, mtd);

	if (nand_scan(mtd, info->num_cs)) {
		dev_err(&pdev->dev, "failed to scan nand\n");
		ret = -ENXIO;
		goto fail_free_irq;
	}
	
	ppdata.of_node = pdev->dev.of_node;
	ret = mtd_device_parse_register(mtd, NULL, &ppdata, NULL, 0);
	if (ret) {
		nand_release(mtd);
		goto no_dev;
	}

	return 0;

no_dev:
#if 0
	if (!IS_ERR(info->clk)) {
		clk_disable_unprepare(info->clk);
		clk_put(info->clk);
	}
#endif
	platform_set_drvdata(pdev, NULL);
	iounmap(info->mmio_base);
	kfree(mtd);
	kfree(nand);
	kfree(info);
	return ret;

fail_free_irq:
	free_irq(irq/*IRQ_AURORA_NFC*/, info);
fail_free_buf:
	if (pdata->use_dma) {
		dma_free_coherent(&pdev->dev, info->data_buff_size,
			info->data_buff, info->data_buff_phys);
	} else
		kfree(info->data_buff);
fail_put_clk:
#if 0
	clk_disable(info->clk);
	clk_put(info->clk);
fail_free_mtd:
#endif
	kfree(mtd);
	kfree(nand);
	kfree(info);
	return ret;
}

static int orion_nfc_remove(struct platform_device *pdev)
{
	struct mtd_info *mtd = platform_get_drvdata(pdev);
	struct orion_nfc_info *info = (struct orion_nfc_info *)((struct nand_chip *)mtd->priv)->priv;

	platform_set_drvdata(pdev, NULL);

	/*del_mtd_device(mtd);*/
	free_irq(IRQ_AURORA_NFC, info);
	if (info->use_dma) {
		dma_free_writecombine(&pdev->dev, info->data_buff_size,
				info->data_buff, info->data_buff_phys);
	} else
		kfree(info->data_buff);
#if 0
	clk_disable(info->clk);
	clk_put(info->clk);
#endif
	if (mtd) {
		mtd_device_unregister(mtd);
		kfree(mtd);
	}
	return 0;
}

#ifdef CONFIG_PM
static int orion_nfc_suspend(struct platform_device *pdev, pm_message_t state)
{
#if 0 /* Code dropped because standby WoL does not power off the device */
	struct mtd_info *mtd = (struct mtd_info *)platform_get_drvdata(pdev);
	struct orion_nfc_info *info = (struct orion_nfc_info *)((struct nand_chip *)mtd->priv)->priv;

	if (info->state != STATE_READY) {
		dev_err(&pdev->dev, "driver busy, state = %d\n", info->state);
		return -EAGAIN;
	}
#ifdef CONFIG_MV_INCLUDE_PDMA
	/* Store PDMA registers.	*/
	info->pdmaDataLen = 128;
	mvPdmaUnitStateStore(info->pdmaUnitData, &info->pdmaDataLen);
#endif
	
	/* Store NFC registers.	*/
	info->nfcDataLen = 128;
	mvNfcUnitStateStore(info->nfcUnitData, &info->nfcDataLen);
#if 0
	clk_disable(info->clk);
#endif
#endif
	return 0;
}

static int orion_nfc_resume(struct platform_device *pdev)
{
#if 0  /* Code dropped because standby WoL does not power off the device */
	struct mtd_info *mtd = (struct mtd_info *)platform_get_drvdata(pdev);
	struct orion_nfc_info *info = (struct orion_nfc_info *)((struct nand_chip *)mtd->priv)->priv;
	uint32_t i;
#if 0
	clk_enable(info->clk);
#endif
	/* restore PDMA registers */
	for(i = 0; i < info->pdmaDataLen; i+=2)
		MV_REG_WRITE(info->pdmaUnitData[i], info->pdmaUnitData[i+1]);

	/* Clear all NAND interrupts */
	MV_REG_WRITE(NFC_STATUS_REG, MV_REG_READ(NFC_STATUS_REG));

	/* restore NAND registers */
	for(i = 0; i < info->nfcDataLen; i+=2)
		MV_REG_WRITE(info->nfcUnitData[i], info->nfcUnitData[i+1]);
#endif
	return 0;
}
#else
#define orion_nfc_suspend	NULL
#define orion_nfc_resume	NULL
#endif

#ifdef CONFIG_OF
static struct of_device_id orion_nfc_of_match_table[] = {
	{ .compatible = "marvell,armada-nand", },
	{},
};
#endif

static struct platform_driver orion_nfc_driver = {
	.driver = {
		.name	= DRIVER_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(orion_nfc_of_match_table),
	},
	.probe		= orion_nfc_probe,
	.remove		= orion_nfc_remove,
	.suspend	= orion_nfc_suspend,
	.resume		= orion_nfc_resume,
};

static int __init orion_nfc_init(void)
{
	return platform_driver_register(&orion_nfc_driver);
}
module_init(orion_nfc_init);

static void __exit orion_nfc_exit(void)
{
	platform_driver_unregister(&orion_nfc_driver);
}
module_exit(orion_nfc_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tzachi Perelstein");
MODULE_DESCRIPTION("NAND glue for Orion platforms");
MODULE_ALIAS("platform:orion_nand");




MODULE_ALIAS(DRIVER_NAME);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ArmadaXP NAND controller driver");
