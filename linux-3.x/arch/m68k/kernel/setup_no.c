/*
 *  linux/arch/m68knommu/kernel/setup.c
 *
 *  Copyright (C) 1999-2007  Greg Ungerer (gerg@snapgear.com)
 *  Copyright (C) 1998,1999  D. Jeff Dionne <jeff@uClinux.org>
 *  Copyleft  ()) 2000       James D. Schettine {james@telos-systems.com}
 *  Copyright (C) 1998       Kenneth Albanowski <kjahds@kjahds.com>
 *  Copyright (C) 1995       Hamish Macdonald
 *  Copyright (C) 2000       Lineo Inc. (www.lineo.com)
 *  Copyright (C) 2001 	     Lineo, Inc. <www.lineo.com>
 *
 *  68VZ328 Fixes/support    Evan Stawnyczy <e@lineo.ca>
 */

/*
 * This file handles the architecture-dependent parts of system setup
 */

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/fb.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/console.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/bootmem.h>
#include <linux/seq_file.h>
#include <linux/init.h>
#include <linux/initrd.h>
#include <linux/root_dev.h>
#include <linux/rtc.h>

#include <asm/setup.h>
#include <asm/irq.h>
#include <asm/machdep.h>
#include <asm/pgtable.h>
#include <asm/sections.h>

unsigned long memory_start;
unsigned long memory_end;

EXPORT_SYMBOL(memory_start);
EXPORT_SYMBOL(memory_end);

char __initdata command_line[COMMAND_LINE_SIZE];

/* machine dependent timer functions */
void (*mach_sched_init)(irq_handler_t handler) __initdata = NULL;
int (*mach_set_clock_mmss)(unsigned long);
int (*mach_hwclk) (int, struct rtc_time*);

/* machine dependent reboot functions */
void (*mach_reset)(void);
void (*mach_halt)(void);
void (*mach_power_off)(void);

#ifdef CONFIG_M68000
#define CPU_NAME	"MC68000"
#endif
#ifdef CONFIG_M68328
#define CPU_NAME	"MC68328"
#endif
#ifdef CONFIG_M68EZ328
#define CPU_NAME	"MC68EZ328"
#endif
#ifdef CONFIG_M68VZ328
#define CPU_NAME	"MC68VZ328"
#endif
#ifdef CONFIG_M68360
#define CPU_NAME	"MC68360"
#endif
#ifndef CPU_NAME
#define	CPU_NAME	"UNKNOWN"
#endif

/*
 * Different cores have different instruction execution timings.
 * The old/traditional 68000 cores are basically all the same, at 16.
 * The ColdFire cores vary a little, their values are defined in their
 * headers. We default to the standard 68000 value here.
 */
#ifndef CPU_INSTR_PER_JIFFY
#define	CPU_INSTR_PER_JIFFY	16
#endif

void __init setup_arch(char **cmdline_p)
{
	int bootmap_size;

	memory_start = PAGE_ALIGN(_ramstart);
	memory_end = _ramend;

	printk("memory_start: %lu, memory_end: %lu\r\n", memory_start, memory_end);

	init_mm.start_code = (unsigned long) &_stext;
	init_mm.end_code = (unsigned long) &_etext;
	init_mm.end_data = (unsigned long) &_edata;
	init_mm.brk = (unsigned long) 0;


	printk("INIT_MM VALS: %lu, %lu, %lu, %lu\r\n", init_mm.start_code, init_mm.end_code, init_mm.end_data, init_mm.brk);


	config_BSP(&command_line[0], sizeof(command_line));

#if defined(CONFIG_BOOTPARAM)
	strncpy(&command_line[0], CONFIG_BOOTPARAM_STRING, sizeof(command_line));
	command_line[sizeof(command_line) - 1] = 0;
#endif /* CONFIG_BOOTPARAM */

	printk(KERN_INFO "\x0F\r\n\nuClinux/" CPU_NAME "\n");


	printk(KERN_INFO "Flat model support (C) 1998,1999 Kenneth Albanowski, D. Jeff Dionne\n");



	printk("KERNEL -> TEXT=0x%p-0x%p DATA=0x%p-0x%p BSS=0x%p-0x%p\n",
		 _stext, _etext, _sdata, _edata, __bss_start, __bss_stop);
	printk("MEMORY -> ROMFS=0x%p-0x%06lx MEM=0x%06lx-0x%06lx\n ",
		 __bss_stop, memory_start, memory_start, memory_end);

	/* Keep a copy of command line */
	*cmdline_p = &command_line[0];
	memcpy(boot_command_line, command_line, COMMAND_LINE_SIZE);
	boot_command_line[COMMAND_LINE_SIZE-1] = 0;



	/*
	 * Give all the memory to the bootmap allocator, tell it to put the
	 * boot mem_map at the start of memory.
	 */
	bootmap_size = init_bootmem_node(
			NODE_DATA(0),
			memory_start >> PAGE_SHIFT, /* map goes here */
			PAGE_OFFSET >> PAGE_SHIFT,	/* 0 on coldfire */
			memory_end >> PAGE_SHIFT);


	printk("bootmap_size: %d\r\n", bootmap_size);


	/*
	 * Free the usable memory, we have to make sure we do not free
	 * the bootmem bitmap so we then reserve it after freeing it :-)
	 */
	free_bootmem(memory_start, memory_end - memory_start);


	reserve_bootmem(memory_start, bootmap_size, BOOTMEM_DEFAULT);


	/*
	 * Get kmalloc into gear.
	 */
	paging_init();
}

/*
 *	Get CPU information for use by the procfs.
 */
static int show_cpuinfo(struct seq_file *m, void *v)
{
	char *cpu, *mmu, *fpu;
	u_long clockfreq;

	cpu = CPU_NAME;
	mmu = "none";
	fpu = "none";
	clockfreq = (loops_per_jiffy * HZ) * CPU_INSTR_PER_JIFFY;

	seq_printf(m, "CPU:\t\t%s\n"
		      "MMU:\t\t%s\n"
		      "FPU:\t\t%s\n"
		      "Clocking:\t%lu.%1luMHz\n"
		      "BogoMips:\t%lu.%02lu\n"
		      "Calibration:\t%lu loops\n",
		      cpu, mmu, fpu,
		      clockfreq / 1000000,
		      (clockfreq / 100000) % 10,
		      (loops_per_jiffy * HZ) / 500000,
		      ((loops_per_jiffy * HZ) / 5000) % 100,
		      (loops_per_jiffy * HZ));

	return 0;
}

static void *c_start(struct seq_file *m, loff_t *pos)
{
	return *pos < NR_CPUS ? ((void *) 0x12345678) : NULL;
}

static void *c_next(struct seq_file *m, void *v, loff_t *pos)
{
	++*pos;
	return c_start(m, pos);
}

static void c_stop(struct seq_file *m, void *v)
{
}

const struct seq_operations cpuinfo_op = {
	.start	= c_start,
	.next	= c_next,
	.stop	= c_stop,
	.show	= show_cpuinfo,
};

