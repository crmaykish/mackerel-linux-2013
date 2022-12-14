/*
 * r2300.c: R2000 and R3000 specific mmu/cache code.
 *
 * Copyright (C) 1996 David S. Miller (dm@engr.sgi.com)
 *
 * with a lot of changes to make this thing work for R3000s
 * Tx39XX R4k style caches added. HK
 * Copyright (C) 1998, 1999, 2000 Harald Koerfgen
 * Copyright (C) 1998 Gleb Raiko & Vladimir Roganov
 * Copyright (C) 2001, 2004  Maciej W. Rozycki
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/mm.h>

#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/mmu_context.h>
#include <asm/system.h>
#include <asm/isadep.h>
#include <asm/io.h>
#include <asm/bootinfo.h>
#include <asm/cpu.h>

/* =====================================================================
	the MIPS's I-cache/D-cache size discovery is not supported
	by LEXTRA series processors.
   ===================================================================== */
#if defined(CONFIG_RTL865XC) || defined(CONFIG_RTL865XB)
#undef _CACHE_DISCOVERY
#else
#define _CACHE_DISCOVERY 1
#endif

#ifdef _CACHE_DISCOVERY
/* =====================================================================
	MIPS's I-cache/D-cache size discovery is supported.
	No any constant is needed.
   ===================================================================== */
#undef _ICACHE_SIZE					/* I-CACHE size */
#undef _DCACHE_SIZE					/* D-CACHE size */
#undef _CACHE_LINE_SIZE					/* I-CACHE/D-CACHE line size */

#else /* _CACHE_DISCOVERY */
/* =====================================================================
	MIPS's I-cache/D-cache size discovery is NOT supported.
	System should defined I-cache/D-cache size by them-self.
   ===================================================================== */
#ifdef CONFIG_RTL865XC
/* For Realtek RTL865XC Network platform series */
#define _ICACHE_SIZE		(16 * 1024)		/* 16K bytes */
#define _DCACHE_SIZE		(8 * 1024)		/* 8K bytes */
#define _CACHE_LINE_SIZE	4			/* 4 words */
#elif defined (CONFIG_RTL865XB)
/* For Realtek RTL865XB Network platform series */
#define _ICACHE_SIZE	(4 * 1024)			/* 4K bytes */
#define _DCACHE_SIZE	(4 * 1024)			/* 4K bytes */
#define _CACHE_LINE_SIZE	4			/* 4 words */
#else
#error "Least one chip would be selected to decide I-cache/D-cache size"
#endif

#endif /* _CACHE_DISCOVERY */

static unsigned long icache_size, dcache_size;		/* Size in bytes */
static unsigned long icache_lsize, dcache_lsize;	/* Size in bytes */

#undef DEBUG_CACHE

/*
 * 	Removing the paramter "__init" because
 * 	the function would be called by other functions
 * 	and can not be reused.
 */
unsigned long r3k_cache_size(unsigned long ca_flags)
{
#ifdef _CACHE_DISCOVERY

	unsigned long flags, status, dummy, size;
	volatile unsigned long *p;

	p = (volatile unsigned long *) KSEG0;

	flags = read_c0_status();

	/* isolate cache space */
	write_c0_status((ca_flags|flags)&~ST0_IEC);

	*p = 0xa5a55a5a;
	dummy = *p;
	status = read_c0_status();

	if (dummy != 0xa5a55a5a || (status & ST0_CM)) {
		size = 0;
	} else {
		for (size = 128; size <= 0x40000; size <<= 1)
			*(p + size) = 0;
		*p = -1;
		for (size = 128;
		     (size <= 0x40000) && (*(p + size) == 0);
		     size <<= 1)
			;
		if (size > 0x40000)
			size = 0;
	}

	write_c0_status(flags);

	return size * sizeof(*p);

#else /* _CACHE_DISCOVERY */

	unsigned long cacheSize;

	switch (ca_flags)
	{
		case ST0_ISC:
			/* D-cache size */
			cacheSize = _DCACHE_SIZE;
			break;
		case (ST0_ISC|ST0_SWC):
			/* I-cache size */
			cacheSize = _ICACHE_SIZE;
			break;
		default:
			cacheSize = 0;
	}

	return cacheSize;

#endif /* _CACHE_DISCOVERY */
}

/*
 * 	Removing the paramter "__init" because
 * 	the function would be called by other functions
 * 	and can not be reused.
 */
unsigned long r3k_cache_lsize(unsigned long ca_flags)
{
#ifdef _CACHE_DISCOVERY

	unsigned long flags, status, lsize, i;
	volatile unsigned long *p;

	p = (volatile unsigned long *) KSEG0;

	flags = read_c0_status();

	/* isolate cache space */
	write_c0_status((ca_flags|flags)&~ST0_IEC);

	for (i = 0; i < 128; i++)
		*(p + i) = 0;
	*(volatile unsigned char *)p = 0;
	for (lsize = 1; lsize < 128; lsize <<= 1) {
		*(p + lsize);
		status = read_c0_status();
		if (!(status & ST0_CM))
			break;
	}
	for (i = 0; i < 128; i += lsize)
		*(volatile unsigned char *)(p + i) = 0;

	write_c0_status(flags);

	return lsize * sizeof(*p);

#else /* _CACHE_DISCOVERY */

	unsigned long cacheLineSize;

	switch (ca_flags)
	{
		case ST0_ISC:
			/* D-cache Line size */
			cacheLineSize = _CACHE_LINE_SIZE;
			break;
		case (ST0_ISC|ST0_SWC):
			/* I-cache Line size */
			cacheLineSize = _CACHE_LINE_SIZE;
			break;
		default:
			cacheLineSize = 0;
	}

	return cacheLineSize;

#endif /* _CACHE_DISCOVERY */
}

static void __init r3k_probe_cache(void)
{
	dcache_size = r3k_cache_size(ST0_ISC);
	if (dcache_size)
		dcache_lsize = r3k_cache_lsize(ST0_ISC);

	icache_size = r3k_cache_size(ST0_ISC|ST0_SWC);
	if (icache_size)
		icache_lsize = r3k_cache_lsize(ST0_ISC|ST0_SWC);
}

static void r3k_flush_icache_range(unsigned long start, unsigned long end)
{
#if defined(CONFIG_RTL865XC) || defined(CONFIG_RTL865XB)
	/*Invalidate I-Cache*/
	__asm__ volatile(
		"mtc0 $0,$20\n\t"
		"nop\n\t"
		"li $8,2\n\t"
		"mtc0 $8,$20\n\t"
		"nop\n\t"
		"nop\n\t"
		"mtc0 $0,$20\n\t"
		"nop"
		: /* no output */
		: /* no input */
			);
#else
	unsigned long size, i, flags;
	volatile unsigned char *p;

	size = end - start;
	if (size > icache_size || KSEGX(start) != KSEG0) {
		start = KSEG0;
		size = icache_size;
	}
	p = (char *)start;

	flags = read_c0_status();

	/* isolate cache space */
	write_c0_status((ST0_ISC|ST0_SWC|flags)&~ST0_IEC);

	for (i = 0; i < size; i += 0x080) {
		asm ( 	"sb\t$0, 0x000(%0)\n\t"
			"sb\t$0, 0x004(%0)\n\t"
			"sb\t$0, 0x008(%0)\n\t"
			"sb\t$0, 0x00c(%0)\n\t"
			"sb\t$0, 0x010(%0)\n\t"
			"sb\t$0, 0x014(%0)\n\t"
			"sb\t$0, 0x018(%0)\n\t"
			"sb\t$0, 0x01c(%0)\n\t"
		 	"sb\t$0, 0x020(%0)\n\t"
			"sb\t$0, 0x024(%0)\n\t"
			"sb\t$0, 0x028(%0)\n\t"
			"sb\t$0, 0x02c(%0)\n\t"
			"sb\t$0, 0x030(%0)\n\t"
			"sb\t$0, 0x034(%0)\n\t"
			"sb\t$0, 0x038(%0)\n\t"
			"sb\t$0, 0x03c(%0)\n\t"
			"sb\t$0, 0x040(%0)\n\t"
			"sb\t$0, 0x044(%0)\n\t"
			"sb\t$0, 0x048(%0)\n\t"
			"sb\t$0, 0x04c(%0)\n\t"
			"sb\t$0, 0x050(%0)\n\t"
			"sb\t$0, 0x054(%0)\n\t"
			"sb\t$0, 0x058(%0)\n\t"
			"sb\t$0, 0x05c(%0)\n\t"
		 	"sb\t$0, 0x060(%0)\n\t"
			"sb\t$0, 0x064(%0)\n\t"
			"sb\t$0, 0x068(%0)\n\t"
			"sb\t$0, 0x06c(%0)\n\t"
			"sb\t$0, 0x070(%0)\n\t"
			"sb\t$0, 0x074(%0)\n\t"
			"sb\t$0, 0x078(%0)\n\t"
			"sb\t$0, 0x07c(%0)\n\t"
			: : "r" (p) );
		p += 0x080;
	}

	write_c0_status(flags);
#endif	
}

static void r3k_flush_dcache_range(unsigned long start, unsigned long end)
{
#if defined(CONFIG_RTL865XC) || defined(CONFIG_RTL865XB)
	/*Invalidate I-Cache*/
#ifdef CONFIG_RTL865XC
	__asm__ volatile(
		"mtc0 $0,$20\n\t"
		"nop\n\t"
		"li $8,512\n\t"
		"mtc0 $8,$20\n\t"
		"nop\n\t"
		"nop\n\t"
		"mtc0 $0,$20\n\t"
		"nop"
		: /* no output */
		: /* no input */
			);
#else
	__asm__ volatile(
		"mtc0 $0,$20\n\t"
		"nop\n\t"
		"li $8,1\n\t"
		"mtc0 $8,$20\n\t"
		"nop\n\t"
		"nop\n\t"
		"mtc0 $0,$20\n\t"
		"nop"
		: /* no output */
		: /* no input */
			);
#endif
#else /* defined(CONFIG_RTL865XC) || defined(CONFIG_RTL865XB) */
	unsigned long size, i, flags;
	volatile unsigned char *p;

	size = end - start;
	if (size > dcache_size || KSEGX(start) != KSEG0) {
		start = KSEG0;
		size = dcache_size;
	}
	p = (char *)start;

	flags = read_c0_status();

	/* isolate cache space */
	write_c0_status((ST0_ISC|flags)&~ST0_IEC);

	for (i = 0; i < size; i += 0x080) {
		asm ( 	"sb\t$0, 0x000(%0)\n\t"
			"sb\t$0, 0x004(%0)\n\t"
			"sb\t$0, 0x008(%0)\n\t"
			"sb\t$0, 0x00c(%0)\n\t"
		 	"sb\t$0, 0x010(%0)\n\t"
			"sb\t$0, 0x014(%0)\n\t"
			"sb\t$0, 0x018(%0)\n\t"
			"sb\t$0, 0x01c(%0)\n\t"
		 	"sb\t$0, 0x020(%0)\n\t"
			"sb\t$0, 0x024(%0)\n\t"
			"sb\t$0, 0x028(%0)\n\t"
			"sb\t$0, 0x02c(%0)\n\t"
		 	"sb\t$0, 0x030(%0)\n\t"
			"sb\t$0, 0x034(%0)\n\t"
			"sb\t$0, 0x038(%0)\n\t"
			"sb\t$0, 0x03c(%0)\n\t"
		 	"sb\t$0, 0x040(%0)\n\t"
			"sb\t$0, 0x044(%0)\n\t"
			"sb\t$0, 0x048(%0)\n\t"
			"sb\t$0, 0x04c(%0)\n\t"
		 	"sb\t$0, 0x050(%0)\n\t"
			"sb\t$0, 0x054(%0)\n\t"
			"sb\t$0, 0x058(%0)\n\t"
			"sb\t$0, 0x05c(%0)\n\t"
		 	"sb\t$0, 0x060(%0)\n\t"
			"sb\t$0, 0x064(%0)\n\t"
			"sb\t$0, 0x068(%0)\n\t"
			"sb\t$0, 0x06c(%0)\n\t"
		 	"sb\t$0, 0x070(%0)\n\t"
			"sb\t$0, 0x074(%0)\n\t"
			"sb\t$0, 0x078(%0)\n\t"
			"sb\t$0, 0x07c(%0)\n\t"
			: : "r" (p) );
		p += 0x080;
	}

	write_c0_status(flags);
#endif	
}

static inline unsigned long get_phys_page (unsigned long addr,
					   struct mm_struct *mm)
{
	pgd_t *pgd;
	pmd_t *pmd;
	pte_t *pte;
	unsigned long physpage;

	pgd = pgd_offset(mm, addr);
	pmd = pmd_offset(pgd, addr);
	pte = pte_offset(pmd, addr);

	if ((physpage = pte_val(*pte)) & _PAGE_VALID)
		return KSEG0ADDR(physpage & PAGE_MASK);

	return 0;
}

static inline void r3k_flush_cache_all(void)
{
}

static inline void r3k___flush_cache_all(void)
{
	r3k_flush_dcache_range(KSEG0, KSEG0 + dcache_size);
	r3k_flush_icache_range(KSEG0, KSEG0 + icache_size);
}

static void r3k_flush_cache_mm(struct mm_struct *mm)
{
}

static void r3k_flush_cache_range(struct mm_struct *mm, unsigned long start,
				  unsigned long end)
{
}

static void r3k_flush_cache_page(struct vm_area_struct *vma,
				   unsigned long page)
{
}

static void r3k_flush_data_cache_page(unsigned long addr)
{
}

static void r3k_flush_icache_page(struct vm_area_struct *vma, struct page *page)
{
	struct mm_struct *mm = vma->vm_mm;
	unsigned long physpage;

	if (cpu_context(smp_processor_id(), mm) == 0)
		return;

	if (!(vma->vm_flags & VM_EXEC))
		return;

#ifdef DEBUG_CACHE
	printk("cpage[%d,%08lx]", cpu_context(smp_processor_id(), mm), page);
#endif

	physpage = (unsigned long) page_address(page);
	if (physpage)
		r3k_flush_icache_range(physpage, physpage + PAGE_SIZE);
}

static void r3k_flush_cache_sigtramp(unsigned long addr)
{
	unsigned long flags;

#if defined(CONFIG_RTL865XC) || defined(CONFIG_RTL865XB)
	save_and_cli(flags);
    
	r3k___flush_cache_all();
	restore_flags(flags);

#else /* defined(CONFIG_RTL865XC) || defined(CONFIG_RTL865XB) */

#ifdef DEBUG_CACHE
	printk("csigtramp[%08lx]", addr);
#endif

	flags = read_c0_status();

	write_c0_status(flags&~ST0_IEC);

	/* Fill the TLB to avoid an exception with caches isolated. */
	asm ( 	"lw\t$0, 0x000(%0)\n\t"
		"lw\t$0, 0x004(%0)\n\t"
		: : "r" (addr) );

	write_c0_status((ST0_ISC|ST0_SWC|flags)&~ST0_IEC);

	asm ( 	"sb\t$0, 0x000(%0)\n\t"
		"sb\t$0, 0x004(%0)\n\t"
		: : "r" (addr) );

	write_c0_status(flags);
#endif
}

static void r3k_dma_cache_wback_inv(unsigned long start, unsigned long size)
{
	iob();
	r3k_flush_dcache_range(start, start + size);
}

void __init ld_mmu_r23000(void)
{
	extern void build_clear_page(void);
	extern void build_copy_page(void);

	r3k_probe_cache();
#if defined(CONFIG_RTL865XC) || defined(CONFIG_RTL865XB)
	r3k_flush_icache_range(0,0);
#endif

	_flush_cache_all = r3k_flush_cache_all;
	___flush_cache_all = r3k___flush_cache_all;
	_flush_cache_mm = r3k_flush_cache_mm;
	_flush_cache_range = r3k_flush_cache_range;
	_flush_cache_page = r3k_flush_cache_page;
	_flush_icache_page = r3k_flush_icache_page;
	_flush_icache_range = r3k_flush_icache_range;

	_flush_cache_sigtramp = r3k_flush_cache_sigtramp;
	_flush_data_cache_page = r3k_flush_data_cache_page;

	_dma_cache_wback_inv = r3k_dma_cache_wback_inv;
#if defined(CONFIG_RTL865XC) || defined(CONFIG_RTL865XB)
	_dma_cache_wback = r3k_dma_cache_wback_inv;
	_dma_cache_inv = r3k_dma_cache_wback_inv;
#endif

	printk("Primary instruction cache %ldkB, linesize %ld bytes.\n",
		icache_size >> 10, icache_lsize);
	printk("Primary data cache %ldkB, linesize %ld bytes.\n",
		dcache_size >> 10, dcache_lsize);

	build_clear_page();
	build_copy_page();
}
