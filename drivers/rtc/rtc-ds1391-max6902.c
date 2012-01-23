/*
 * rtc-ds1391-max6902.c -- combined driver for the 
 *
 * 	- Dallas/Maxim DS1391/93/94 SPI RTC (new)
 * 	- Maxim MAX6902 SPI RTC (old)
 *
 * used by the Access Manager MIFARE/LEGIC.
 * 
 * Copyright (C) 2012 Kaba AG
 * Written by Stefan Wyss <stefan.wyss@kaba.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NOTE: Currently this driver only supports the bare minimum for read
 * and write the RTC. The extra features provided by the chip family
 * (alarms, trickle charger, different control registers) are unavailable.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/rtc.h>
#include <linux/spi/spi.h>
#include <linux/bcd.h>

#define DS1391_REG_100THS		0x00
#define DS1391_REG_SECONDS		0x01
#define DS1391_REG_MINUTES		0x02
#define DS1391_REG_HOURS		0x03
#define DS1391_REG_DAY			0x04
#define DS1391_REG_DATE			0x05
#define DS1391_REG_MONTH_CENT		0x06
#define DS1391_REG_YEAR			0x07

#define DS1391_REG_ALARM_100THS		0x08
#define DS1391_REG_ALARM_SECONDS	0x09
#define DS1391_REG_ALARM_MINUTES	0x0A
#define DS1391_REG_ALARM_HOURS		0x0B
#define DS1391_REG_ALARM_DAY_DATE	0x0C

#define DS1391_REG_CONTROL		0x0D
#define DS1391_REG_STATUS		0x0E
#define DS1391_REG_TRICKLE		0x0F

#define MAX6902_REG_SECONDS		0x01
#define MAX6902_REG_MINUTES		0x03
#define MAX6902_REG_HOURS		0x05
#define MAX6902_REG_DATE		0x07
#define MAX6902_REG_MONTH		0x09
#define MAX6902_REG_DAY			0x0B
#define MAX6902_REG_YEAR		0x0D
#define MAX6902_REG_CONTROL		0x0F
#define MAX6902_REG_CENTURY		0x13

/* type of RTC for autodetection */ 
#define MAX6902				0x81
#define DS1391				0x00
static int rtctype;

struct ds1391_max6902 {
	struct rtc_device *rtc;
	u8 txrx_buf[9];	/* cmd + 8 registers */
};

//*****************************************************************************
// Get/Set Register Functions
//*****************************************************************************
static int max6902_get_reg(struct device *dev, unsigned char address,
				unsigned char *data)
{
	struct spi_device *spi = to_spi_device(dev);

	/* Set MSB to indicate read */
	*data = address | 0x80;

	return spi_write_then_read(spi, data, 1, data, 1);
}
static int max6902_set_reg(struct device *dev, unsigned char address,
				unsigned char data)
{
	struct spi_device *spi = to_spi_device(dev);
	unsigned char buf[2];

	/* MSB must be '0' to write */
	buf[0] = address & 0x7f;
	buf[1] = data;

	return spi_write_then_read(spi, buf, 2, NULL, 0);
}

static int ds1391_get_reg(struct device *dev, unsigned char address,
				unsigned char *data)
{
	struct spi_device *spi = to_spi_device(dev);
	struct ds1391_max6902 *chip = dev_get_drvdata(dev);
	int status;

	if (!data)
		return -EINVAL;

	/* Clear MSB to indicate read */
	chip->txrx_buf[0] = address & 0x7f;
	/* do the i/o */
	status = spi_write_then_read(spi, chip->txrx_buf, 1, chip->txrx_buf, 1);
	if (status != 0)
		return status;

	*data = chip->txrx_buf[0];

	return 0;
}

static int ds1391_set_reg(struct device *dev, unsigned char address, unsigned char data)
{
	int ret=0;

	struct spi_device *spi = to_spi_device(dev);
	struct ds1391_max6902 *chip = dev_get_drvdata(dev);

	/* build the message */
	chip->txrx_buf[0] = address | 0x80;
	chip->txrx_buf[1] = data;

	/* do the i/o */
	ret = spi_write_then_read(spi, chip->txrx_buf, 2, NULL, 0);
	
	return ret;
}
//*****************************************************************************
// Read Time Functions
//*****************************************************************************
static int max6902_read_time(struct device *dev, struct rtc_time *dt)
{
	int err, century;
	struct spi_device *spi = to_spi_device(dev);
	unsigned char buf[8];

	buf[0] = 0xbf;	/* Burst read */

	err = spi_write_then_read(spi, buf, 1, buf, 8);
	if (err != 0)
		return err;

	/* The chip sends data in this order:
	 * Seconds, Minutes, Hours, Date, Month, Day, Year */
	dt->tm_sec	= bcd2bin(buf[0]);
	dt->tm_min	= bcd2bin(buf[1]);
	dt->tm_hour	= bcd2bin(buf[2]);
	dt->tm_mday	= bcd2bin(buf[3]);
	dt->tm_mon	= bcd2bin(buf[4]) - 1;
	dt->tm_wday	= bcd2bin(buf[5]);
	dt->tm_year	= bcd2bin(buf[6]);

	/* Read century */
	err = max6902_get_reg(dev, MAX6902_REG_CENTURY, &buf[0]);
	if (err != 0)
		return err;

	century = bcd2bin(buf[0]) * 100;

	dt->tm_year += century;
	dt->tm_year -= 1900;

	return rtc_valid_tm(dt);
}

static int ds1391_read_time(struct device *dev, struct rtc_time *dt)
{
	struct spi_device *spi = to_spi_device(dev);
	struct ds1391_max6902 *chip = dev_get_drvdata(dev);
	int status;

	/* build the message */
	chip->txrx_buf[0] = DS1391_REG_SECONDS;

	/* do the i/o */
	status = spi_write_then_read(spi, chip->txrx_buf, 1, chip->txrx_buf, 8);
	if (status != 0)
		return status;

	/* The chip sends data in this order:
	 * Seconds, Minutes, Hours, Day, Date, Month / Century, Year */
	dt->tm_sec	= bcd2bin(chip->txrx_buf[0]);
	dt->tm_min	= bcd2bin(chip->txrx_buf[1]);
	dt->tm_hour	= bcd2bin(chip->txrx_buf[2]);
	dt->tm_wday	= bcd2bin(chip->txrx_buf[3]);
	dt->tm_mday	= bcd2bin(chip->txrx_buf[4]);
	/* mask off century bit */
	dt->tm_mon	= bcd2bin(chip->txrx_buf[5] & 0x7f) - 1;
	/* adjust for century bit */
	dt->tm_year = bcd2bin(chip->txrx_buf[6]) + ((chip->txrx_buf[5] & 0x80) ? 100 : 0);

	return rtc_valid_tm(dt);
}
static int ds1391_max6902_read_time(struct device *dev, struct rtc_time *dt)
{
	if (rtctype == MAX6902)
		return max6902_read_time(dev,dt);
	else
		return ds1391_read_time(dev,dt);
}
//*****************************************************************************
// Set Time Functions
//*****************************************************************************
static int max6902_set_time(struct device *dev, struct rtc_time *dt)
{
	dt->tm_year = dt->tm_year + 1900;

	/* Remove write protection */
	max6902_set_reg(dev, 0xF, 0);

	max6902_set_reg(dev, 0x01, bin2bcd(dt->tm_sec));
	max6902_set_reg(dev, 0x03, bin2bcd(dt->tm_min));
	max6902_set_reg(dev, 0x05, bin2bcd(dt->tm_hour));

	max6902_set_reg(dev, 0x07, bin2bcd(dt->tm_mday));
	max6902_set_reg(dev, 0x09, bin2bcd(dt->tm_mon + 1));
	max6902_set_reg(dev, 0x0B, bin2bcd(dt->tm_wday));
	max6902_set_reg(dev, 0x0D, bin2bcd(dt->tm_year % 100));
	max6902_set_reg(dev, 0x13, bin2bcd(dt->tm_year / 100));

	/* Compulab used a delay here. However, the datasheet
	 * does not mention a delay being required anywhere... */
	/* delay(2000); */

	/* Write protect */
	max6902_set_reg(dev, 0xF, 0x80);

	return 0;
}

static int ds1391_set_time(struct device *dev, struct rtc_time *dt)
{
	struct spi_device *spi = to_spi_device(dev);
	struct ds1391_max6902 *chip = dev_get_drvdata(dev);

	/* build the message */
	chip->txrx_buf[0] = DS1391_REG_SECONDS | 0x80;
	chip->txrx_buf[1] = bin2bcd(dt->tm_sec);
	chip->txrx_buf[2] = bin2bcd(dt->tm_min);
	chip->txrx_buf[3] = bin2bcd(dt->tm_hour);
	chip->txrx_buf[4] = bin2bcd(dt->tm_wday);
	chip->txrx_buf[5] = bin2bcd(dt->tm_mday);
	chip->txrx_buf[6] = bin2bcd(dt->tm_mon + 1) |
				((dt->tm_year > 99) ? 0x80 : 0x00);
	chip->txrx_buf[7] = bin2bcd(dt->tm_year % 100);

	/* do the i/o */
	return spi_write_then_read(spi, chip->txrx_buf, 8, NULL, 0);
}

static int ds1391_max6902_set_time(struct device *dev, struct rtc_time *dt)
{
	if (rtctype == MAX6902)
		return max6902_set_time(dev,dt);
	else
		return ds1391_set_time(dev,dt);
}

//*****************************************************************************
// Probe and Remove Functions
//*****************************************************************************
static const struct rtc_class_ops ds1391_max6902_rtc_ops = {
	.read_time	= ds1391_max6902_read_time,
	.set_time	= ds1391_max6902_set_time,
};

static int __devinit ds1391_max6902_probe(struct spi_device *spi)
{
	unsigned char tmp;
	struct ds1391_max6902 *chip;
	int res;

	spi->mode = SPI_MODE_3;
	spi->bits_per_word = 8;
	spi_setup(spi);

	chip = kzalloc(sizeof *chip, GFP_KERNEL);
	if (!chip) {
		dev_err(&spi->dev, "unable to allocate device memory\n");
		return -ENOMEM;
	}
	dev_set_drvdata(&spi->dev, chip);

	// start device autodetection (trickle charge register of DS1391)
	res = ds1391_set_reg(&spi->dev, DS1391_REG_TRICKLE, 0xA6);	// Charge with 2KOhm at V5.0
	if (res != 0) {
		dev_err(&spi->dev, "unable to write to device\n");
		kfree(chip);
		return res;
	}		

	res = ds1391_get_reg(&spi->dev, DS1391_REG_TRICKLE, &tmp);	
	if (res != 0) {
		dev_err(&spi->dev, "unable to read device\n");
		kfree(chip);
		return res;
	}
	
	if (tmp == 0xA6)
	{
		rtctype = DS1391;
		dev_info(&spi->dev, "probe found DS139x on SPI bus\n");
	}
	else
	{
		rtctype = MAX6902;
		dev_info(&spi->dev, "probe found MAX6902 on SPI bus\n");
	}
	
	chip->rtc = rtc_device_register("ds1391-max6902",
				&spi->dev, &ds1391_max6902_rtc_ops, THIS_MODULE);
	
	if (IS_ERR(chip->rtc)) {
		dev_err(&spi->dev, "unable to register device\n");
		res = PTR_ERR(chip->rtc);
		kfree(chip);
	}

	return res;
}

static int __devexit ds1391_max6902_remove(struct spi_device *spi)
{
	struct ds1391_max6902 *chip = platform_get_drvdata(spi);

	rtc_device_unregister(chip->rtc);
	kfree(chip);

	return 0;
}

//*****************************************************************************
// Static Structs
//*****************************************************************************
static struct spi_driver ds1391_max6902_driver = {
	.driver = {
		.name	= "rtc-ds1391-max6902",
		.owner	= THIS_MODULE,
	},
	.probe	= ds1391_max6902_probe,
	.remove = __devexit_p(ds1391_max6902_remove),
};

static __init int ds1391_max6902_init(void)
{
	return spi_register_driver(&ds1391_max6902_driver);
}
module_init(ds1391_max6902_init);

static __exit void ds1391_max6902_exit(void)
{
	spi_unregister_driver(&ds1391_max6902_driver);
}
module_exit(ds1391_max6902_exit);

MODULE_DESCRIPTION("Dallas/Maxim DS1391/93/94 & MAX6902 SPI RTC driver");
MODULE_AUTHOR("Stefan Wyss <stefan.wyss@kaba.com>");
MODULE_LICENSE("GPL");
