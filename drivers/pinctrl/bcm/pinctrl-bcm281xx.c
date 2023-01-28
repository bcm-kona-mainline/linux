// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2013-2017 Broadcom

#include <linux/err.h>
#include <linux/io.h>
#include <linux/init.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/pinctrl/pinconf-generic.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include "../core.h"
#include "../pinctrl-utils.h"
#include "pinctrl-bcm281xx.h"

/* Function Select bits are the same for all pin control registers */
#define BCM281XX_PIN_REG_F_SEL_MASK		0x0700
#define BCM281XX_PIN_REG_F_SEL_SHIFT		8

/* Standard pin register */
#define BCM281XX_STD_PIN_REG_DRV_STR_MASK	0x0007
#define BCM281XX_STD_PIN_REG_DRV_STR_SHIFT	0
#define BCM281XX_STD_PIN_REG_INPUT_DIS_MASK	0x0008
#define BCM281XX_STD_PIN_REG_INPUT_DIS_SHIFT	3
#define BCM281XX_STD_PIN_REG_SLEW_MASK		0x0010
#define BCM281XX_STD_PIN_REG_SLEW_SHIFT		4
#define BCM281XX_STD_PIN_REG_PULL_UP_MASK	0x0020
#define BCM281XX_STD_PIN_REG_PULL_UP_SHIFT	5
#define BCM281XX_STD_PIN_REG_PULL_DN_MASK	0x0040
#define BCM281XX_STD_PIN_REG_PULL_DN_SHIFT	6
#define BCM281XX_STD_PIN_REG_HYST_MASK		0x0080
#define BCM281XX_STD_PIN_REG_HYST_SHIFT		7

/* I2C pin register */
#define BCM281XX_I2C_PIN_REG_INPUT_DIS_MASK	0x0004
#define BCM281XX_I2C_PIN_REG_INPUT_DIS_SHIFT	2
#define BCM281XX_I2C_PIN_REG_SLEW_MASK		0x0008
#define BCM281XX_I2C_PIN_REG_SLEW_SHIFT		3
#define BCM281XX_I2C_PIN_REG_PULL_UP_STR_MASK	0x0070
#define BCM281XX_I2C_PIN_REG_PULL_UP_STR_SHIFT	4

/* HDMI pin register */
#define BCM281XX_HDMI_PIN_REG_INPUT_DIS_MASK	0x0008
#define BCM281XX_HDMI_PIN_REG_INPUT_DIS_SHIFT	3
#define BCM281XX_HDMI_PIN_REG_MODE_MASK		0x0010
#define BCM281XX_HDMI_PIN_REG_MODE_SHIFT	4

/* BCM21664 I2C pins are slightly different from BCM218xx: */
#define BCM21664_I2C_PIN_REG_INPUT_DIS_MASK	0x0008
#define BCM21664_I2C_PIN_REG_INPUT_DIS_SHIFT	3
#define BCM21664_I2C_PIN_REG_SLEW_MASK		0x0010
#define BCM21664_I2C_PIN_REG_SLEW_SHIFT		4
#define BCM21664_I2C_PIN_REG_PULL_UP_STR_MASK	0x0020
#define BCM21664_I2C_PIN_REG_PULL_UP_STR_SHIFT	5

/* BCM21664 access lock registers */
#define BCM21664_WR_ACCESS_PASSWORD		0xA5A501
#define BCM21664_WR_ACCESS_OFFSET		0x07F0
#define BCM21664_ACCESS_LOCK_OFFSET(lock)	(0x0780 + (lock * 4))
#define BCM21664_ACCESS_LOCK_COUNT		5

static inline enum bcm281xx_pin_type pin_type_get(struct pinctrl_dev *pctldev,
						  unsigned pin)
{
	struct bcm281xx_pinctrl_data *pdata = pinctrl_dev_get_drvdata(pctldev);

	if (pin >= pdata->drv_data->npins)
		return BCM281XX_PIN_TYPE_UNKNOWN;

	return *(enum bcm281xx_pin_type *)(pdata->drv_data->pins[pin].drv_data);
}

#define BCM281XX_PIN_SHIFT(type, param) \
	(BCM281XX_ ## type ## _PIN_REG_ ## param ## _SHIFT)

#define BCM281XX_PIN_MASK(type, param) \
	(BCM281XX_ ## type ## _PIN_REG_ ## param ## _MASK)

/* Only used for I2C pins */
#define BCM21664_PIN_SHIFT(type, param) \
	(BCM21664_ ## type ## _PIN_REG_ ## param ## _SHIFT)

#define BCM21664_PIN_MASK(type, param) \
	(BCM21664_ ## type ## _PIN_REG_ ## param ## _MASK)

/*
 * This helper function is used to build up the value and mask used to write to
 * a pin register, but does not actually write to the register.
 */
static inline void bcm281xx_pin_update(u32 *reg_val, u32 *reg_mask,
				       u32 param_val, u32 param_shift,
				       u32 param_mask)
{
	*reg_val &= ~param_mask;
	*reg_val |= (param_val << param_shift) & param_mask;
	*reg_mask |= param_mask;
}

static int bcm281xx_pinctrl_get_groups_count(struct pinctrl_dev *pctldev)
{
	struct bcm281xx_pinctrl_data *pdata = pinctrl_dev_get_drvdata(pctldev);

	return pdata->drv_data->npins;
}

static const char *bcm281xx_pinctrl_get_group_name(struct pinctrl_dev *pctldev,
						   unsigned group)
{
	struct bcm281xx_pinctrl_data *pdata = pinctrl_dev_get_drvdata(pctldev);

	return pdata->drv_data->pins[group].name;
}

static int bcm281xx_pinctrl_get_group_pins(struct pinctrl_dev *pctldev,
					   unsigned group,
					   const unsigned **pins,
					   unsigned *num_pins)
{
	struct bcm281xx_pinctrl_data *pdata = pinctrl_dev_get_drvdata(pctldev);

	*pins = &pdata->drv_data->pins[group].number;
	*num_pins = 1;

	return 0;
}

static void bcm281xx_pinctrl_pin_dbg_show(struct pinctrl_dev *pctldev,
					  struct seq_file *s,
					  unsigned offset)
{
	seq_printf(s, " %s", dev_name(pctldev->dev));
}

static const struct pinctrl_ops bcm281xx_pinctrl_ops = {
	.get_groups_count = bcm281xx_pinctrl_get_groups_count,
	.get_group_name = bcm281xx_pinctrl_get_group_name,
	.get_group_pins = bcm281xx_pinctrl_get_group_pins,
	.pin_dbg_show = bcm281xx_pinctrl_pin_dbg_show,
	.dt_node_to_map = pinconf_generic_dt_node_to_map_pin,
	.dt_free_map = pinctrl_utils_free_map,
};

static int bcm281xx_pinctrl_get_fcns_count(struct pinctrl_dev *pctldev)
{
	struct bcm281xx_pinctrl_data *pdata = pinctrl_dev_get_drvdata(pctldev);

	return pdata->drv_data->nfunctions;
}

static const char *bcm281xx_pinctrl_get_fcn_name(struct pinctrl_dev *pctldev,
						 unsigned function)
{
	struct bcm281xx_pinctrl_data *pdata = pinctrl_dev_get_drvdata(pctldev);

	return pdata->drv_data->functions[function].name;
}

static int bcm281xx_pinctrl_get_fcn_groups(struct pinctrl_dev *pctldev,
					   unsigned function,
					   const char * const **groups,
					   unsigned * const num_groups)
{
	struct bcm281xx_pinctrl_data *pdata = pinctrl_dev_get_drvdata(pctldev);

	*groups = pdata->drv_data->functions[function].groups;
	*num_groups = pdata->drv_data->functions[function].ngroups;

	return 0;
}

static int bcm281xx_pinmux_set(struct pinctrl_dev *pctldev,
			       unsigned function,
			       unsigned group)
{
	struct bcm281xx_pinctrl_data *pdata = pinctrl_dev_get_drvdata(pctldev);
	const struct bcm281xx_pin_function *f = &pdata->drv_data->functions[function];
	u32 offset = 4 * pdata->drv_data->pins[group].number;
	int rc = 0;

	dev_dbg(pctldev->dev,
		"%s(): Enable function %s (%d) of pin %s (%d) @offset 0x%x.\n",
		__func__, f->name, function, pdata->drv_data->pins[group].name,
		pdata->drv_data->pins[group].number, offset);

	rc = regmap_update_bits(pdata->regmap, offset,
		BCM281XX_PIN_REG_F_SEL_MASK,
		function << BCM281XX_PIN_REG_F_SEL_SHIFT);
	if (rc)
		dev_err(pctldev->dev,
			"Error updating register for pin %s (%d).\n",
			pdata->drv_data->pins[group].name, pdata->drv_data->pins[group].number);

	return rc;
}

static const struct pinmux_ops bcm281xx_pinctrl_pinmux_ops = {
	.get_functions_count = bcm281xx_pinctrl_get_fcns_count,
	.get_function_name = bcm281xx_pinctrl_get_fcn_name,
	.get_function_groups = bcm281xx_pinctrl_get_fcn_groups,
	.set_mux = bcm281xx_pinmux_set,
};

static int bcm281xx_pinctrl_pin_config_get(struct pinctrl_dev *pctldev,
					   unsigned pin,
					   unsigned long *config)
{
	return -ENOTSUPP;
}


/* Goes through the configs and update register val/mask */
static int bcm281xx_std_pin_update(struct pinctrl_dev *pctldev,
				   unsigned pin,
				   unsigned long *configs,
				   unsigned num_configs,
				   u32 *val,
				   u32 *mask)
{
	struct bcm281xx_pinctrl_data *pdata = pinctrl_dev_get_drvdata(pctldev);
	int i;
	enum pin_config_param param;
	u32 arg;

	for (i = 0; i < num_configs; i++) {
		param = pinconf_to_config_param(configs[i]);
		arg = pinconf_to_config_argument(configs[i]);

		switch (param) {
		case PIN_CONFIG_INPUT_SCHMITT_ENABLE:
			arg = (arg >= 1 ? 1 : 0);
			bcm281xx_pin_update(val, mask, arg,
				BCM281XX_PIN_SHIFT(STD, HYST),
				BCM281XX_PIN_MASK(STD, HYST));
			break;
		/*
		 * The pin bias can only be one of pull-up, pull-down, or
		 * disable.  The user does not need to specify a value for the
		 * property, and the default value from pinconf-generic is
		 * ignored.
		 */
		case PIN_CONFIG_BIAS_DISABLE:
			bcm281xx_pin_update(val, mask, 0,
				BCM281XX_PIN_SHIFT(STD, PULL_UP),
				BCM281XX_PIN_MASK(STD, PULL_UP));
			bcm281xx_pin_update(val, mask, 0,
				BCM281XX_PIN_SHIFT(STD, PULL_DN),
				BCM281XX_PIN_MASK(STD, PULL_DN));
			break;

		case PIN_CONFIG_BIAS_PULL_UP:
			bcm281xx_pin_update(val, mask, 1,
				BCM281XX_PIN_SHIFT(STD, PULL_UP),
				BCM281XX_PIN_MASK(STD, PULL_UP));
			bcm281xx_pin_update(val, mask, 0,
				BCM281XX_PIN_SHIFT(STD, PULL_DN),
				BCM281XX_PIN_MASK(STD, PULL_DN));
			break;

		case PIN_CONFIG_BIAS_PULL_DOWN:
			bcm281xx_pin_update(val, mask, 0,
				BCM281XX_PIN_SHIFT(STD, PULL_UP),
				BCM281XX_PIN_MASK(STD, PULL_UP));
			bcm281xx_pin_update(val, mask, 1,
				BCM281XX_PIN_SHIFT(STD, PULL_DN),
				BCM281XX_PIN_MASK(STD, PULL_DN));
			break;

		case PIN_CONFIG_SLEW_RATE:
			arg = (arg >= 1 ? 1 : 0);
			bcm281xx_pin_update(val, mask, arg,
				BCM281XX_PIN_SHIFT(STD, SLEW),
				BCM281XX_PIN_MASK(STD, SLEW));
			break;

		case PIN_CONFIG_INPUT_ENABLE:
			/* inversed since register is for input _disable_ */
			arg = (arg >= 1 ? 0 : 1);
			bcm281xx_pin_update(val, mask, arg,
				BCM281XX_PIN_SHIFT(STD, INPUT_DIS),
				BCM281XX_PIN_MASK(STD, INPUT_DIS));
			break;

		case PIN_CONFIG_DRIVE_STRENGTH:
			/* Valid range is 2-16 mA, even numbers only */
			if ((arg < 2) || (arg > 16) || (arg % 2)) {
				dev_err(pctldev->dev,
					"Invalid Drive Strength value (%d) for "
					"pin %s (%d). Valid values are "
					"(2..16) mA, even numbers only.\n",
					arg, pdata->drv_data->pins[pin].name, pin);
				return -EINVAL;
			}
			bcm281xx_pin_update(val, mask, (arg/2)-1,
				BCM281XX_PIN_SHIFT(STD, DRV_STR),
				BCM281XX_PIN_MASK(STD, DRV_STR));
			break;

		default:
			dev_err(pctldev->dev,
				"Unrecognized pin config %d for pin %s (%d).\n",
				param, pdata->drv_data->pins[pin].name, pin);
			return -EINVAL;

		} /* switch config */
	} /* for each config */

	return 0;
}

/*
 * The pull-up strength for an I2C pin is represented by bits 4-6 in the
 * register with the following mapping:
 *   0b000: No pull-up
 *   0b001: 1200 Ohm
 *   0b010: 1800 Ohm
 *   0b011: 720 Ohm
 *   0b100: 2700 Ohm
 *   0b101: 831 Ohm
 *   0b110: 1080 Ohm
 *   0b111: 568 Ohm
 * This array maps pull-up strength in Ohms to register values (1+index).
 */
static const u16 bcm281xx_pullup_map[] = {
	1200, 1800, 720, 2700, 831, 1080, 568
};

/* Goes through the configs and update register val/mask */
static int bcm281xx_i2c_pin_update(struct pinctrl_dev *pctldev,
				   unsigned pin,
				   unsigned long *configs,
				   unsigned num_configs,
				   u32 *val,
				   u32 *mask)
{
	struct bcm281xx_pinctrl_data *pdata = pinctrl_dev_get_drvdata(pctldev);
	int dev_type = pdata->drv_data->device_type;
	int i, j;
	enum pin_config_param param;
	u32 arg;

	for (i = 0; i < num_configs; i++) {
		param = pinconf_to_config_param(configs[i]);
		arg = pinconf_to_config_argument(configs[i]);

		switch (param) {
		case PIN_CONFIG_BIAS_PULL_UP:
			for (j = 0; j < ARRAY_SIZE(bcm281xx_pullup_map); j++)
				if (bcm281xx_pullup_map[j] == arg)
					break;

			if (j == ARRAY_SIZE(bcm281xx_pullup_map)) {
				dev_err(pctldev->dev,
					"Invalid pull-up value (%d) for pin %s "
					"(%d). Valid values are 568, 720, 831, "
					"1080, 1200, 1800, 2700 Ohms.\n",
					arg, pdata->drv_data->pins[pin].name, pin);
				return -EINVAL;
			}

			if (dev_type == BCM281XX_PINCTRL_TYPE) {
				bcm281xx_pin_update(val, mask, j+1,
					BCM281XX_PIN_SHIFT(I2C, PULL_UP_STR),
					BCM281XX_PIN_MASK(I2C, PULL_UP_STR));
			} else if (dev_type == BCM21664_PINCTRL_TYPE) {
				bcm281xx_pin_update(val, mask, j+1,
					BCM21664_PIN_SHIFT(I2C, PULL_UP_STR),
					BCM21664_PIN_MASK(I2C, PULL_UP_STR));
			}
			break;

		case PIN_CONFIG_BIAS_DISABLE:
			if (dev_type == BCM281XX_PINCTRL_TYPE) {
				bcm281xx_pin_update(val, mask, 0,
					BCM281XX_PIN_SHIFT(I2C, PULL_UP_STR),
					BCM281XX_PIN_MASK(I2C, PULL_UP_STR));
			} else if (dev_type == BCM21664_PINCTRL_TYPE) {
				bcm281xx_pin_update(val, mask, 0,
					BCM21664_PIN_SHIFT(I2C, PULL_UP_STR),
					BCM21664_PIN_MASK(I2C, PULL_UP_STR));
			}
			break;

		case PIN_CONFIG_SLEW_RATE:
			arg = (arg >= 1 ? 1 : 0);
			if (dev_type == BCM281XX_PINCTRL_TYPE) {
				bcm281xx_pin_update(val, mask, arg,
					BCM281XX_PIN_SHIFT(I2C, SLEW),
					BCM281XX_PIN_MASK(I2C, SLEW));
			} else if (dev_type == BCM21664_PINCTRL_TYPE) {
				bcm281xx_pin_update(val, mask, arg,
					BCM21664_PIN_SHIFT(I2C, SLEW),
					BCM21664_PIN_MASK(I2C, SLEW));
			}
			break;

		case PIN_CONFIG_INPUT_ENABLE:
			/* inversed since register is for input _disable_ */
			arg = (arg >= 1 ? 0 : 1);
			if (dev_type == BCM281XX_PINCTRL_TYPE) {
				bcm281xx_pin_update(val, mask, arg,
					BCM281XX_PIN_SHIFT(I2C, INPUT_DIS),
					BCM281XX_PIN_MASK(I2C, INPUT_DIS));
			} else if (dev_type == BCM21664_PINCTRL_TYPE) {
				bcm281xx_pin_update(val, mask, arg,
					BCM21664_PIN_SHIFT(I2C, INPUT_DIS),
					BCM21664_PIN_MASK(I2C, INPUT_DIS));
			}
			break;

		default:
			dev_err(pctldev->dev,
				"Unrecognized pin config %d for pin %s (%d).\n",
				param, pdata->drv_data->pins[pin].name, pin);
			return -EINVAL;

		} /* switch config */
	} /* for each config */

	return 0;
}

/* Goes through the configs and update register val/mask */
static int bcm281xx_hdmi_pin_update(struct pinctrl_dev *pctldev,
				    unsigned pin,
				    unsigned long *configs,
				    unsigned num_configs,
				    u32 *val,
				    u32 *mask)
{
	struct bcm281xx_pinctrl_data *pdata = pinctrl_dev_get_drvdata(pctldev);
	int i;
	enum pin_config_param param;
	u32 arg;

	for (i = 0; i < num_configs; i++) {
		param = pinconf_to_config_param(configs[i]);
		arg = pinconf_to_config_argument(configs[i]);

		switch (param) {
		case PIN_CONFIG_SLEW_RATE:
			arg = (arg >= 1 ? 1 : 0);
			bcm281xx_pin_update(val, mask, arg,
				BCM281XX_PIN_SHIFT(HDMI, MODE),
				BCM281XX_PIN_MASK(HDMI, MODE));
			break;

		case PIN_CONFIG_INPUT_ENABLE:
			/* inversed since register is for input _disable_ */
			arg = (arg >= 1 ? 0 : 1);
			bcm281xx_pin_update(val, mask, arg,
				BCM281XX_PIN_SHIFT(HDMI, INPUT_DIS),
				BCM281XX_PIN_MASK(HDMI, INPUT_DIS));
			break;

		default:
			dev_err(pctldev->dev,
				"Unrecognized pin config %d for pin %s (%d).\n",
				param, pdata->drv_data->pins[pin].name, pin);
			return -EINVAL;

		} /* switch config */
	} /* for each config */

	return 0;
}

static int bcm281xx_pinctrl_pin_config_set(struct pinctrl_dev *pctldev,
					   unsigned pin,
					   unsigned long *configs,
					   unsigned num_configs)
{
	struct bcm281xx_pinctrl_data *pdata = pinctrl_dev_get_drvdata(pctldev);
	enum bcm281xx_pin_type pin_type;
	u32 offset = 4 * pin;
	u32 cfg_val, cfg_mask;
	int rc;

	cfg_val = 0;
	cfg_mask = 0;
	pin_type = pin_type_get(pctldev, pin);

	/* Different pins have different configuration options */
	switch (pin_type) {
	case BCM281XX_PIN_TYPE_STD:
		rc = bcm281xx_std_pin_update(pctldev, pin, configs,
			num_configs, &cfg_val, &cfg_mask);
		break;

	case BCM281XX_PIN_TYPE_I2C:
		rc = bcm281xx_i2c_pin_update(pctldev, pin, configs,
			num_configs, &cfg_val, &cfg_mask);
		break;

	case BCM281XX_PIN_TYPE_HDMI:
		rc = bcm281xx_hdmi_pin_update(pctldev, pin, configs,
			num_configs, &cfg_val, &cfg_mask);
		break;

	default:
		dev_err(pctldev->dev, "Unknown pin type for pin %s (%d).\n",
			pdata->drv_data->pins[pin].name, pin);
		return -EINVAL;

	} /* switch pin type */

	if (rc)
		return rc;

	dev_dbg(pctldev->dev,
		"%s(): Set pin %s (%d) with config 0x%x, mask 0x%x\n",
		__func__, pdata->drv_data->pins[pin].name, pin, cfg_val, cfg_mask);

	rc = regmap_update_bits(pdata->regmap, offset, cfg_mask, cfg_val);
	if (rc) {
		dev_err(pctldev->dev,
			"Error updating register for pin %s (%d).\n",
			pdata->drv_data->pins[pin].name, pin);
		return rc;
	}

	return 0;
}

static const struct pinconf_ops bcm281xx_pinctrl_pinconf_ops = {
	.pin_config_get = bcm281xx_pinctrl_pin_config_get,
	.pin_config_set = bcm281xx_pinctrl_pin_config_set,
};

static struct pinctrl_desc bcm281xx_pinctrl_desc = {
	/* name, pins, npins members initialized in probe function */
	.pctlops = &bcm281xx_pinctrl_ops,
	.pmxops = &bcm281xx_pinctrl_pinmux_ops,
	.confops = &bcm281xx_pinctrl_pinconf_ops,
	.owner = THIS_MODULE,
};

/* BCM21664 padctrl has access lock registers, this function unlocks them */
static void bcm21664_pinctrl_unlock(struct bcm281xx_pinctrl_data *pdata) {
	int i;

	for (i = 0; i < BCM21664_ACCESS_LOCK_COUNT; i++) {
		writel(BCM21664_WR_ACCESS_PASSWORD,
			pdata->reg_base + BCM21664_WR_ACCESS_OFFSET);
		writel(0x0, pdata->reg_base + BCM21664_ACCESS_LOCK_OFFSET(i));
	}
};

static struct bcm281xx_pinctrl_data bcm281xx_pinctrl_pdata;

static int __init bcm281xx_pinctrl_probe(struct platform_device *pdev)
{
	const struct bcm281xx_pinctrl_drv_data *drv_data;
	struct bcm281xx_pinctrl_data *pdata = &bcm281xx_pinctrl_pdata;
	struct pinctrl_dev *pctl;

	drv_data = of_device_get_match_data(&pdev->dev);
	pdata->drv_data = drv_data;

	BUG_ON(drv_data->device_type != BCM281XX_PINCTRL_TYPE && \
		drv_data->device_type != BCM21664_PINCTRL_TYPE);

	/* So far We can assume there is only 1 bank of registers */
	pdata->reg_base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(pdata->reg_base)) {
		dev_err(&pdev->dev, "Failed to ioremap MEM resource\n");
		return PTR_ERR(pdata->reg_base);
	}

	/* Initialize the dynamic part of pinctrl_desc */
	pdata->regmap = devm_regmap_init_mmio(&pdev->dev, pdata->reg_base,
		&drv_data->regmap_config);
	if (IS_ERR(pdata->regmap)) {
		dev_err(&pdev->dev, "Regmap MMIO init failed.\n");
		return -ENODEV;
	}

	bcm281xx_pinctrl_desc.name = dev_name(&pdev->dev);
	bcm281xx_pinctrl_desc.pins = drv_data->pins;
	bcm281xx_pinctrl_desc.npins = drv_data->npins;

	pctl = devm_pinctrl_register(&pdev->dev, &bcm281xx_pinctrl_desc, pdata);
	if (IS_ERR(pctl)) {
		dev_err(&pdev->dev, "Failed to register pinctrl\n");
		return PTR_ERR(pctl);
	}

	if (pdata->drv_data->device_type == BCM21664_PINCTRL_TYPE)
		bcm21664_pinctrl_unlock(pdata);

	platform_set_drvdata(pdev, pdata);

	return 0;
}

static const struct of_device_id bcm281xx_pinctrl_of_match[] = {
	{ .compatible = "brcm,bcm11351-pinctrl", .data = &bcm281xx_pinctrl},
	{ .compatible = "brcm,bcm21664-pinctrl", .data = &bcm21664_pinctrl},
	{ },
};

static struct platform_driver bcm281xx_pinctrl_driver = {
	.driver = {
		.name = "bcm281xx-pinctrl",
		.of_match_table = bcm281xx_pinctrl_of_match,
	},
};
builtin_platform_driver_probe(bcm281xx_pinctrl_driver, bcm281xx_pinctrl_probe);
