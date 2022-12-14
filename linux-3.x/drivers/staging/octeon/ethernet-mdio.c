/**********************************************************************
 * Author: Cavium Networks
 *
 * Contact: support@caviumnetworks.com
 * This file is part of the OCTEON SDK
 *
 * Copyright (c) 2003-2007 Cavium Networks
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, Version 2, as
 * published by the Free Software Foundation.
 *
 * This file is distributed in the hope that it will be useful, but
 * AS-IS and WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE, TITLE, or
 * NONINFRINGEMENT.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this file; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 * or visit http://www.gnu.org/licenses/.
 *
 * This file may also be available under a different license from Cavium.
 * Contact Cavium Networks for more information
**********************************************************************/
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/ethtool.h>
#include <linux/phy.h>
#include <linux/ratelimit.h>
#include <linux/of_mdio.h>

#include <net/dst.h>

#include <asm/octeon/octeon.h>

#include "ethernet-defines.h"
#include "octeon-ethernet.h"
#include "ethernet-mdio.h"
#include "ethernet-util.h"

#include <asm/octeon/cvmx-helper-board.h>

#include <asm/octeon/cvmx-smix-defs.h>

#ifdef CONFIG_SG590
/*
 *	The Marvel 88e6064 switch attached to eth0 on the SnapGear SG590
 *	board needs some extra code to access its registers. It uses a
 *	cmd:address/data register pair.
 */

int mvl88e6064_readmii(int p, int r)
{
	union cvmx_smix_cmd smi_cmd;
	union cvmx_smix_wr_dat smi_wr;
	union cvmx_smix_rd_dat smi_rd;

	/* Write out address register */
	smi_wr.u64 = 0;
	smi_wr.s.dat = 0x9800 | (p << 5) | r;
	cvmx_write_csr(CVMX_SMIX_WR_DAT(0), smi_wr.u64);
	smi_cmd.u64 = 0;
	smi_cmd.s.phy_op = 0;
	smi_cmd.s.phy_adr = 0x10;
	smi_cmd.s.reg_adr = 0;
	cvmx_write_csr(CVMX_SMIX_CMD(0), smi_cmd.u64);

	do {
		smi_wr.u64 = cvmx_read_csr(CVMX_SMIX_WR_DAT(0));
	} while (smi_wr.s.pending);

retry:
	/* Check BUSY bit not set anymore */
	smi_cmd.u64 = 0;
	smi_cmd.s.phy_op = 1;
	smi_cmd.s.phy_adr = 0x10;
	smi_cmd.s.reg_adr = 0;
	cvmx_write_csr(CVMX_SMIX_CMD(0), smi_cmd.u64);

	do {
	    smi_rd.u64 = cvmx_read_csr(CVMX_SMIX_RD_DAT(0));
	} while (smi_rd.s.pending);

	if ((smi_rd.s.val) && (smi_rd.s.dat & 0x8000))
		goto retry;

	/* Read the register value from DATA register */
	smi_cmd.u64 = 0;
	smi_cmd.s.phy_op = 1;
	smi_cmd.s.phy_adr = 0x10;
	smi_cmd.s.reg_adr = 1;
	cvmx_write_csr(CVMX_SMIX_CMD(0), smi_cmd.u64);

	do {
	    smi_rd.u64 = cvmx_read_csr(CVMX_SMIX_RD_DAT(0));
	} while (smi_rd.s.pending);

	if (smi_rd.s.val)
		return smi_rd.s.dat;
	return 0xffff;
}

void mvl88e6064_writemii(int p, int r, int v)
{
	union cvmx_smix_cmd smi_cmd;
	union cvmx_smix_wr_dat smi_wr;
	union cvmx_smix_rd_dat smi_rd;

	/* Write out data register first */
	smi_wr.u64 = 0;
	smi_wr.s.dat = v & 0xffff;
	cvmx_write_csr(CVMX_SMIX_WR_DAT(0), smi_wr.u64);
	smi_cmd.u64 = 0;
	smi_cmd.s.phy_op = 0;
	smi_cmd.s.phy_adr = 0x10;
	smi_cmd.s.reg_adr = 1;
	cvmx_write_csr(CVMX_SMIX_CMD(0), smi_cmd.u64);

	do {
		smi_wr.u64 = cvmx_read_csr(CVMX_SMIX_WR_DAT(0));
	} while (smi_wr.s.pending);

	/* Write out command/address register */
	smi_wr.u64 = 0;
	smi_wr.s.dat = 0x9400 | (p << 5) | r;
	cvmx_write_csr(CVMX_SMIX_WR_DAT(0), smi_wr.u64);
	smi_cmd.u64 = 0;
	smi_cmd.s.phy_op = 0;
	smi_cmd.s.phy_adr = 0x10;
	smi_cmd.s.reg_adr = 0;
	cvmx_write_csr(CVMX_SMIX_CMD(0), smi_cmd.u64);

	do {
		smi_wr.u64 = cvmx_read_csr(CVMX_SMIX_WR_DAT(0));
	} while (smi_wr.s.pending);

retry:
	/* Check BUSY bit not set anymore */
	smi_cmd.u64 = 0;
	smi_cmd.s.phy_op = 1;
	smi_cmd.s.phy_adr = 0x10;
	smi_cmd.s.reg_adr = 0;
	cvmx_write_csr(CVMX_SMIX_CMD(0), smi_cmd.u64);

	do {
	    smi_rd.u64 = cvmx_read_csr(CVMX_SMIX_RD_DAT(0));
	} while (smi_rd.s.pending);

	if (smi_rd.s.dat & 0x8000)
		goto retry;
}

#if 0
static void mvl88e6064_dumpmii(void)
{
	int i, j;

	for (i = 0; (i < 32); i++) {
		printk("PHY ADDR=%02x:", i);
		for (j = 0; (j < 32); j++) {
			if ((j % 8) == 0) printk("\n  %08x:  ", j);
			printk("%04x ", mvl88e6064_readmii(i, j));
		}
		printk("\n\n");
	}
}
#endif

void cvm_oct_marvel_switch_init(void)
{
	/* First, set the switch ports so they only forward to 
	 * the rgmii port. This is done using the VLAN table
	 * register bits, and for the switch ports, only setting
	 * the bit for the rgmii port, and for the rgmii port,
	 * setting all bits bar itself.
	 */
	mvl88e6064_writemii(0x10, 6, 0x400);
	mvl88e6064_writemii(0x11, 6, 0x400);
	mvl88e6064_writemii(0x12, 6, 0x400);
	mvl88e6064_writemii(0x13, 6, 0x400);
	mvl88e6064_writemii(0x19, 6, 0x400);
	/* Enable all ports, except the RGMII port */
	mvl88e6064_writemii(0x1A, 6, 0x3FF);     
	/* Hard set link for the rgmii port to the 88e6064 core */
	mvl88e6064_writemii(0x1a, 1, 0x003e);

#if 0
	mvl88e6064_dumpmii();
#endif
}

#endif /* CONFIG_SG590 */

/**
 * Perform an MII read. Called by the generic MII routines
 *
 * @dev:      Device to perform read for
 * @phy_id:   The MII phy id
 * @location: Register location to read
 * Returns Result from the read or zero on failure
 */
static int cvm_oct_mdio_read(struct net_device *dev, int phy_id, int location)
{
	union cvmx_smix_cmd smi_cmd;
	union cvmx_smix_rd_dat smi_rd;

#ifdef CONFIG_SG590
	if (phy_id == 16) {
		smi_rd.s.dat = mvl88e6064_readmii(0x1a, location);
		return smi_rd.s.dat;
	} else if (phy_id >= 32) {
		/*
		 * On the SG590, we map all of the switch registers
		 * above address 32.
		 */
		smi_rd.s.dat = mvl88e6064_readmii((phy_id % 32), location);
		return smi_rd.s.dat;
	}
#endif

	smi_cmd.u64 = 0;
	smi_cmd.s.phy_op = 1;
	smi_cmd.s.phy_adr = phy_id;
	smi_cmd.s.reg_adr = location;
	cvmx_write_csr(CVMX_SMIX_CMD(0), smi_cmd.u64);

	do {
		if (!in_interrupt())
			yield();
		smi_rd.u64 = cvmx_read_csr(CVMX_SMIX_RD_DAT(0));
	} while (smi_rd.s.pending);

	if (smi_rd.s.val)
		return smi_rd.s.dat;
	else
		return 0;
}

static int cvm_oct_mdio_dummy_read(struct net_device *dev, int phy_id,
				   int location)
{
	return 0xffff;
}

/**
 * Perform an MII write. Called by the generic MII routines
 *
 * @dev:      Device to perform write for
 * @phy_id:   The MII phy id
 * @location: Register location to write
 * @val:      Value to write
 */
static void cvm_oct_mdio_write(struct net_device *dev, int phy_id, int location,
			       int val)
{
	union cvmx_smix_cmd smi_cmd;
	union cvmx_smix_wr_dat smi_wr;

#ifdef CONFIG_SG590
	if (phy_id == 16) {
		mvl88e6064_writemii(0, location, val);
		return;
	} else if (phy_id >= 32) {
		/*
		 * On the 590, we map all of the switch registers
		 * above address 32.
		 */
		mvl88e6064_writemii((phy_id % 32), location, val);
		return;
	}
#endif

	smi_wr.u64 = 0;
	smi_wr.s.dat = val;
	cvmx_write_csr(CVMX_SMIX_WR_DAT(0), smi_wr.u64);

	smi_cmd.u64 = 0;
	smi_cmd.s.phy_op = 0;
	smi_cmd.s.phy_adr = phy_id;
	smi_cmd.s.reg_adr = location;
	cvmx_write_csr(CVMX_SMIX_CMD(0), smi_cmd.u64);

	do {
		if (!in_interrupt())
			yield();
		smi_wr.u64 = cvmx_read_csr(CVMX_SMIX_WR_DAT(0));
	} while (smi_wr.s.pending);
}

static void cvm_oct_get_drvinfo(struct net_device *dev,
				struct ethtool_drvinfo *info)
{
	strlcpy(info->driver, "cavium-ethernet", sizeof(info->driver));
	strlcpy(info->version, OCTEON_ETHERNET_VERSION, sizeof(info->version));
	strlcpy(info->bus_info, "Builtin", sizeof(info->bus_info));
}

static int cvm_oct_get_settings(struct net_device *dev, struct ethtool_cmd *cmd)
{
	struct octeon_ethernet *priv = netdev_priv(dev);

	if (priv->phydev)
		return phy_ethtool_gset(priv->phydev, cmd);

	return -EINVAL;
}

static int cvm_oct_set_settings(struct net_device *dev, struct ethtool_cmd *cmd)
{
	struct octeon_ethernet *priv = netdev_priv(dev);

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	if (priv->phydev)
		return phy_ethtool_sset(priv->phydev, cmd);

	return -EINVAL;
}

static int cvm_oct_nway_reset(struct net_device *dev)
{
	struct octeon_ethernet *priv = netdev_priv(dev);

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	if (priv->phydev)
		return phy_start_aneg(priv->phydev);

	return -EINVAL;
}

const struct ethtool_ops cvm_oct_ethtool_ops = {
	.get_drvinfo = cvm_oct_get_drvinfo,
	.get_settings = cvm_oct_get_settings,
	.set_settings = cvm_oct_set_settings,
	.nway_reset = cvm_oct_nway_reset,
	.get_link = ethtool_op_get_link,
};

/**
 * cvm_oct_ioctl - IOCTL support for PHY control
 * @dev:    Device to change
 * @rq:     the request
 * @cmd:    the command
 *
 * Returns Zero on success
 */
int cvm_oct_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
	struct octeon_ethernet *priv = netdev_priv(dev);

	if (!netif_running(dev))
		return -EINVAL;

	if (!priv->phydev)
		return -EINVAL;

	return phy_mii_ioctl(priv->phydev, rq, cmd);
}

static void cvm_oct_adjust_link(struct net_device *dev)
{
	struct octeon_ethernet *priv = netdev_priv(dev);
	cvmx_helper_link_info_t link_info;

	if (priv->last_link != priv->phydev->link) {
		priv->last_link = priv->phydev->link;
		link_info.u64 = 0;
		link_info.s.link_up = priv->last_link ? 1 : 0;
		link_info.s.full_duplex = priv->phydev->duplex ? 1 : 0;
		link_info.s.speed = priv->phydev->speed;
		cvmx_helper_link_set( priv->port, link_info);
		if (priv->last_link) {
			netif_carrier_on(dev);
			if (priv->queue != -1)
				printk_ratelimited("%s: %u Mbps %s duplex, "
						   "port %2d, queue %2d\n",
						   dev->name, priv->phydev->speed,
						   priv->phydev->duplex ?
						   "Full" : "Half",
						   priv->port, priv->queue);
			else
				printk_ratelimited("%s: %u Mbps %s duplex, "
						   "port %2d, POW\n",
						   dev->name, priv->phydev->speed,
						   priv->phydev->duplex ?
						   "Full" : "Half",
						   priv->port);
		} else {
			netif_carrier_off(dev);
			printk_ratelimited("%s: Link down\n", dev->name);
		}
	}
}


/**
 * cvm_oct_phy_setup_device - setup the PHY
 *
 * @dev:    Device to setup
 *
 * Returns Zero on success, negative on failure
 */
int cvm_oct_phy_setup_device(struct net_device *dev)
{
	struct octeon_ethernet *priv = netdev_priv(dev);
	struct device_node *phy_node;

	if (!priv->of_node)
		return 0;

	phy_node = of_parse_phandle(priv->of_node, "phy-handle", 0);
	if (!phy_node)
		return 0;

	priv->phydev = of_phy_connect(dev, phy_node, cvm_oct_adjust_link, 0,
				      PHY_INTERFACE_MODE_GMII);

	if (priv->phydev == NULL)
		return -ENODEV;

	priv->last_link = 0;
	phy_start_aneg(priv->phydev);

	return 0;
}
