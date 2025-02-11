/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Broadcom BCM590xx PMU
 *
 * Copyright 2014 Linaro Limited
 * Author: Matt Porter <mporter@linaro.org>
 */

#ifndef __LINUX_MFD_BCM590XX_H
#define __LINUX_MFD_BCM590XX_H

#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/regmap.h>

/* device types */
enum bcm590xx_dev_type {
	BCM59054_TYPE,
	BCM59056_TYPE,
	BCM590XX_TYPE_MAX,
};

/* max register address */
#define BCM590XX_MAX_REGISTER_PRI	0xe7
#define BCM590XX_MAX_REGISTER_SEC	0xf0

struct bcm590xx {
	struct device *dev;
	enum bcm590xx_dev_type dev_type;
	struct i2c_client *i2c_pri;
	struct i2c_client *i2c_sec;
	struct regmap *regmap_pri;
	struct regmap *regmap_sec;
	unsigned int id;

	/* Chip revision, read from PMUREV reg */
	u8 rev_dig;
	u8 rev_ana;
};

/* Known chip revision IDs */
#define BCM59054_REV_DIG_A1		1
#define BCM59054_REV_ANA_A1		2

#define BCM59056_REV_DIG_A0		1
#define BCM59056_REV_ANA_A0		1

#define BCM59056_REV_DIG_B0		2
#define BCM59056_REV_ANA_B0		2

#endif /*  __LINUX_MFD_BCM590XX_H */
