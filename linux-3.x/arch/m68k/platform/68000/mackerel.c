#include <linux/init.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/rtc.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <asm/machdep.h>

void m68000_reset(void)
{
    printk("m68000_reset()\r\n");
}

int m68000_hwclk(int set, struct rtc_time *t)
{
    printk("m68000_hwclk()\r\n");
}

void m68000_hw_timer_init(irq_handler_t handler)
{
    printk("m68000_hw_timer_init()\r\n");
}

int m68000_set_clock_mmss(unsigned long a)
{
    printk("m68000_set_clock_mmss()\r\n");
}

void __init config_BSP(char *command, int len)
{
    pr_info("Mackerel 68k support by Colin Maykish - 2022\n");

    mach_sched_init = m68000_hw_timer_init;
    mach_hwclk = m68000_hwclk;
    mach_set_clock_mmss = m68000_set_clock_mmss;
    mach_reset = m68000_reset;
}
