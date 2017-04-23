/*
 * Semtech SX1276 LoRa transceiver
 *
 * Copyright (c) 2016-2017 Andreas Färber
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <linux/delay.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/spi/spi.h>

#include "lora.h"

#define REG_OPMODE	0x01
#define REG_VERSION	0x42

#define REG_OPMODE_LONG_RANGE_MODE	BIT(7)

struct sx1276_priv {
	struct lora_priv lora;
};

static int sx1276_read_single(struct spi_device *spi, u8 reg, u8 *val)
{
	u8 addr = reg & 0x7f;
	return spi_write_then_read(spi, &addr, 1, val, 1);
}

static int sx1276_write_single(struct spi_device *spi, u8 reg, u8 val)
{
	u8 buf[2];

	buf[0] = reg | 0x80;
	buf[1] = val;
	return spi_write_then_read(spi, buf, 2, NULL, 0);
}

static const struct net_device_ops sx1276_netdev_ops =  {
};

static int sx1276_probe(struct spi_device *spi)
{
	struct net_device *netdev;
	int rst, dio[6], ret, model, i;
	u8 val;

	rst = of_get_named_gpio(spi->dev.of_node, "reset-gpio", 0);
	if (rst == -ENOENT)
		dev_warn(&spi->dev, "no reset GPIO available, ignoring");

	for (i = 0; i < 6; i++) {
		dio[i] = of_get_named_gpio(spi->dev.of_node, "dio-gpios", i);
		if (dio[i] == -ENOENT)
			dev_warn(&spi->dev, "DIO%d not available, ignoring", i);
		else {
			ret = gpio_direction_input(dio[i]);
			if (ret)
				dev_err(&spi->dev, "couldn't set DIO%d to input", i);
		}
	}

	if (gpio_is_valid(rst)) {
		gpio_set_value(rst, 1);
		udelay(100);
		gpio_set_value(rst, 0);
		msleep(5);
	}

	spi->bits_per_word = 8;
	spi_setup(spi);

	ret = sx1276_read_single(spi, REG_VERSION, &val);
	if (ret) {
		dev_err(&spi->dev, "version read failed");
		return ret;
	}

	if (val == 0x22)
		model = 1272;
	else {
		if (gpio_is_valid(rst)) {
			gpio_set_value(rst, 0);
			udelay(100);
			gpio_set_value(rst, 1);
			msleep(5);
		}

		ret = sx1276_read_single(spi, REG_VERSION, &val);
		if (ret) {
			dev_err(&spi->dev, "version read failed");
			return ret;
		}

		if (val == 0x12)
			model = 1276;
		else {
			dev_err(&spi->dev, "transceiver not recognized (RegVersion = 0x%02x)", (unsigned)val);
			return -EINVAL;
		}
	}

	ret = sx1276_write_single(spi, REG_OPMODE, REG_OPMODE_LONG_RANGE_MODE);
	if (ret) {
		dev_err(&spi->dev, "failed writing opmode");
		return ret;
	}

	netdev = alloc_loradev(sizeof(struct sx1276_priv));
	if (!netdev)
		return -ENOMEM;

	netdev->netdev_ops = &sx1276_netdev_ops;
	spi_set_drvdata(spi, netdev);
	SET_NETDEV_DEV(netdev, &spi->dev);

	ret = register_loradev(netdev);
	if (ret) {
		free_loradev(netdev);
		return ret;
	}

	dev_info(&spi->dev, "SX1276 module probed (SX%d)", model);

	return 0;
}

static int sx1276_remove(struct spi_device *spi)
{
	struct net_device *netdev = spi_get_drvdata(spi);

	unregister_loradev(netdev);
	free_loradev(netdev);

	dev_info(&spi->dev, "SX1276 module removed");

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id sx1276_dt_ids[] = {
	{ .compatible = "semtech,sx1272" },
	{ .compatible = "semtech,sx1276" },
	{}
};
MODULE_DEVICE_TABLE(of, sx1276_dt_ids);
#endif

static struct spi_driver sx1276_spi_driver = {
	.driver = {
		.name = "sx1276",
		.of_match_table = of_match_ptr(sx1276_dt_ids),
	},
	.probe = sx1276_probe,
	.remove = sx1276_remove,
};

module_spi_driver(sx1276_spi_driver);

MODULE_DESCRIPTION("SX1276 SPI driver");
MODULE_AUTHOR("Andreas Färber <afaerber@suse.de>");
MODULE_LICENSE("GPL");
