/*
 *  Atheros AR71XX/AR724X/AR913X specific prom routines
 *
 *  Copyright (C) 2008-2010 Gabor Juhos <juhosg@openwrt.org>
 *  Copyright (C) 2008 Imre Kaloz <kaloz@openwrt.org>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/string.h>

#include <asm/bootinfo.h>
#include <asm/addrspace.h>
#include <asm/fw/myloader/myloader.h>

#include "common.h"

static char ath79_cmdline_buf[COMMAND_LINE_SIZE] __initdata;

static inline int is_valid_ram_addr(void *addr)
{
	if (((u32) addr > KSEG0) &&
	    ((u32) addr < (KSEG0 + ATH79_MEM_SIZE_MAX)))
		return 1;

	if (((u32) addr > KSEG1) &&
	    ((u32) addr < (KSEG1 + ATH79_MEM_SIZE_MAX)))
		return 1;

	return 0;
}

static void __init ath79_prom_append_cmdline(const char *name,
					      const char *value)
{
	snprintf(ath79_cmdline_buf, sizeof(ath79_cmdline_buf),
		 " %s=%s", name, value);
	strlcat(arcs_cmdline, ath79_cmdline_buf, sizeof(arcs_cmdline));
}

static const char * __init ath79_prom_find_env(char **envp, const char *name)
{
	const char *ret = NULL;
	int len;
	char **p;

	if (!is_valid_ram_addr(envp))
		return NULL;

	len = strlen(name);
	for (p = envp; is_valid_ram_addr(*p); p++) {
		if (strncmp(name, *p, len) == 0 && (*p)[len] == '=') {
			ret = *p + len + 1;
			break;
		}

		/* RedBoot env comes in pointer pairs - key, value */
		if (strncmp(name, *p, len) == 0 && (*p)[len] == 0)
			if (is_valid_ram_addr(*(++p))) {
				ret = *p;
				break;
			}
	}

	return ret;
}

#ifdef CONFIG_IMAGE_CMDLINE_HACK
extern char __image_cmdline[];

static int __init ath79_use_image_cmdline(void)
{
	char *p = __image_cmdline;
	int replace = 0;

	if (*p == '-') {
		replace = 1;
		p++;
	}

	if (*p == '\0')
		return 0;

	if (replace) {
		strlcpy(arcs_cmdline, p, sizeof(arcs_cmdline));
	} else {
		strlcat(arcs_cmdline, " ", sizeof(arcs_cmdline));
		strlcat(arcs_cmdline, p, sizeof(arcs_cmdline));
	}

	return 1;
}
#else
static inline int ath79_use_image_cmdline(void) { return 0; }
#endif

static int __init ath79_prom_init_myloader(void)
{
	struct myloader_info *mylo;
	char mac_buf[32];
	unsigned char *mac;

	mylo = myloader_get_info();
	if (!mylo)
		return 0;

	switch (mylo->did) {
	case DEVID_COMPEX_WP543:
		ath79_prom_append_cmdline("board", "WP543");
		break;
	case DEVID_COMPEX_WPE72:
		ath79_prom_append_cmdline("board", "WPE72");
		break;
	default:
		pr_warn("prom: unknown device id: %x\n", mylo->did);
		return 0;
	}

	mac = mylo->macs[0];
	snprintf(mac_buf, sizeof(mac_buf), "%02x:%02x:%02x:%02x:%02x:%02x",
		 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	ath79_prom_append_cmdline("ethaddr", mac_buf);

	ath79_use_image_cmdline();

	return 1;
}

static __init void ath79_prom_init_cmdline(int argc, char **argv)
{
	int i;

	if (ath79_use_image_cmdline())
		return;

	if (!is_valid_ram_addr(argv))
		return;

	for (i = 0; i < argc; i++)
		if (is_valid_ram_addr(argv[i])) {
			strlcat(arcs_cmdline, " ", sizeof(arcs_cmdline));
			strlcat(arcs_cmdline, argv[i], sizeof(arcs_cmdline));
		}
}

#ifdef CONFIG_ATH9K_PROM_VARS
static __init void ath9k_prom_set_eth(const char *estr, u8 *ethaddr)
{
	u8 mac[6];
	int i;

	if (estr == NULL)
		return;
	memset(&mac[0], 0, sizeof(mac));
	i = sscanf(estr, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
		&mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
	if (i != 6)
		return;

	/* Only set it if we got an entire good MAC address */
	memcpy(ethaddr, &mac[0], sizeof(mac));
}
#endif

/*
 * Store sway the size of the uimage that loaded us.
 * We pull out and store some parts of the environment locally.
 */
unsigned long ath79_prom_isize;
u8 ath9k_prom_eth0addr[6];
u8 ath9k_prom_eth1addr[6];
u8 ath9k_prom_eth2addr[6];
u8 ath9k_prom_eth3addr[6];
u8 ath9k_prom_eth4addr[6];
u8 ath9k_prom_eth5addr[6];

void __init prom_init(void)
{
	const char *env;
	char **envp;

	if (ath79_prom_init_myloader())
		return;

	ath79_prom_init_cmdline(fw_arg0, (char **)fw_arg1);

	envp = (char **)fw_arg2;

#ifdef CONFIG_ATH9K_PROM_VARS
	env = ath79_prom_find_env(envp, "ethaddr");
	ath9k_prom_set_eth(env, ath9k_prom_eth0addr);
	env = ath79_prom_find_env(envp, "eth1addr");
	ath9k_prom_set_eth(env, ath9k_prom_eth1addr);
	env = ath79_prom_find_env(envp, "eth2addr");
	ath9k_prom_set_eth(env, ath9k_prom_eth2addr);
	env = ath79_prom_find_env(envp, "eth3addr");
	ath9k_prom_set_eth(env, ath9k_prom_eth3addr);
	env = ath79_prom_find_env(envp, "eth4addr");
	ath9k_prom_set_eth(env, ath9k_prom_eth4addr);
	env = ath79_prom_find_env(envp, "eth5addr");
	ath9k_prom_set_eth(env, ath9k_prom_eth5addr);
#else
	if (!strstr(arcs_cmdline, "ethaddr=")) {
		env = ath79_prom_find_env(envp, "ethaddr");
		if (env)
			ath79_prom_append_cmdline("ethaddr", env);
	}
#endif

	if (!strstr(arcs_cmdline, "board=")) {
		env = ath79_prom_find_env(envp, "board");
		if (env) {
			/* Workaround for buggy bootloaders */
			if (strcmp(env, "RouterStation") == 0 ||
			    strcmp(env, "Ubiquiti AR71xx-based board") == 0)
				env = "UBNT-RS";

			if (strcmp(env, "RouterStation PRO") == 0)
				env = "UBNT-RSPRO";

			ath79_prom_append_cmdline("board", env);
		}
	}

	if (strstr(arcs_cmdline, "board=750Gr3"))
		ath79_prom_append_cmdline("console", "ttyS0,115200");

	env = ath79_prom_find_env(envp, "image_start");
	if (env) {
		unsigned long istart, iend;
		int rc0, rc1;
		rc0 = kstrtoul(env, 0, &istart);
		env = ath79_prom_find_env(envp, "image_end");
		rc1 = kstrtoul(env, 0, &iend);
		if ((rc0 == 0) && (rc1 == 0))
			ath79_prom_isize = iend - istart;
	}
}

void __init prom_free_prom_memory(void)
{
	/* We do not have to prom memory to free */
}
