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
#include <linux/regulator/driver.h>

/* max register address */
#define BCM590XX_MAX_REGISTER_PRI	0xe7
#define BCM590XX_MAX_REGISTER_SEC	0xf0

struct bcm590xx {
	struct device *dev;
	char *device_type;
	struct i2c_client *i2c_pri;
	struct i2c_client *i2c_sec;
	struct regmap *regmap_pri;
	struct regmap *regmap_sec;
	unsigned int id;
};

#define BCM590XX_REG_ENABLE	BIT(7)
#define BCM590XX_VBUS_ENABLE	BIT(2)
#define BCM590XX_LDO_VSEL_MASK	GENMASK(5, 3)
#define BCM590XX_SR_VSEL_MASK	GENMASK(5, 0)

/* LDO group A: supported voltages in microvolts */
static const unsigned int ldo_a_table[] = {
	1200000, 1800000, 2500000, 2700000, 2800000,
	2900000, 3000000, 3300000,
};

/* LDO group C: supported voltages in microvolts */
static const unsigned int ldo_c_table[] = {
	3100000, 1800000, 2500000, 2700000, 2800000,
	2900000, 3000000, 3300000,
};

/* LDO group ?: supported voltages in microvolts */
static const unsigned int ldo_d_table[] = {
	1000000, 1107000, 1143000, 1214000, 1250000,
	1464000, 1500000, 1786000,
};

static const unsigned int ldo_vbus[] = {
	5000000,
};

/* BCM59054 locks the voltage of micldo at 1.8v. */
static const unsigned int ldo_mic[] = {
	1800000,
};

/* DCDC group CSR: supported voltages in microvolts */
static const struct linear_range dcdc_csr_ranges[] = {
	REGULATOR_LINEAR_RANGE(860000, 2, 50, 10000),
	REGULATOR_LINEAR_RANGE(1360000, 51, 55, 20000),
	REGULATOR_LINEAR_RANGE(900000, 56, 63, 0),
};

/* DCDC group IOSR1: supported voltages in microvolts */
static const struct linear_range dcdc_iosr1_ranges[] = {
	REGULATOR_LINEAR_RANGE(860000, 2, 51, 10000),
	REGULATOR_LINEAR_RANGE(1500000, 52, 52, 0),
	REGULATOR_LINEAR_RANGE(1800000, 53, 53, 0),
	REGULATOR_LINEAR_RANGE(900000, 54, 63, 0),
};

/* DCDC group SDSR1: supported voltages in microvolts */
static const struct linear_range dcdc_sdsr1_ranges[] = {
	REGULATOR_LINEAR_RANGE(860000, 2, 50, 10000),
	REGULATOR_LINEAR_RANGE(1340000, 51, 51, 0),
	REGULATOR_LINEAR_RANGE(900000, 52, 63, 0),
};

struct bcm590xx_info {
	const char *name;
	const char *vin_name;
	u8 n_voltages;
	const unsigned int *volt_table;
	u8 n_linear_ranges;
	const struct linear_range *linear_ranges;
};

#define BCM590XX_REG_TABLE(_name, _table) \
	{ \
		.name = #_name, \
		.n_voltages = ARRAY_SIZE(_table), \
		.volt_table = _table, \
	}

#define BCM590XX_REG_RANGES(_name, _ranges) \
	{ \
		.name = #_name, \
		.n_voltages = 64, \
		.n_linear_ranges = ARRAY_SIZE(_ranges), \
		.linear_ranges = _ranges, \
	}

struct bcm590xx_reg {
	struct regulator_desc *desc;
	struct bcm590xx *mfd;
};

#endif /*  __LINUX_MFD_BCM590XX_H */
