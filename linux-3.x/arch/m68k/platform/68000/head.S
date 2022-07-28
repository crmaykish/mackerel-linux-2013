/*
 * head.S - Common startup code for 68000 core based CPU's
 *
 * 2012.10.21, Luis Alves <ljalvs@gmail.com>, Single head.S file for all
 *             68000 core based CPU's. Based on the sources from:
 *             Coldfire by Greg Ungerer <gerg@snapgear.com>
 *             68328 by D. Jeff Dionne <jeff@ryeham.ee.ryerson.ca>,
 *                      Kenneth Albanowski <kjahds@kjahds.com>,
 *                      The Silver Hammer Group, Ltd.
 *
 */

#include <linux/linkage.h>
#include <linux/init.h>
#include <asm/asm-offsets.h>
#include <asm/thread_info.h>


#define RAMEND (CONFIG_RAMBASE+CONFIG_RAMSIZE)

.global _start
.global _rambase
.global _ramvec
.global _ramstart
.global _ramend

.data

/*****************************************************************************
 * RAM setup pointers. Used by the kernel to determine RAM location and size.
 *****************************************************************************/

_rambase:
	.long	0
_ramvec:
	.long	0
_ramstart:
	.long	0
_ramend:
	.long	0

__HEAD

/*****************************************************************************
 * Entry point, where all begins!
 *****************************************************************************/

_start:
	movew	#0x2700, %sr			/* disable all interrupts */

/*****************************************************************************
 * Setup basic memory information for kernel
 *****************************************************************************/
	movel	#CONFIG_VECTORBASE,_ramvec	/* set vector base location */
	movel	#CONFIG_RAMBASE,_rambase	/* set the base of RAM */
	movel	#RAMEND, _ramend		/* set end ram addr */
	lea	__bss_stop,%a1
	movel	%a1,_ramstart

/*****************************************************************************
 * If the kernel is in RAM, move romfs to right above bss and
 * adjust _ramstart to where romfs ends.
 *
 * (Do this only if CONFIG_MTD_UCLINUX is true)
 *****************************************************************************/

#if defined(CONFIG_ROMFS_FS) && defined(CONFIG_RAMKERNEL) && \
    defined(CONFIG_MTD_UCLINUX)
	lea	__bss_start, %a0		/* get start of bss */
	lea	__bss_stop, %a1			/* set up destination  */
	movel	%a0, %a2			/* copy of bss start */

	movel	8(%a0), %d0			/* get size of ROMFS */
	addql	#8, %d0				/* allow for rounding */
	andl	#0xfffffffc, %d0		/* whole words */

	addl	%d0, %a0			/* copy from end */
	addl	%d0, %a1			/* copy from end */
	movel	%a1, _ramstart			/* set start of ram */
_copy_romfs:
	movel	-(%a0), -(%a1)			/* copy dword */
	cmpl	%a0, %a2			/* check if at end */
	bne	_copy_romfs
#endif /* CONFIG_ROMFS_FS && CONFIG_RAMKERNEL && CONFIG_MTD_UCLINUX */

/*****************************************************************************
 * Clear bss region
 *****************************************************************************/
	lea	__bss_start, %a0		/* get start of bss */
	lea	__bss_stop, %a1			/* get end of bss */
_clear_bss:
	movel	#0, (%a0)+			/* clear each word */
	cmpl	%a0, %a1			/* check if at end */
	bne	_clear_bss

/*****************************************************************************
 * Load the current task pointer and stack.
 *****************************************************************************/
	lea	init_thread_union,%a0
	lea	THREAD_SIZE(%a0),%sp
	jsr	start_kernel			/* start Linux kernel */
_exit:
	jmp	_exit				/* should never get here */
