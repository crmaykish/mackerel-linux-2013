/*
 * 8300.c -- special support for the Accelecon 8300
 *
 * (C) Copyright 2013, Greg Ungerer <greg.ungerer@accelecon.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/delay.h>

#define GPIO_USB1_PWR	4
#define GPIO_USB2_PWR	31
#define GPIO_USB3_PWR	47


void ac8300_usb_dumpregs(void)
{
	void __iomem *rp;

	rp = ioremap(0xd0000000, 0x00100000);
	if (rp == NULL) {
		printk("AC8300: failed to ioremap(0xd0000000)\n");
		return;
	}

	printk("  PCI-EXPRESS 0:\n");
	printk("  [0x00040000] = 0x%08x\n", readl(rp + 0x00040000));
	printk("  [0x00040008] = 0x%08x\n\n", readl(rp + 0x00040008));

	printk("  MISCELLANEOUS REGISTERS:\n");
	printk("  [0x00018000] = 0x%08x\n", readl(rp + 0x00018000));
	printk("  [0x00018004] = 0x%08x\n", readl(rp + 0x00018004));
	printk("  [0x00018008] = 0x%08x\n", readl(rp + 0x00018008));
	printk("  [0x0001800c] = 0x%08x\n", readl(rp + 0x0001800c));
	printk("  [0x00018010] = 0x%08x\n", readl(rp + 0x00018010));
	printk("  [0x00018014] = 0x%08x\n", readl(rp + 0x00018014));
	printk("  [0x00018018] = 0x%08x\n", readl(rp + 0x00018018));
	printk("  [0x0001801c] = 0x%08x\n", readl(rp + 0x0001801c));
	printk("  [0x00018020] = 0x%08x\n", readl(rp + 0x00018020));

	printk("  [0x00018204] = 0x%08x\n", readl(rp + 0x00018204));
	printk("  [0x00018208] = 0x%08x\n", readl(rp + 0x00018208));
	printk("  [0x00018210] = 0x%08x\n", readl(rp + 0x00018210));
	printk("  [0x00018214] = 0x%08x\n", readl(rp + 0x00018214));
	printk("  [0x00018218] = 0x%08x\n", readl(rp + 0x00018218));
	printk("  [0x0001821c] = 0x%08x\n", readl(rp + 0x0001821c));
	printk("  [0x00018220] = 0x%08x\n", readl(rp + 0x00018220));
	printk("  [0x00018228] = 0x%08x\n", readl(rp + 0x00018228));
	printk("  [0x0001822c] = 0x%08x\n", readl(rp + 0x0001822c));
	printk("  [0x00018230] = 0x%08x\n", readl(rp + 0x00018230));
	printk("  [0x00018234] = 0x%08x\n", readl(rp + 0x00018234));
	printk("  [0x00018238] = 0x%08x\n", readl(rp + 0x00018238));
	printk("  [0x0001823c] = 0x%08x\n", readl(rp + 0x0001823c));
	printk("  [0x00018250] = 0x%08x\n", readl(rp + 0x00018250));
	printk("  [0x00018260] = 0x%08x\n", readl(rp + 0x00018260));
	printk("  [0x00018264] = 0x%08x\n", readl(rp + 0x00018264));
	printk("  [0x00018270] = 0x%08x\n", readl(rp + 0x00018270));
	printk("  [0x000182d0] = 0x%08x\n", readl(rp + 0x000182d0));
	printk("  [0x000182d4] = 0x%08x\n\n", readl(rp + 0x000182d4));

	printk("  USB 0:\n");
	printk("  [0x00050310] = 0x%08x\n", readl(rp + 0x00050310));
	printk("  [0x00050314] = 0x%08x\n", readl(rp + 0x00050314));
	printk("  [0x00050318] = 0x%08x\n", readl(rp + 0x00050318));
	printk("  [0x0005031c] = 0x%08x\n", readl(rp + 0x0005031c));
	printk("  [0x00050300] = 0x%08x\n", readl(rp + 0x00050300));
	printk("  [0x00050320] = 0x%08x\n", readl(rp + 0x00050320));
	printk("  [0x00050324] = 0x%08x\n", readl(rp + 0x00050324));
	printk("  [0x00050330] = 0x%08x\n", readl(rp + 0x00050330));
	printk("  [0x00050334] = 0x%08x\n", readl(rp + 0x00050334));
	printk("  [0x00050340] = 0x%08x\n", readl(rp + 0x00050340));
	printk("  [0x00050344] = 0x%08x\n", readl(rp + 0x00050344));
	printk("  [0x00050350] = 0x%08x\n", readl(rp + 0x00050350));
	printk("  [0x00050354] = 0x%08x\n", readl(rp + 0x00050354));
	printk("  [0x00050400] = 0x%08x\n", readl(rp + 0x00050400));
	printk("  [0x00050404] = 0x%08x\n", readl(rp + 0x00050404));
	printk("  [0x00050420] = 0x%08x\n", readl(rp + 0x00050420));
	printk("  [0x00050430] = 0x%08x\n", readl(rp + 0x00050430));
	printk("  [0x00050440] = 0x%08x\n", readl(rp + 0x00050440));
	printk("  [0x00050800] = 0x%08x\n", readl(rp + 0x00050800));
	printk("  [0x00050804] = 0x%08x\n", readl(rp + 0x00050804));
	printk("  [0x00050808] = 0x%08x\n\n", readl(rp + 0x00050808));

	printk("  USB 1:\n");
	printk("  [0x00051310] = 0x%08x\n", readl(rp + 0x00051310));
	printk("  [0x00051314] = 0x%08x\n", readl(rp + 0x00051314));
	printk("  [0x00051318] = 0x%08x\n", readl(rp + 0x00051318));
	printk("  [0x0005131c] = 0x%08x\n", readl(rp + 0x0005131c));
	printk("  [0x00051300] = 0x%08x\n", readl(rp + 0x00051300));
	printk("  [0x00051320] = 0x%08x\n", readl(rp + 0x00051320));
	printk("  [0x00051324] = 0x%08x\n", readl(rp + 0x00051324));
	printk("  [0x00051330] = 0x%08x\n", readl(rp + 0x00051330));
	printk("  [0x00051334] = 0x%08x\n", readl(rp + 0x00051334));
	printk("  [0x00051340] = 0x%08x\n", readl(rp + 0x00051340));
	printk("  [0x00051344] = 0x%08x\n", readl(rp + 0x00051344));
	printk("  [0x00051350] = 0x%08x\n", readl(rp + 0x00051350));
	printk("  [0x00051354] = 0x%08x\n", readl(rp + 0x00051354));
	printk("  [0x00051400] = 0x%08x\n", readl(rp + 0x00051400));
	printk("  [0x00051404] = 0x%08x\n", readl(rp + 0x00051404));
	printk("  [0x00051420] = 0x%08x\n", readl(rp + 0x00051420));
	printk("  [0x00051430] = 0x%08x\n", readl(rp + 0x00051430));
	printk("  [0x00051440] = 0x%08x\n", readl(rp + 0x00051440));
	printk("  [0x00051800] = 0x%08x\n", readl(rp + 0x00051800));
	printk("  [0x00051804] = 0x%08x\n", readl(rp + 0x00051804));
	printk("  [0x00051808] = 0x%08x\n\n", readl(rp + 0x00051808));

	iounmap(rp);
}

static void ac8300_io_coherency(void __iomem *rp)
{
	u32 v;

printk("%s(%d): MV_COHERENCY_FABRIC_CTRL_REG=0x%08x\n", __FILE__, __LINE__, readl(rp+0x20200));
printk("%s(%d): MV_COHERENCY_FABRIC_CFG_REG=0x%08x\n", __FILE__, __LINE__, readl(rp+0x20204));
printk("%s(%d): MV_CIB_CTRL_CFG_REG=0x%08x\n", __FILE__, __LINE__, readl(rp+0x20280));

	v = readl(rp+0x20280);
	v &= ~(7 << 16);
	v |= (7 << 16);
	writel(v, rp+0x20280);

	v = readl(rp+0x20200);
	v |= (1 << 24);
	writel(v, rp+0x20200);
}

static int ac8300_usb_powerup(void)
{
	printk("AC8300: powering up external USB ports\n");

	/* Enable power to external USB connectors */
	gpio_request(GPIO_USB1_PWR, "USB1 Power");
	gpio_direction_output(GPIO_USB1_PWR, 1);
	gpio_set_value(GPIO_USB1_PWR, 0);

	gpio_request(GPIO_USB2_PWR, "USB2 Power");
	gpio_direction_output(GPIO_USB2_PWR, 1);
	gpio_set_value(GPIO_USB2_PWR, 0);

	gpio_request(GPIO_USB3_PWR, "USB3 Power");
	gpio_direction_output(GPIO_USB3_PWR, 1);
	gpio_set_value(GPIO_USB3_PWR, 0);

	return 0;
}
late_initcall(ac8300_usb_powerup);

/*
 * Using Marvell's own code this is done in there HAL. There is no current
 * main-line linux kernel equivilent to what this USB initialization code is
 * doing. I intend putting this in our boot loader first chance I get.
 */
static void ac8300_usb_init(void __iomem *rp, int port)
{
	void __iomem *up;
	void __iomem *pp;
	u32 v;

#if 1
	writel(0x3, rp+0x18204);
#endif

	up = rp + 0x00050000 + ((port) ? 0x1000 : 0);
	pp = rp + 0x00050000 + ((port) ? 0x0880 : 0x0840);
	
	/* Set USB memory windows */
	writel(0x1fff0e01, up + 0x320); /* 512MB, DRAM, CS0 */
	writel(0x00000000, up + 0x324);
	writel(0x1fff0d01, up + 0x330);
	writel(0x20000000, up + 0x334);
	writel(0x01ff1841, up + 0x340);
	writel(0xb0000000, up + 0x344);

#if 0
	/* Turn on the USB1 PLL */
	v = readl(rp+0x00050804);
	v = (v & ~0x3ff) | 0x605;
	writel(v, rp+0x00050804);

	v = readl(rp+0x00050808);
	v |= 0x00000200;
	writel(v, rp+0x00050808);

	v = readl(rp+0x00050804);
	v |= 0x00200000;
	writel(v, rp+0x00050804);

	/* Turn on the USB2 PLL */
	v = readl(rp+0x00051804);
	v = (v & ~0x3ff) | 0x605;
	writel(v, rp+0x00051804);

	v = readl(rp+0x00051808);
	v |= 0x00000200;
	writel(v, rp+0x00051808);

	v = readl(rp+0x00051804);
	v |= 0x00200000;
	writel(v, rp+0x00051804);
#endif
	if (port == 0) {
		/* Turn on all clocks we are interrested in */
		//writel(0x91cc023f, rp+0x00018220);
		//writel(0x91cc0018, rp+0x00018220);
		//writel(0xb3cec23f, rp+0x00018220);
		writel(0xf3cec23f, rp+0x00018220);

		udelay(1000);

		/* Turn on the USB PLL */
		v = readl(up+0x804);
		v &= ~0x3ff;
		v |= 0x605;
		writel(v, up+0x804);

		v = readl(up+0x808);
		v |= 0x200;
		writel(v, up+0x808);

		v = readl(up+0x804);
		v |= 0x00200000;
		writel(v, up+0x804);

		udelay(1000);
	}

	/* Clear USB bridge interrupt cause and mask */
	writel(0, up+0x310);
	writel(0, up+0x314);

	/* Reset Controller */
	v = readl(up+0x140);
	v |= 0x2;
	writel(v, up+0x140);

	for (v = 0; v < 1000; v++) {
		if ((readl(up+0x140) & 0x2) == 0)
			break;
	}

	/* Set special 0x360 register?? */
	v = readl(up+0x360);
	v = (v & ~(0x7F << 8)) | (0xd << 8);
	writel(v, up+0x360);

	/* mvUsbPhy40nmInit() */
	v = readl(pp+0xc);
	v |= 0x00008000;
	writel(v, pp+0xc);

	v = readl(pp+0x4);
	v |= 0x00001000;
	writel(v, pp+0x4);
	udelay(50);
	v = readl(pp+0x4);
	v &= ~0x00001000;
	writel(v, pp+0x4);

	/* Better PHY TX drive strength settings - as per errata */
	writel(0x20000131, pp+0x10);

	/* Stop the controller */
	v = readl(up+0x140);
	v &= ~0x1;
	writel(v, up+0x140);

	/* Reset Controller */
	v = readl(up+0x140);
	v |= 0x2;
	writel(v, up+0x140);

	for (v = 0; v < 1000; v++) {
		if ((readl(up+0x140) & 0x2) == 0)
			break;
	}

	/* Set USB host mode */
	writel(3, up+0x1a8);
}

static int ac8300_init(void)
{
	void __iomem *rp;
	u32 v;

	printk("AC8300: machine setup\n");

	rp = ioremap(0xd0000000, 0x00100000);
	if (rp == NULL) {
		printk("AC8300: failed to ioremap(0xd0000000)\n");
		return 0;
	}

#if 0
	ac8300_io_coherency(rp);
#endif
#if 0
	ac8300_usb_init(rp, 0);
	ac8300_usb_init(rp, 1);
#endif
	iounmap(rp);

#if 0
	ac8300_usb_dumpregs();
#endif

	return 0;
}

arch_initcall(ac8300_init);
