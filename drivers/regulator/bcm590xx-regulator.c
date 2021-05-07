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
#include <linux/printk.h>
#include "bcm59056-regulator.h"
#include "bcm59054-regulator.h"

const char *device_type;

static int bcm590xx_reg_is_ldo(int id)
{
	if (strcmp(device_type, "bcm59056")) {
		return BCM59056_REG_IS_LDO(id);
	} else if (strcmp(device_type, "bcm59054")) {
		return BCM59054_REG_IS_LDO(id);
	}
	return 0;
}

static int bcm590xx_reg_is_gpldo(int id)
{
	if (strcmp(device_type, "bcm59056")) {
		return BCM59056_REG_IS_GPLDO(id);
	} else if (strcmp(device_type, "bcm59054")) {
		return BCM59054_REG_IS_GPLDO(id);
	}
	return 0;
}

static int bcm590xx_reg_is_vbus(int id)
{
	if (strcmp(device_type, "bcm59056")) {
		return BCM59056_REG_IS_VBUS(id);
	} else if (strcmp(device_type, "bcm59054")) {
		/*return BCM59054_REG_IS_GPLDO(id);*/
		return 0;
	}
	return 0;
}

static int bcm590xx_get_vsel_register(int id)
{
	if (strcmp(device_type, "bcm59056")) {
		if (BCM59056_REG_IS_LDO(id))
			return BCM59056_RFLDOCTRL + id;
		else if (BCM59056_REG_IS_GPLDO(id))
			return BCM59056_GPLDO1CTRL + id;
		else
			return BCM59056_CSRVOUT1 + (id - BCM59056_REG_CSR) * 3;
	} else if (strcmp(device_type, "bcm59054")) {
		if (BCM59054_REG_IS_LDO(id))
			return BCM59054_RFLDOCTRL + id;
		else if (BCM59054_REG_IS_GPLDO(id))
			return BCM59054_GPLDO1CTRL + id;
		else
			return BCM59054_CSRVOUT1 + (id - BCM59054_REG_CSR) * 3;
	}
	return 0;
}

static int bcm590xx_get_enable_register(int id)
{
	int reg = 0;

	if (strcmp(device_type, "bcm59056")) {
		if (BCM59056_REG_IS_LDO(id))
			reg = BCM59056_RFLDOPMCTRL1 + id * 2;
		else if (BCM59056_REG_IS_GPLDO(id))
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
	} else if (strcmp(device_type, "bcm59054")) {
		if (BCM59054_REG_IS_LDO(id))
			reg = BCM59054_RFLDOPMCTRL1 + id * 2;
		else if (BCM59054_REG_IS_GPLDO(id))
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
	}

	return reg;
}

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

static const struct regulator_ops bcm590xx_ops_vbus = {
	.is_enabled		= regulator_is_enabled_regmap,
	.enable			= regulator_enable_regmap,
	.disable		= regulator_disable_regmap,
};

static int bcm590xx_probe(struct platform_device *pdev)
{
	struct bcm590xx *bcm590xx = dev_get_drvdata(pdev->dev.parent);
	struct bcm590xx_reg *pmu;
	struct regulator_config config = { };
	struct bcm590xx_info *info;
	struct regulator_dev *rdev;
	int num_regs;
	int i;

	device_type = bcm590xx->device_type;

	pmu = devm_kzalloc(&pdev->dev, sizeof(*pmu), GFP_KERNEL);
	if (!pmu)
		return -ENOMEM;

	pmu->mfd = bcm590xx;

	if (strcmp(device_type, "bcm59056")) {
		info = bcm59056_regs;
		num_regs = BCM59056_NUM_REGS;
	} else if (strcmp(device_type, "bcm59054")) {
		info = bcm59054_regs;
		num_regs = BCM59054_NUM_REGS;
	}

	platform_set_drvdata(pdev, pmu);

	pmu->desc = devm_kcalloc(&pdev->dev,
				 num_regs,
				 sizeof(struct regulator_desc),
				 GFP_KERNEL);
	if (!pmu->desc)
		return -ENOMEM;

	for (i = 0; i < num_regs; i++, info++) {
		/* Register the regulators */
		pmu->desc[i].name = info->name;
		pmu->desc[i].of_match = of_match_ptr(info->name);
		pmu->desc[i].regulators_node = of_match_ptr("regulators");
		pmu->desc[i].supply_name = info->vin_name;
		pmu->desc[i].id = i;
		pmu->desc[i].volt_table = info->volt_table;
		pmu->desc[i].n_voltages = info->n_voltages;
		pmu->desc[i].linear_ranges = info->linear_ranges;
		pmu->desc[i].n_linear_ranges = info->n_linear_ranges;

		if ((bcm590xx_reg_is_ldo(i)) || (bcm590xx_reg_is_gpldo(i))) {
			pmu->desc[i].ops = &bcm590xx_ops_ldo;
			pmu->desc[i].vsel_mask = BCM590XX_LDO_VSEL_MASK;
		} else if (strcmp(device_type, "bcm59056") && bcm590xx_reg_is_vbus(i))
			pmu->desc[i].ops = &bcm590xx_ops_vbus;
		else if (strcmp(device_type, "bcm59054") && i == BCM59054_REG_MICLDO)
			pmu->desc[i].ops = &bcm590xx_ops_vbus;
		else {
			pmu->desc[i].ops = &bcm590xx_ops_dcdc;
			pmu->desc[i].vsel_mask = BCM590XX_SR_VSEL_MASK;
		}

		if (strcmp(device_type, "bcm59056") && bcm590xx_reg_is_vbus(i))
			pmu->desc[i].enable_mask = BCM590XX_VBUS_ENABLE;
		else {
			pmu->desc[i].vsel_reg = bcm590xx_get_vsel_register(i);
			pmu->desc[i].enable_is_inverted = true;
			pmu->desc[i].enable_mask = BCM590XX_REG_ENABLE;
		}
		pmu->desc[i].enable_reg = bcm590xx_get_enable_register(i);
		pmu->desc[i].type = REGULATOR_VOLTAGE;
		pmu->desc[i].owner = THIS_MODULE;

		config.dev = bcm590xx->dev;
		config.driver_data = pmu;
		if (bcm590xx_reg_is_gpldo(i) || bcm590xx_reg_is_vbus(i))
			config.regmap = bcm590xx->regmap_sec;
		else
			config.regmap = bcm590xx->regmap_pri;

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
