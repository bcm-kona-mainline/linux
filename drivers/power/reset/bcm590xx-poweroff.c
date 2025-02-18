// SPDX-License-Identifier: GPL-2.0-only
/*
 * Poweroff/shutdown handler for Broadcom BCM590XX PMUs
 *
 * Copyright (c) 2025 Artur Weber <aweber.kernel@gmail.com>
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>
#include <linux/module.h>
#include <linux/reboot.h>
#include <linux/mfd/bcm590xx.h>

/* Under primary I2C address: */
#define BCM590XX_REG_HOSTCTRL1		0x01
#define BCM590XX_HOSTCTRL1_SHDWN_SHIFT	2
#define BCM590XX_HOSTCTRL1_SHDWN_MASK	(1 << BCM590XX_HOSTCTRL1_SHDWN_SHIFT)

static int bcm590xx_poweroff_do_poweroff(struct sys_off_data *data)
{
	struct bcm590xx *mfd = data->cb_data;
	int ret = 0;

	ret = regmap_update_bits(mfd->regmap_pri, BCM590XX_REG_HOSTCTRL1,
				 BCM590XX_HOSTCTRL1_SHDWN_MASK,
				 BCM590XX_HOSTCTRL1_SHDWN_MASK);

	if (ret)
		dev_err(data->dev, "Failed to write shutdown bit: %d\n", ret);

	return ret;
}

static int bcm590xx_poweroff_probe(struct platform_device *pdev)
{
	struct bcm590xx *bcm590xx = dev_get_drvdata(pdev->dev.parent);

	return devm_register_sys_off_handler(&pdev->dev,
					     SYS_OFF_MODE_POWER_OFF,
					     SYS_OFF_PRIO_HIGH,
					     bcm590xx_poweroff_do_poweroff,
					     bcm590xx);
}

static const struct platform_device_id bcm590xx_poweroff_id_table[] = {
	{ "bcm590xx-poweroff", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(platform, bcm590xx_poweroff_id_table);

static struct platform_driver bcm590xx_poweroff_driver = {
	.probe = bcm590xx_poweroff_probe,
	.driver = {
		.name = "bcm590xx-poweroff",
	},
	.id_table = bcm590xx_poweroff_id_table,
};
module_platform_driver(bcm590xx_poweroff_driver);

MODULE_AUTHOR("Artur Weber <aweber.kernel@gmail.com>");
MODULE_DESCRIPTION("Broadcom BCM590XX poweroff driver");
