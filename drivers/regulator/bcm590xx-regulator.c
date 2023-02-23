// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Broadcom BCM590xx regulator driver
 *
 * Copyright 2014 Linaro Limited
 * Author: Matt Porter <mporter@linaro.org>
 */

#include <linux/err.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/mfd/bcm590xx.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/of_regulator.h>
#include <linux/slab.h>

struct bcm590xx_info {
	const char *name;
	const char *vin_name;
	u8 n_voltages;
	const unsigned int *volt_table;
	u8 n_linear_ranges;
	const struct linear_range *linear_ranges;
};

struct bcm590xx_device_ops {
	int (*reg_is_ldo)(int id);
	int (*reg_is_gpldo)(int id);
	int (*reg_is_static)(int id);

	int (*reg_is_secondary)(int id);
	int (*reg_mode_is_3bit)(int id);

	int (*get_vsel_register)(int id);
	int (*get_enable_register)(int id);
};

struct bcm590xx_device_info {
	struct bcm590xx_info *regulators;
	int n_regulators;
	struct bcm590xx_device_ops ops;
};

#define BCM590XX_REG_ENABLE		GENMASK(7, 0)
#define BCM590XX_REG_3BIT_ENABLE	GENMASK(5, 0)
#define BCM590XX_VBUS_ENABLE		BIT(2)
#define BCM590XX_LDO_VSEL_MASK		GENMASK(5, 3)
#define BCM590XX_SR_VSEL_MASK		GENMASK(5, 0)

/* BCM59056 registers */

/* I2C slave 0 registers */
#define BCM59056_RFLDOPMCTRL1	0x60
#define BCM59056_IOSR1PMCTRL1	0x7a
#define BCM59056_IOSR2PMCTRL1	0x7c
#define BCM59056_CSRPMCTRL1	0x7e
#define BCM59056_SDSR1PMCTRL1	0x82
#define BCM59056_SDSR2PMCTRL1	0x86
#define BCM59056_MSRPMCTRL1	0x8a
#define BCM59056_VSRPMCTRL1	0x8e
#define BCM59056_RFLDOCTRL	0x96
#define BCM59056_CSRVOUT1	0xc0

/* I2C slave 1 registers */
#define BCM59056_GPLDO5PMCTRL1	0x16
#define BCM59056_GPLDO6PMCTRL1	0x18
#define BCM59056_GPLDO1CTRL	0x1a
#define BCM59056_GPLDO2CTRL	0x1b
#define BCM59056_GPLDO3CTRL	0x1c
#define BCM59056_GPLDO4CTRL	0x1d
#define BCM59056_GPLDO5CTRL	0x1e
#define BCM59056_GPLDO6CTRL	0x1f
#define BCM59056_OTG_CTRL	0x40
#define BCM59056_GPLDO1PMCTRL1	0x57
#define BCM59056_GPLDO2PMCTRL1	0x59
#define BCM59056_GPLDO3PMCTRL1	0x5b
#define BCM59056_GPLDO4PMCTRL1	0x5d

/*
 * RFLDO to VSR regulators are
 * accessed via I2C slave 0
 */

/* LDO regulator IDs */
#define BCM59056_REG_RFLDO	0
#define BCM59056_REG_CAMLDO1	1
#define BCM59056_REG_CAMLDO2	2
#define BCM59056_REG_SIMLDO1	3
#define BCM59056_REG_SIMLDO2	4
#define BCM59056_REG_SDLDO	5
#define BCM59056_REG_SDXLDO	6
#define BCM59056_REG_MMCLDO1	7
#define BCM59056_REG_MMCLDO2	8
#define BCM59056_REG_AUDLDO	9
#define BCM59056_REG_MICLDO	10
#define BCM59056_REG_USBLDO	11
#define BCM59056_REG_VIBLDO	12

/* DCDC regulator IDs */
#define BCM59056_REG_CSR	13
#define BCM59056_REG_IOSR1	14
#define BCM59056_REG_IOSR2	15
#define BCM59056_REG_MSR	16
#define BCM59056_REG_SDSR1	17
#define BCM59056_REG_SDSR2	18
#define BCM59056_REG_VSR	19

/*
 * GPLDO1 to VBUS regulators are
 * accessed via I2C slave 1
 */

#define BCM59056_REG_GPLDO1	20
#define BCM59056_REG_GPLDO2	21
#define BCM59056_REG_GPLDO3	22
#define BCM59056_REG_GPLDO4	23
#define BCM59056_REG_GPLDO5	24
#define BCM59056_REG_GPLDO6	25
#define BCM59056_REG_VBUS	26

#define BCM59056_NUM_REGS	27

/* LDO group A: supported voltages in microvolts */
static const unsigned int bcm59056_ldo_a_table[] = {
	1200000, 1800000, 2500000, 2700000, 2800000,
	2900000, 3000000, 3300000,
};

/* LDO group C: supported voltages in microvolts */
static const unsigned int bcm59056_ldo_c_table[] = {
	3100000, 1800000, 2500000, 2700000, 2800000,
	2900000, 3000000, 3300000,
};

static const unsigned int bcm59056_ldo_vbus[] = {
	5000000,
};

/* DCDC group CSR: supported voltages in microvolts */
static const struct linear_range bcm59056_dcdc_csr_ranges[] = {
	REGULATOR_LINEAR_RANGE(860000, 2, 50, 10000),
	REGULATOR_LINEAR_RANGE(1360000, 51, 55, 20000),
	REGULATOR_LINEAR_RANGE(900000, 56, 63, 0),
};

/* DCDC group IOSR1: supported voltages in microvolts */
static const struct linear_range bcm59056_dcdc_iosr1_ranges[] = {
	REGULATOR_LINEAR_RANGE(860000, 2, 51, 10000),
	REGULATOR_LINEAR_RANGE(1500000, 52, 52, 0),
	REGULATOR_LINEAR_RANGE(1800000, 53, 53, 0),
	REGULATOR_LINEAR_RANGE(900000, 54, 63, 0),
};

/* DCDC group SDSR1: supported voltages in microvolts */
static const struct linear_range bcm59056_dcdc_sdsr1_ranges[] = {
	REGULATOR_LINEAR_RANGE(860000, 2, 50, 10000),
	REGULATOR_LINEAR_RANGE(1340000, 51, 51, 0),
	REGULATOR_LINEAR_RANGE(900000, 52, 63, 0),
};

#define BCM59056_REG_TABLE(_name, _table) \
	{ \
		.name = #_name, \
		.n_voltages = ARRAY_SIZE(bcm59056_##_table), \
		.volt_table = bcm59056_##_table, \
	}

#define BCM59056_REG_RANGES(_name, _ranges) \
	{ \
		.name = #_name, \
		.n_voltages = 64, \
		.n_linear_ranges = ARRAY_SIZE(bcm59056_##_ranges), \
		.linear_ranges = bcm59056_##_ranges, \
	}

static struct bcm590xx_info bcm59056_regs[] = {
	BCM59056_REG_TABLE(rfldo, ldo_a_table),
	BCM59056_REG_TABLE(camldo1, ldo_c_table),
	BCM59056_REG_TABLE(camldo2, ldo_c_table),
	BCM59056_REG_TABLE(simldo1, ldo_a_table),
	BCM59056_REG_TABLE(simldo2, ldo_a_table),
	BCM59056_REG_TABLE(sdldo, ldo_c_table),
	BCM59056_REG_TABLE(sdxldo, ldo_a_table),
	BCM59056_REG_TABLE(mmcldo1, ldo_a_table),
	BCM59056_REG_TABLE(mmcldo2, ldo_a_table),
	BCM59056_REG_TABLE(audldo, ldo_a_table),
	BCM59056_REG_TABLE(micldo, ldo_a_table),
	BCM59056_REG_TABLE(usbldo, ldo_a_table),
	BCM59056_REG_TABLE(vibldo, ldo_c_table),
	BCM59056_REG_RANGES(csr, dcdc_csr_ranges),
	BCM59056_REG_RANGES(iosr1, dcdc_iosr1_ranges),
	BCM59056_REG_RANGES(iosr2, dcdc_iosr1_ranges),
	BCM59056_REG_RANGES(msr, dcdc_iosr1_ranges),
	BCM59056_REG_RANGES(sdsr1, dcdc_sdsr1_ranges),
	BCM59056_REG_RANGES(sdsr2, dcdc_iosr1_ranges),
	BCM59056_REG_RANGES(vsr, dcdc_iosr1_ranges),
	BCM59056_REG_TABLE(gpldo1, ldo_a_table),
	BCM59056_REG_TABLE(gpldo2, ldo_a_table),
	BCM59056_REG_TABLE(gpldo3, ldo_a_table),
	BCM59056_REG_TABLE(gpldo4, ldo_a_table),
	BCM59056_REG_TABLE(gpldo5, ldo_a_table),
	BCM59056_REG_TABLE(gpldo6, ldo_a_table),
	BCM59056_REG_TABLE(vbus, ldo_vbus),
};

static int bcm59056_reg_is_ldo(int id) { return (id < BCM59056_REG_CSR); }
static int bcm59056_reg_is_gpldo(int id) \
	{ return ((id > BCM59056_REG_VSR) && (id < BCM59056_REG_VBUS)); }
static int bcm59056_reg_is_static(int id) { return (id == BCM59056_REG_VBUS); }

static int bcm59056_reg_is_secondary(int id) \
	{ return (bcm59056_reg_is_gpldo(id) || id == BCM59056_REG_VBUS); }
static int bcm59056_reg_mode_is_3bit(int id) \
	{ return false; }

static int bcm59056_get_vsel_register(int id)
{
	if (bcm59056_reg_is_ldo(id))
		return BCM59056_RFLDOCTRL + id;
	else if (bcm59056_reg_is_gpldo(id))
		return BCM59056_GPLDO1CTRL + id;
	else
		return BCM59056_CSRVOUT1 + (id - BCM59056_REG_CSR) * 3;
}

static int bcm59056_get_enable_register(int id)
{
	int reg = 0;

	if (bcm59056_reg_is_ldo(id))
		reg = BCM59056_RFLDOPMCTRL1 + id * 2;
	else if (bcm59056_reg_is_gpldo(id))
		reg = BCM59056_GPLDO1PMCTRL1 + id * 2;
	else
		switch (id) {
		case BCM59056_REG_CSR:
			reg = BCM59056_CSRPMCTRL1;
			break;
		case BCM59056_REG_IOSR1:
			reg = BCM59056_IOSR1PMCTRL1;
			break;
		case BCM59056_REG_IOSR2:
			reg = BCM59056_IOSR2PMCTRL1;
			break;
		case BCM59056_REG_MSR:
			reg = BCM59056_MSRPMCTRL1;
			break;
		case BCM59056_REG_SDSR1:
			reg = BCM59056_SDSR1PMCTRL1;
			break;
		case BCM59056_REG_SDSR2:
			reg = BCM59056_SDSR2PMCTRL1;
			break;
		case BCM59056_REG_VSR:
			reg = BCM59056_VSRPMCTRL1;
			break;
		case BCM59056_REG_VBUS:
			reg = BCM59056_OTG_CTRL;
			break;
		}

	return reg;
}

static const struct bcm590xx_device_ops bcm59056_device_ops = {
	.reg_is_ldo = bcm59056_reg_is_ldo,
	.reg_is_gpldo = bcm59056_reg_is_gpldo,
	.reg_is_static = bcm59056_reg_is_static,

	.reg_is_secondary = bcm59056_reg_is_secondary,
	.reg_mode_is_3bit = bcm59056_reg_mode_is_3bit,

	.get_vsel_register = bcm59056_get_vsel_register,
	.get_enable_register = bcm59056_get_enable_register,
};

static const struct bcm590xx_device_info bcm59056_device_info = {
	.regulators = bcm59056_regs,
	.n_regulators = BCM59056_NUM_REGS,
	.ops = bcm59056_device_ops,
};

/* BCM59054 registers */

/* I2C slave 0 registers */
#define BCM59054_RFLDOPMCTRL1	0x60
#define BCM59054_IOSR1PMCTRL1	0x7a
#define BCM59054_IOSR2PMCTRL1	0x7c
#define BCM59054_CSRPMCTRL1	0x7e
#define BCM59054_SDSR1PMCTRL1	0x82
#define BCM59054_SDSR2PMCTRL1	0x86
#define BCM59054_MMSRPMCTRL1	0x8a
#define BCM59054_VSRPMCTRL1	0x8e
#define BCM59054_RFLDOCTRL	0x96
#define BCM59054_CSRVOUT1	0xc0

/* I2C slave 1 registers */
#define BCM59054_LVLDO1PMCTRL1	0x16
#define BCM59054_LVLDO2PMCTRL1	0x18
#define BCM59054_GPLDO1CTRL	0x1a
#define BCM59054_GPLDO2CTRL	0x1b
#define BCM59054_GPLDO3CTRL	0x1c
#define BCM59054_TCXLDOCTRL	0x1d
#define BCM59054_LVLDO1CTRL	0x1e
#define BCM59054_LVLDO2CTRL	0x1f
#define BCM59054_OTG_CTRL	0x40
#define BCM59054_GPLDO1PMCTRL1	0x57
#define BCM59054_GPLDO2PMCTRL1	0x59
#define BCM59054_GPLDO3PMCTRL1	0x5b
#define BCM59054_TCXLDOPMCTRL1	0x5d

/*
 * RFLDO to VSR regulators are
 * accessed via I2C slave 0
 */

/* LDO regulator IDs */
#define BCM59054_REG_RFLDO	0
#define BCM59054_REG_CAMLDO1	1
#define BCM59054_REG_CAMLDO2	2
#define BCM59054_REG_SIMLDO1	3
#define BCM59054_REG_SIMLDO2	4
#define BCM59054_REG_SDLDO	5
#define BCM59054_REG_SDXLDO	6
#define BCM59054_REG_MMCLDO1	7
#define BCM59054_REG_MMCLDO2	8
#define BCM59054_REG_AUDLDO	9
#define BCM59054_REG_MICLDO	10
#define BCM59054_REG_USBLDO	11
#define BCM59054_REG_VIBLDO	12

/* DCDC regulator IDs */
#define BCM59054_REG_CSR	13
#define BCM59054_REG_IOSR1	14
#define BCM59054_REG_IOSR2	15
#define BCM59054_REG_MMSR	16
#define BCM59054_REG_SDSR1	17
#define BCM59054_REG_SDSR2	18
#define BCM59054_REG_VSR	19

/*
 * GPLDO1 to LVLDO regulators are
 * accessed via I2C slave 1
 */

#define BCM59054_REG_GPLDO1	20
#define BCM59054_REG_GPLDO2	21
#define BCM59054_REG_GPLDO3	22
#define BCM59054_REG_TCXLDO	23
#define BCM59054_REG_LVLDO1	24
#define BCM59054_REG_LVLDO2	25

#define BCM59054_NUM_REGS	26

/* LDO group 1: supported voltages in microvolts */
static const unsigned int bcm59054_ldo_1_table[] = {
	1200000, 1800000, 2500000, 2700000, 2800000,
	2900000, 3000000, 3300000,
};

/* LDO group 2: supported voltages in microvolts */
static const unsigned int bcm59054_ldo_2_table[] = {
	3100000, 1800000, 2500000, 2700000, 2800000,
	2900000, 3000000, 3300000,
};

/* LDO group 3: supported voltages in microvolts */
static const unsigned int bcm59054_ldo_3_table[] = {
	1000000, 1107000, 1143000, 1214000, 1250000,
	1464000, 1500000, 1786000,
};

/* DCDC group SR: supported voltages in microvolts */
static const struct linear_range bcm59054_dcdc_sr_ranges[] = {
	REGULATOR_LINEAR_RANGE(0, 0, 1, 0),
	REGULATOR_LINEAR_RANGE(860000, 2, 60, 10000),
	REGULATOR_LINEAR_RANGE(1500000, 61, 61, 0),
	REGULATOR_LINEAR_RANGE(1800000, 62, 62, 0),
	REGULATOR_LINEAR_RANGE(900000, 63, 63, 0),
};

/* DCDC group VSR: supported voltages in microvolts */
static const struct linear_range bcm59054_dcdc_vsr_ranges[] = {
	REGULATOR_LINEAR_RANGE(0, 0, 1, 0),
	REGULATOR_LINEAR_RANGE(860000, 2, 59, 10000),
	REGULATOR_LINEAR_RANGE(1700000, 60, 60, 0),
	REGULATOR_LINEAR_RANGE(1500000, 61, 61, 0),
	REGULATOR_LINEAR_RANGE(1800000, 62, 62, 0),
	REGULATOR_LINEAR_RANGE(1600000, 63, 63, 0),
};

/* DCDC group CSR: supported voltages in microvolts */
static const struct linear_range bcm59054_dcdc_csr_ranges[] = {
	REGULATOR_LINEAR_RANGE(700000, 0, 1, 100000),
	REGULATOR_LINEAR_RANGE(860000, 2, 60, 10000),
	REGULATOR_LINEAR_RANGE(900000, 61, 63, 0),
};

#define BCM59054_REG_TABLE(_name, _table) \
	{ \
		.name = #_name, \
		.n_voltages = ARRAY_SIZE(bcm59054_##_table), \
		.volt_table = bcm59054_##_table, \
	}

#define BCM59054_REG_RANGES(_name, _ranges) \
	{ \
		.name = #_name, \
		.n_voltages = 64, \
		.n_linear_ranges = ARRAY_SIZE(bcm59054_##_ranges), \
		.linear_ranges = bcm59054_##_ranges, \
	}

static struct bcm590xx_info bcm59054_regs[] = {
	BCM59054_REG_TABLE(rfldo, ldo_1_table),
	BCM59054_REG_TABLE(camldo1, ldo_2_table),
	BCM59054_REG_TABLE(camldo2, ldo_2_table),
	BCM59054_REG_TABLE(simldo1, ldo_1_table),
	BCM59054_REG_TABLE(simldo2, ldo_1_table),
	BCM59054_REG_TABLE(sdldo, ldo_2_table),
	BCM59054_REG_TABLE(sdxldo, ldo_1_table),
	BCM59054_REG_TABLE(mmcldo1, ldo_1_table),
	BCM59054_REG_TABLE(mmcldo2, ldo_1_table),
	BCM59054_REG_TABLE(audldo, ldo_1_table),
	BCM59054_REG_TABLE(micldo, ldo_1_table),
	BCM59054_REG_TABLE(usbldo, ldo_1_table),
	BCM59054_REG_TABLE(vibldo, ldo_2_table),
	BCM59054_REG_RANGES(csr, dcdc_csr_ranges),
	BCM59054_REG_RANGES(iosr1, dcdc_sr_ranges),
	BCM59054_REG_RANGES(iosr2, dcdc_sr_ranges),
	BCM59054_REG_RANGES(mmsr, dcdc_sr_ranges),
	BCM59054_REG_RANGES(sdsr1, dcdc_sr_ranges),
	BCM59054_REG_RANGES(sdsr2, dcdc_sr_ranges),
	BCM59054_REG_RANGES(vsr, dcdc_vsr_ranges),
	BCM59054_REG_TABLE(gpldo1, ldo_1_table),
	BCM59054_REG_TABLE(gpldo2, ldo_1_table),
	BCM59054_REG_TABLE(gpldo3, ldo_1_table),
	BCM59054_REG_TABLE(tcxldo, ldo_1_table),
	BCM59054_REG_TABLE(lvldo1, ldo_3_table),
	BCM59054_REG_TABLE(lvldo2, ldo_3_table),
};

static int bcm59054_reg_is_ldo(int id) { return (id < BCM59054_REG_CSR); }
static int bcm59054_reg_is_gpldo(int id) { return (id > BCM59054_REG_VSR); }
static int bcm59054_reg_is_static(int id) \
	{ return false; }

static int bcm59054_reg_is_secondary(int id) \
	{ return bcm59054_reg_is_gpldo(id); }
static int bcm59054_reg_mode_is_3bit(int id) \
	{ return (id == BCM59056_REG_CSR || \
		 (id > BCM59054_REG_IOSR2 && id < BCM59054_REG_GPLDO1)); }

static int bcm59054_get_vsel_register(int id)
{
	if (bcm59054_reg_is_ldo(id))
		return BCM59054_RFLDOCTRL + id;
	else if (bcm59054_reg_is_gpldo(id))
		return BCM59054_GPLDO1CTRL + id;
	else
		return BCM59054_CSRVOUT1 + (id - BCM59054_REG_CSR) * 3;
}

static int bcm59054_get_enable_register(int id)
{
	int reg = 0;

	if (bcm59054_reg_is_ldo(id))
		reg = BCM59054_RFLDOPMCTRL1 + id * 2;
	else if (bcm59054_reg_is_gpldo(id))
		reg = BCM59054_GPLDO1PMCTRL1 + id * 2;
	else
		switch (id) {
		case BCM59054_REG_VSR:
			reg = BCM59054_VSRPMCTRL1;
			break;
		case BCM59054_REG_CSR:
			reg = BCM59054_CSRPMCTRL1;
			break;
		case BCM59054_REG_MMSR:
			reg = BCM59054_MMSRPMCTRL1;
			break;
		case BCM59054_REG_SDSR1:
			reg = BCM59054_SDSR1PMCTRL1;
			break;
		case BCM59054_REG_SDSR2:
			reg = BCM59054_SDSR2PMCTRL1;
			break;
		case BCM59054_REG_IOSR1:
			reg = BCM59054_IOSR1PMCTRL1;
			break;
		case BCM59054_REG_IOSR2:
			reg = BCM59054_IOSR2PMCTRL1;
			break;
		}

	return reg;
}

static const struct bcm590xx_device_ops bcm59054_device_ops = {
	.reg_is_ldo = bcm59054_reg_is_ldo,
	.reg_is_gpldo = bcm59054_reg_is_gpldo,
	.reg_is_static = bcm59054_reg_is_static,

	.reg_is_secondary = bcm59054_reg_is_secondary,
	.reg_mode_is_3bit = bcm59054_reg_mode_is_3bit,

	.get_vsel_register = bcm59054_get_vsel_register,
	.get_enable_register = bcm59054_get_enable_register,
};

static const struct bcm590xx_device_info bcm59054_device_info = {
	.regulators = bcm59054_regs,
	.n_regulators = BCM59054_NUM_REGS,
	.ops = bcm59054_device_ops
};

struct bcm590xx_reg {
	struct regulator_desc *desc;
	struct bcm590xx *mfd;
	const struct bcm590xx_device_info *device_info;
};

static const struct regulator_ops bcm590xx_ops_ldo = {
	.is_enabled		= regulator_is_enabled_regmap,
	.enable			= regulator_enable_regmap,
	.disable		= regulator_disable_regmap,
	.get_voltage_sel	= regulator_get_voltage_sel_regmap,
	.set_voltage_sel	= regulator_set_voltage_sel_regmap,
	.list_voltage		= regulator_list_voltage_table,
	.map_voltage		= regulator_map_voltage_iterate,
};

static const struct regulator_ops bcm590xx_ops_dcdc = {
	.is_enabled		= regulator_is_enabled_regmap,
	.enable			= regulator_enable_regmap,
	.disable		= regulator_disable_regmap,
	.get_voltage_sel	= regulator_get_voltage_sel_regmap,
	.set_voltage_sel	= regulator_set_voltage_sel_regmap,
	.list_voltage		= regulator_list_voltage_linear_range,
	.map_voltage		= regulator_map_voltage_linear_range,
};

static const struct regulator_ops bcm590xx_ops_static = {
	.is_enabled		= regulator_is_enabled_regmap,
	.enable			= regulator_enable_regmap,
	.disable		= regulator_disable_regmap,
};

static int bcm590xx_probe(struct platform_device *pdev)
{
	struct bcm590xx *bcm590xx = dev_get_drvdata(pdev->dev.parent);
	const struct bcm590xx_device_info *dev_info;
	struct bcm590xx_reg *pmu;
	struct regulator_config config = { };
	struct bcm590xx_info *info;
	struct regulator_dev *rdev;
	int i;

	pmu = devm_kzalloc(&pdev->dev, sizeof(*pmu), GFP_KERNEL);
	if (!pmu)
		return -ENOMEM;

	pmu->mfd = bcm590xx;

	if (pmu->mfd->device_type == BCM59054_TYPE)
		pmu->device_info = &bcm59054_device_info;
	else if (pmu->mfd->device_type == BCM59056_TYPE)
		pmu->device_info = &bcm59056_device_info;

	platform_set_drvdata(pdev, pmu);

	pmu->desc = devm_kcalloc(&pdev->dev,
				 pmu->device_info->n_regulators,
				 sizeof(struct regulator_desc),
				 GFP_KERNEL);
	if (!pmu->desc)
		return -ENOMEM;

	dev_info = pmu->device_info;
	info = pmu->device_info->regulators;

	/* Register the regulators */
	for (i = 0; i < pmu->device_info->n_regulators; i++, info++) {
		pmu->desc[i].name = info->name;
		pmu->desc[i].of_match = of_match_ptr(info->name);
		pmu->desc[i].regulators_node = of_match_ptr("regulators");
		pmu->desc[i].supply_name = info->vin_name;
		pmu->desc[i].id = i;
		pmu->desc[i].volt_table = info->volt_table;
		pmu->desc[i].n_voltages = info->n_voltages;
		pmu->desc[i].linear_ranges = info->linear_ranges;
		pmu->desc[i].n_linear_ranges = info->n_linear_ranges;

		if (dev_info->ops.reg_is_ldo(i) || \
				dev_info->ops.reg_is_gpldo(i)) {
			pmu->desc[i].ops = &bcm590xx_ops_ldo;
			pmu->desc[i].vsel_mask = BCM590XX_LDO_VSEL_MASK;
		} else if (dev_info->ops.reg_is_static(i)) {
			pmu->desc[i].ops = &bcm590xx_ops_static;
		} else {
			pmu->desc[i].ops = &bcm590xx_ops_dcdc;
			pmu->desc[i].vsel_mask = BCM590XX_SR_VSEL_MASK;
		}

		if (pmu->mfd->device_type == BCM59056_TYPE && \
				i == BCM59056_REG_VBUS) {
			pmu->desc[i].enable_mask = BCM590XX_VBUS_ENABLE;
		} else {
			pmu->desc[i].vsel_reg = dev_info->ops.get_vsel_register(i);
			pmu->desc[i].enable_is_inverted = true;
			if (dev_info->ops.reg_mode_is_3bit(i))
				pmu->desc[i].enable_mask = BCM590XX_REG_3BIT_ENABLE;
			else
				pmu->desc[i].enable_mask = BCM590XX_REG_ENABLE;
		}
		pmu->desc[i].enable_reg = dev_info->ops.get_enable_register(i);
		pmu->desc[i].type = REGULATOR_VOLTAGE;
		pmu->desc[i].owner = THIS_MODULE;

		config.dev = pmu->mfd->dev;
		config.driver_data = pmu;
		if (dev_info->ops.reg_is_secondary(i))
			config.regmap = pmu->mfd->regmap_sec;
		else
			config.regmap = pmu->mfd->regmap_pri;

		rdev = devm_regulator_register(&pdev->dev, &pmu->desc[i],
					       &config);
		if (IS_ERR(rdev)) {
			dev_err(bcm590xx->dev,
				"failed to register %s regulator\n",
				pdev->name);
			return PTR_ERR(rdev);
		}
	}

	return 0;
}

static struct platform_driver bcm590xx_regulator_driver = {
	.driver = {
		.name = "bcm590xx-vregs",
	},
	.probe = bcm590xx_probe,
};
module_platform_driver(bcm590xx_regulator_driver);

MODULE_AUTHOR("Matt Porter <mporter@linaro.org>");
MODULE_DESCRIPTION("BCM590xx voltage regulator driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:bcm590xx-vregs");
