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

struct bcm590xx_reg {
	struct regulator_desc *desc;
	struct bcm590xx *mfd;
};

#define BCM590XX_REG_ENABLE		GENMASK(7, 0)
#define BCM590XX_REG_3BIT_ENABLE	(GENMASK(4, 3) | GENMASK(1, 0))
#define BCM590XX_VBUS_ENABLE		BIT(2)
#define BCM590XX_LDO_VSEL_MASK		GENMASK(5, 3)
#define BCM590XX_SR_VSEL_MASK		GENMASK(5, 0)

#define BCM590XX_PMMODE_ON		0x0
#define BCM590XX_PMMODE_LPM		0x1
#define BCM590XX_PMMODE_OFF		0x2

#define PMMODE_3BIT_MASK(mode)						\
	((mode << 3) | mode)
#define PMMODE_2BIT_MASK(mode)						\
	((mode << 6) | (mode << 4) | (mode << 2) | mode)

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
 * GPLDO1 to VBUS regulators are
 * accessed via I2C slave 1
 */

#define BCM59054_REG_GPLDO1	20
#define BCM59054_REG_GPLDO2	21
#define BCM59054_REG_GPLDO3	22
#define BCM59054_REG_TCXLDO	23
#define BCM59054_REG_LVLDO1	24
#define BCM59054_REG_LVLDO2	25
#define BCM59054_REG_VBUS	26

#define BCM59054_NUM_REGS	27

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

static const unsigned int bcm59054_ldo_vbus[] = {
	5000000,
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
	BCM59054_REG_TABLE(vbus, ldo_vbus),
};

static int bcm590xx_reg_is_ldo(struct bcm590xx_reg *pmu, int id)
{
	switch (pmu->mfd->device_type) {
	case BCM59054_TYPE:
		return (id < BCM59054_REG_CSR);
	case BCM59056_TYPE:
		return (id < BCM59056_REG_CSR);
	}
	return -EINVAL;
}

static int bcm590xx_reg_is_gpldo(struct bcm590xx_reg *pmu, int id)
{
	switch (pmu->mfd->device_type) {
	case BCM59054_TYPE:
		return ((id > BCM59054_REG_VSR) && (id < BCM59054_REG_VBUS));
	case BCM59056_TYPE:
		return ((id > BCM59056_REG_VSR) && (id < BCM59056_REG_VBUS));
	}
	return -EINVAL;
}

static int bcm590xx_reg_is_vbus(struct bcm590xx_reg *pmu, int id)
{
	switch (pmu->mfd->device_type) {
	case BCM59054_TYPE:
		return (id == BCM59054_REG_VBUS);
	case BCM59056_TYPE:
		return (id == BCM59056_REG_VBUS);
	}
	return -EINVAL;
}

static int bcm590xx_reg_is_static(struct bcm590xx_reg *pmu, int id)
{
	return bcm590xx_reg_is_vbus(pmu, id);
}

static int bcm590xx_reg_is_secondary(struct bcm590xx_reg *pmu, int id)
{
	switch (pmu->mfd->device_type) {
	case BCM59054_TYPE:
		return (bcm590xx_reg_is_gpldo(pmu, id) || \
			id == BCM59054_REG_VBUS);
	case BCM59056_TYPE:
		return (bcm590xx_reg_is_gpldo(pmu, id) || \
			id == BCM59056_REG_VBUS);
	}
	return -EINVAL;
}

static int bcm590xx_reg_mode_is_3bit(struct bcm590xx_reg *pmu, int id)
{
	switch (pmu->mfd->device_type) {
	case BCM59054_TYPE:
		return (id == BCM59054_REG_CSR || \
			(id > BCM59054_REG_IOSR2 && id < BCM59054_REG_GPLDO1));
	case BCM59056_TYPE:
		return false;
	}
	return -EINVAL;
}

static int bcm590xx_get_vsel_register(struct bcm590xx_reg *pmu, int id)
{
	switch (pmu->mfd->device_type) {
	case BCM59054_TYPE:
		if (bcm590xx_reg_is_ldo(pmu, id))
			return BCM59054_RFLDOCTRL + (id - BCM59054_REG_RFLDO);
		else if (bcm590xx_reg_is_gpldo(pmu, id))
			return BCM59054_GPLDO1CTRL + (id - BCM59054_REG_GPLDO1);
		else
			return BCM59054_CSRVOUT1 + (id - BCM59054_REG_CSR) * 3;
	case BCM59056_TYPE:
		if (bcm590xx_reg_is_ldo(pmu, id))
			return BCM59056_RFLDOCTRL + (id - BCM59056_REG_RFLDO);
		else if (bcm590xx_reg_is_gpldo(pmu, id))
			return BCM59056_GPLDO1CTRL + (id - BCM59056_REG_GPLDO1);
		else
			return BCM59056_CSRVOUT1 + (id - BCM59056_REG_CSR) * 3;
	}
	return -EINVAL;
}

static int bcm59054_get_pmctrl_register(struct bcm590xx_reg *pmu, int id)
{
	int reg = 0;

	if (bcm590xx_reg_is_ldo(pmu, id))
		reg = BCM59054_RFLDOPMCTRL1 + (id - BCM59054_REG_RFLDO) * 2;
	else if (bcm590xx_reg_is_gpldo(pmu, id))
		reg = BCM59054_GPLDO1PMCTRL1 + (id - BCM59054_REG_GPLDO1) * 2;
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
		case BCM59054_REG_VBUS:
			reg = BCM59054_OTG_CTRL;
			break;
		}

	return reg;
}

static int bcm59056_get_pmctrl_register(struct bcm590xx_reg *pmu, int id)
{
	int reg = 0;

	if (bcm590xx_reg_is_ldo(pmu, id))
		reg = BCM59056_RFLDOPMCTRL1 + (id - BCM59056_REG_RFLDO) * 2;
	else if (bcm590xx_reg_is_gpldo(pmu, id))
		reg = BCM59056_GPLDO1PMCTRL1 + (id - BCM59056_REG_GPLDO1) * 2;
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

static int bcm590xx_get_pmctrl_register(struct bcm590xx_reg *pmu, int id)
{
	switch (pmu->mfd->device_type) {
	case BCM59054_TYPE:
		return bcm59054_get_pmctrl_register(pmu, id);
	case BCM59056_TYPE:
		return bcm59056_get_pmctrl_register(pmu, id);
	}
	return -EINVAL;
}

static int bcm590xx_get_enable_mask(struct bcm590xx_reg *pmu, int id)
{
	if (bcm590xx_reg_mode_is_3bit(pmu, id))
		return BCM590XX_REG_3BIT_ENABLE;

	return BCM590XX_REG_ENABLE;
}

/*
 * The state of BCM590XX regulators is controlled by the PM mode; most
 * have 3 such modes (off, low-power and on), but some have more.
 *
 * These modes are then stored in the PMCTRL registers - there are 7
 * PMMODE entries within these registers for any given regulator.
 * Which one is selected is decided by the PC1 and PC2 pins (and the
 * optional PC3 pin, if configured).
 *
 * For simplicity, to set a PM mode, we write it to all available
 * PMMODE registers.
 */
static int
_bcm590xx_set_pmmode(struct bcm590xx_reg *pmu, int reg_id, unsigned int mode)
{
	struct regmap *regmap;
	u8 pmctrl_addr = bcm590xx_get_pmctrl_register(pmu, reg_id);
	unsigned int i;
	int pmctrl_count;
	int mode_mask;
	int ret;

	/*
	 * Regulators using 2-bit mode controls have 2 PMCTRL registers;
	 * regulators using 3-bit mode controls have 4 PMCTRL registers.
	 * This is to accomodate all 7 selectable modes.
	 */
	if (bcm590xx_reg_mode_is_3bit(pmu, reg_id)) {
		pmctrl_count = 4;
		mode_mask = PMMODE_3BIT_MASK(mode);
	} else {
		pmctrl_count = 2;
		mode_mask = PMMODE_2BIT_MASK(mode);
	}

	if (bcm590xx_reg_is_secondary(pmu, reg_id))
		regmap = pmu->mfd->regmap_sec;
	else
		regmap = pmu->mfd->regmap_pri;

	for (i = 0; i < pmctrl_count; i++) {
		ret = regmap_write(regmap, pmctrl_addr + i, mode_mask);
		if (ret)
			return ret;
	}

	return 0;
}

static int bcm590xx_regulator_enable(struct regulator_dev *rdev)
{
	struct bcm590xx_reg *pmu = rdev->reg_data;

	return _bcm590xx_set_pmmode(pmu, rdev->desc->id, BCM590XX_PMMODE_ON);
}

static int bcm590xx_regulator_disable(struct regulator_dev *rdev)
{
	struct bcm590xx_reg *pmu = rdev->reg_data;

	return _bcm590xx_set_pmmode(pmu, rdev->desc->id, BCM590XX_PMMODE_OFF);
}

static const struct regulator_ops bcm590xx_ops_ldo = {
	.is_enabled		= regulator_is_enabled_regmap,
	.enable			= bcm590xx_regulator_enable,
	.disable		= bcm590xx_regulator_disable,
	.get_voltage_sel	= regulator_get_voltage_sel_regmap,
	.set_voltage_sel	= regulator_set_voltage_sel_regmap,
	.list_voltage		= regulator_list_voltage_table,
	.map_voltage		= regulator_map_voltage_iterate,
};

static const struct regulator_ops bcm590xx_ops_dcdc = {
	.is_enabled		= regulator_is_enabled_regmap,
	.enable			= bcm590xx_regulator_enable,
	.disable		= bcm590xx_regulator_disable,
	.get_voltage_sel	= regulator_get_voltage_sel_regmap,
	.set_voltage_sel	= regulator_set_voltage_sel_regmap,
	.list_voltage		= regulator_list_voltage_linear_range,
	.map_voltage		= regulator_map_voltage_linear_range,
};

static const struct regulator_ops bcm590xx_ops_static = {
	.is_enabled		= regulator_is_enabled_regmap,
	.enable			= bcm590xx_regulator_enable,
	.disable		= bcm590xx_regulator_disable,
};

static int bcm590xx_probe(struct platform_device *pdev)
{
	struct bcm590xx *bcm590xx = dev_get_drvdata(pdev->dev.parent);
	struct bcm590xx_reg *pmu;
	struct regulator_config config = { };
	struct bcm590xx_info *info;
	struct regulator_dev *rdev;
	int n_regulators;
	int i;

	pmu = devm_kzalloc(&pdev->dev, sizeof(*pmu), GFP_KERNEL);
	if (!pmu)
		return -ENOMEM;

	pmu->mfd = bcm590xx;

	platform_set_drvdata(pdev, pmu);

	if (pmu->mfd->device_type == BCM59054_TYPE) {
		info = bcm59054_regs;
		n_regulators = BCM59054_NUM_REGS;
	} else if (pmu->mfd->device_type == BCM59056_TYPE) {
		info = bcm59056_regs;
		n_regulators = BCM59056_NUM_REGS;
	}

	pmu->desc = devm_kcalloc(&pdev->dev,
				 n_regulators,
				 sizeof(struct regulator_desc),
				 GFP_KERNEL);
	if (!pmu->desc)
		return -ENOMEM;

	/* Register the regulators */
	for (i = 0; i < n_regulators; i++, info++) {
		pmu->desc[i].name = info->name;
		pmu->desc[i].of_match = of_match_ptr(info->name);
		pmu->desc[i].regulators_node = of_match_ptr("regulators");
		pmu->desc[i].supply_name = info->vin_name;
		pmu->desc[i].id = i;
		pmu->desc[i].volt_table = info->volt_table;
		pmu->desc[i].n_voltages = info->n_voltages;
		pmu->desc[i].linear_ranges = info->linear_ranges;
		pmu->desc[i].n_linear_ranges = info->n_linear_ranges;

		if (bcm590xx_reg_is_ldo(pmu, i) || \
				bcm590xx_reg_is_gpldo(pmu, i)) {
			pmu->desc[i].ops = &bcm590xx_ops_ldo;
			pmu->desc[i].vsel_mask = BCM590XX_LDO_VSEL_MASK;
		} else if (bcm590xx_reg_is_static(pmu, i)) {
			pmu->desc[i].ops = &bcm590xx_ops_static;
		} else {
			pmu->desc[i].ops = &bcm590xx_ops_dcdc;
			pmu->desc[i].vsel_mask = BCM590XX_SR_VSEL_MASK;
		}

		if (bcm590xx_reg_is_vbus(pmu, i)) {
			pmu->desc[i].enable_mask = BCM590XX_VBUS_ENABLE;
		} else {
			pmu->desc[i].vsel_reg = \
				bcm590xx_get_vsel_register(pmu, i);
			pmu->desc[i].enable_mask = \
				bcm590xx_get_enable_mask(pmu, i);
			pmu->desc[i].enable_is_inverted = true;
		}
		pmu->desc[i].enable_reg = \
			bcm590xx_get_pmctrl_register(pmu, i);
		pmu->desc[i].type = REGULATOR_VOLTAGE;
		pmu->desc[i].owner = THIS_MODULE;

		config.dev = pmu->mfd->dev;
		config.driver_data = pmu;
		if (bcm590xx_reg_is_secondary(pmu, i))
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
		.probe_type = PROBE_PREFER_ASYNCHRONOUS,
	},
	.probe = bcm590xx_probe,
};
module_platform_driver(bcm590xx_regulator_driver);

MODULE_AUTHOR("Matt Porter <mporter@linaro.org>");
MODULE_DESCRIPTION("BCM590xx voltage regulator driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:bcm590xx-vregs");
