/*
 * SPI driver for Broadcom BCM53118 8-port ethernet switch
 *
 * (C) Copyright 2013, Greg Ungerer <greg.ungerer@accelecon.com>
 *
 * This file was based on: drivers/net/phy/spi_ks8995.c
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/spi/spi.h>
#include <linux/io.h>

#define BCM53118_CMD_WRITE	0x61
#define BCM53118_CMD_READ	0x60

#define BCM53118_REGS_SIZE	0x100

struct bcm53118_switch {
	struct spi_device	*spi;
	struct mutex		lock;
};

static int bcm53118_read_spi(struct bcm53118_switch *sp, u8 reg, u8 *buf, u8 cnt)
{
	struct spi_transfer t[2];
	struct spi_message m;
	int err;
	u8 cmd[2];

	cmd[0] = BCM53118_CMD_READ;
	cmd[1] = reg;

	spi_message_init(&m);

	memset(&t, 0, sizeof(t));
	t[0].tx_buf = cmd;
	t[0].len = sizeof(cmd);
	spi_message_add_tail(&t[0], &m);

	t[1].rx_buf = buf;
	t[1].len = cnt;
	spi_message_add_tail(&t[1], &m);

	mutex_lock(&sp->lock);
	err = spi_sync(sp->spi, &m);
	mutex_unlock(&sp->lock);

	return (err) ? err : cnt;
}

static int bcm53118_write_spi(struct bcm53118_switch *sp, u8 reg, u8 *buf, u8 cnt)
{
	struct spi_transfer t[2];
	struct spi_message m;
	int err;
	u8 cmd[2];

	cmd[0] = BCM53118_CMD_WRITE;
	cmd[1] = reg;

	spi_message_init(&m);

	memset(&t, 0, sizeof(t));
	t[0].tx_buf = cmd;
	t[0].len = sizeof(cmd);
	spi_message_add_tail(&t[0], &m);

	t[1].tx_buf = buf;
	t[1].len = cnt;
	spi_message_add_tail(&t[1], &m);

	mutex_lock(&sp->lock);
	err = spi_sync(sp->spi, &m);
	mutex_unlock(&sp->lock);

	return (err) ? err : cnt;
}

static ssize_t bcm53118_sysfs_read(struct file *filp, struct kobject *kobj,
				   struct bin_attribute *bin_attr,
				   char *buf, loff_t off, size_t count)
{
	struct device *dev;
	struct bcm53118_switch *sp;

	dev = container_of(kobj, struct device, kobj);
	sp = dev_get_drvdata(dev);

	if (unlikely(off > BCM53118_REGS_SIZE))
		return 0;

	if ((off + count) > BCM53118_REGS_SIZE)
		count = BCM53118_REGS_SIZE - off;

	if (unlikely(!count))
		return count;

	return bcm53118_read_spi(sp, off, buf, count);
}

static ssize_t bcm53118_sysfs_write(struct file *filp, struct kobject *kobj,
				    struct bin_attribute *bin_attr,
				    char *buf, loff_t off, size_t count)
{
	struct device *dev;
	struct bcm53118_switch *sp;

	dev = container_of(kobj, struct device, kobj);
	sp = dev_get_drvdata(dev);

	if (unlikely(off >= BCM53118_REGS_SIZE))
		return -EFBIG;

	if ((off + count) > BCM53118_REGS_SIZE)
		count = BCM53118_REGS_SIZE - off;

	if (unlikely(!count))
		return count;

	return bcm53118_write_spi(sp, off, buf, count);
}

static struct bin_attribute bcm53118_registers_attr = {
	.attr = {
		.name   = "registers",
		.mode   = S_IRUSR | S_IWUSR,
	},
	.size   = BCM53118_REGS_SIZE,
	.read   = bcm53118_sysfs_read,
	.write  = bcm53118_sysfs_write,
};

static int bcm53118_probe(struct spi_device *spi)
{
	struct bcm53118_switch *sp;
	int err;

	sp = kzalloc(sizeof(*sp), GFP_KERNEL);
	if (!sp)
		return -ENOMEM;

	mutex_init(&sp->lock);
	sp->spi = spi_dev_get(spi);
	spi_set_drvdata(spi, sp);

	err = spi_setup(spi);
	if (err) {
		dev_err(&spi->dev, "spi_setup failed, err=%d\n", err);
		goto err_drvdata;
	}

	err = sysfs_create_bin_file(&spi->dev.kobj, &bcm53118_registers_attr);
	if (err) {
		dev_err(&spi->dev, "failed to create sysfs file, err=%d\n", err);
		goto err_drvdata;
	}

	dev_info(&spi->dev, "BCM53118 device found\n");
	return 0;

err_drvdata:
	spi_set_drvdata(spi, NULL);
	kfree(sp);
	return err;
}

static int bcm53118_remove(struct spi_device *spi)
{
	struct bcm53118_data *dp;

	dp = spi_get_drvdata(spi);
	sysfs_remove_bin_file(&spi->dev.kobj, &bcm53118_registers_attr);
	spi_set_drvdata(spi, NULL);
	kfree(dp);
	return 0;
}

static struct spi_driver bcm53118_driver = {
	.driver = {
		.name	= "bcm53118",
		.owner	= THIS_MODULE,
	},
	.probe	= bcm53118_probe,
	.remove	= bcm53118_remove,
};

module_spi_driver(bcm53118_driver);

MODULE_DESCRIPTION("Broadcom BCM53118 Ethernet switch SPI driver");
MODULE_AUTHOR("Greg Ungerer <greg.ungerer@accelecon.com>");
MODULE_LICENSE("GPL");
