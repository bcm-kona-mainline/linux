// SPDX-License-Identifier: GPL-2.0-only
/*
 * Driver for the power-on key (PONKEY) found in Broadcom BCM590XX PMICs.
 *
 * Copyright (C) 2025 Artur Weber <aweber.kernel@gmail.com>
 */

#include "linux/interrupt.h"
#include <linux/input.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/mfd/bcm590xx.h>

/* Model-specific data struct */
struct bcm590xx_ponkey_data {
	int press_irq;
	int release_irq;
	u8 ponkeyctrl_base;
};

struct bcm590xx_ponkey {
	struct bcm590xx *mfd;
	struct input_dev *input;

	const struct bcm590xx_ponkey_data *data;

	u32 press_debounce_ns;
	u32 release_debounce_ns;
};

/* Under primary I2C address: */
#define BCM590XX_REG_PONKEYCTRL1			0x08
#define BCM590XX_PONKEYCTRL1_PRESS_DEB_MASK		0x7
#define BCM590XX_PONKEYCTRL1_PRESS_DEB_SHIFT		0
#define BCM590XX_PONKEYCTRL1_RELEASE_DEB_MASK		(0x7 << 3)
#define BCM590XX_PONKEYCTRL1_RELEASE_DEB_SHIFT		3

/* Button press/release debounce values */
enum bcm590xx_ponkey_button_debounce {
	PONKEY_DEBOUNCE_330US = 0,	/* 300ns */
	PONKEY_DEBOUNCE_1P2MS,	/* 1.2ms */
	PONKEY_DEBOUNCE_10MS,	/* 10ms  */
	PONKEY_DEBOUNCE_50MS,	/* 50ms  */
	PONKEY_DEBOUNCE_100MS,	/* 100ms */
	PONKEY_DEBOUNCE_500MS,	/* 500ms */
	PONKEY_DEBOUNCE_1000MS,	/* 1000ms */
	PONKEY_DEBOUNCE_2000MS,	/* 2000ms */
};

enum bcm590xx_ponkey_state {
	PONKEY_PRESS,
	PONKEY_RELEASE,
};

/* Set debounce interval for button. */
static int
bcm590xx_ponkey_set_debounce_ns(struct bcm590xx_ponkey *ponkey,
				enum bcm590xx_ponkey_state target, u32 value)
{
	int ret = 0, hw_val;

	switch (value) {
	case 330:
		hw_val = PONKEY_DEBOUNCE_330US;
		break;
	case 1200000:
		hw_val = PONKEY_DEBOUNCE_1P2MS;
		break;
	case 10000000:
		hw_val = PONKEY_DEBOUNCE_10MS;
		break;
	case 50000000:
		hw_val = PONKEY_DEBOUNCE_50MS;
		break;
	case 100000000:
		hw_val = PONKEY_DEBOUNCE_100MS;
		break;
	case 500000000:
		hw_val = PONKEY_DEBOUNCE_500MS;
		break;
	case 1000000000:
		hw_val = PONKEY_DEBOUNCE_1000MS;
		break;
	case 2000000000:
		hw_val = PONKEY_DEBOUNCE_2000MS;
		break;
	default:
		dev_err(&ponkey->input->dev, "Invalid debounce value\n");
		return -EINVAL;
	}

	if (target == PONKEY_PRESS) {
		ret = regmap_update_bits(ponkey->mfd->regmap_pri,
			ponkey->data->ponkeyctrl_base,
			BCM590XX_PONKEYCTRL1_PRESS_DEB_MASK,
			(hw_val << BCM590XX_PONKEYCTRL1_PRESS_DEB_SHIFT));
	} else if (target == PONKEY_RELEASE) {
		ret = regmap_update_bits(ponkey->mfd->regmap_pri,
			ponkey->data->ponkeyctrl_base,
			BCM590XX_PONKEYCTRL1_RELEASE_DEB_MASK,
			(hw_val << BCM590XX_PONKEYCTRL1_RELEASE_DEB_SHIFT));
	} else {
		dev_err(&ponkey->input->dev, "Invalid debounce target\n");
		return -EINVAL;
	}

	if (ret)
		dev_err(&ponkey->input->dev, "Failed to write debounce value: %d\n", ret);

	return ret;
}

static irqreturn_t bcm590xx_ponkey_irq_pressed(int irq, void *data)
{
	struct bcm590xx_ponkey *ponkey = data;

	input_report_key(ponkey->input, KEY_POWER, 1);
	input_sync(ponkey->input);

	return IRQ_HANDLED;
}

static irqreturn_t bcm590xx_ponkey_irq_released(int irq, void *data)
{
	struct bcm590xx_ponkey *ponkey = data;

	input_report_key(ponkey->input, KEY_POWER, 0);
	input_sync(ponkey->input);

	return IRQ_HANDLED;
}

const struct bcm590xx_ponkey_data bcm59054_ponkey_data = {
	.press_irq = BCM59054_IRQ_POK_PRESSED,
	.release_irq = BCM59054_IRQ_POK_RELEASED,
	.ponkeyctrl_base = BCM590XX_REG_PONKEYCTRL1,
};

const struct bcm590xx_ponkey_data bcm59056_ponkey_data = {
	.press_irq = BCM59056_IRQ_PONKEYB_F,
	.release_irq = BCM59056_IRQ_PONKEYB_R,
	.ponkeyctrl_base = BCM590XX_REG_PONKEYCTRL1,
};

static int bcm590xx_ponkey_probe(struct platform_device *pdev)
{
	struct bcm590xx *bcm590xx = dev_get_drvdata(pdev->dev.parent);
	struct bcm590xx_ponkey *ponkey;
	int ret;

	ponkey = devm_kzalloc(&pdev->dev, sizeof(*ponkey), GFP_KERNEL);
	if (!ponkey) {
		dev_err(&pdev->dev, "Failed to allocate memory\n");
		return -ENOMEM;
	}

	ponkey->mfd = bcm590xx;

	switch (bcm590xx->pmu_id) {
	case BCM590XX_PMUID_BCM59054:
		ponkey->data = &bcm59054_ponkey_data;
		break;
	case BCM590XX_PMUID_BCM59056:
		ponkey->data = &bcm59056_ponkey_data;
		break;
	default:
		return -ENODEV;
	}

	ponkey->input = devm_input_allocate_device(&pdev->dev);
	if (!ponkey->input) {
		dev_err(&pdev->dev, "Failed to allocate input device\n");
		return -ENOMEM;
	}

	ponkey->input->name = "bcm590xx-ponkey";
	ponkey->input->phys = "bcm590xx-ponkey/input0";
	ponkey->input->dev.parent = &pdev->dev;
	ponkey->input->evbit[0] = BIT_MASK(EV_KEY);
	__set_bit(KEY_POWER, ponkey->input->keybit);

	/* Request IRQs */
	ret = bcm590xx_devm_request_irq(&pdev->dev, bcm590xx,
					ponkey->data->press_irq,
					bcm590xx_ponkey_irq_pressed, 0,
					"ponkey-pressed", ponkey);
	if (ret) {
		dev_err(&pdev->dev, "Failed to request press IRQ: %d\n", ret);
		return ret;
	}

	ret = bcm590xx_devm_request_irq(&pdev->dev, bcm590xx,
					ponkey->data->release_irq,
					bcm590xx_ponkey_irq_released, 0,
					"ponkey-pressed", ponkey);
	if (ret) {
		dev_err(&pdev->dev, "Failed to request press IRQ: %d\n", ret);
		return ret;
	}

	if (pdev->dev.of_node) {
		/* Set up button press debounce */
		ret = of_property_read_u32(pdev->dev.of_node,
					   "press-debounce-interval",
					   &ponkey->press_debounce_ns);
		if (ret)
			ponkey->press_debounce_ns = 100000000;  /* 100ms */

		/* Set up button release debounce */
		ret = of_property_read_u32(pdev->dev.of_node,
					   "release-debounce-interval",
					   &ponkey->release_debounce_ns);
		if (ret)
			ponkey->release_debounce_ns = 100000000;  /* 100ms */
	} else {
		ponkey->press_debounce_ns = 100000000;  /* 100ms */
		ponkey->release_debounce_ns = 100000000;  /* 100ms */
	}

	ret = bcm590xx_ponkey_set_debounce_ns(ponkey, PONKEY_PRESS,
					      ponkey->press_debounce_ns);
	if (ret)
		return ret;

	ret = bcm590xx_ponkey_set_debounce_ns(ponkey, PONKEY_RELEASE,
					      ponkey->release_debounce_ns);
	if (ret)
		return ret;

	/* Register input device */
	ret = input_register_device(ponkey->input);
	if (ret) {
		dev_err(&pdev->dev, "Failed to register input device: %d\n",
			ret);
		goto err_free;
	}

	platform_set_drvdata(pdev, ponkey);

	return 0;

err_free:
	input_free_device(ponkey->input);

	return ret;
};

static void bcm590xx_ponkey_remove(struct platform_device *pdev)
{
	struct bcm590xx_ponkey *ponkey = platform_get_drvdata(pdev);

	input_unregister_device(ponkey->input);
}

#ifdef CONFIG_OF
static const struct of_device_id bcm590xx_ponkey_match[] = {
	{ .compatible = "brcm,bcm59054-ponkey", },
	{ .compatible = "brcm,bcm59056-ponkey", },
	{}
};
MODULE_DEVICE_TABLE(of, bcm590xx_ponkey_match);
#endif

static struct platform_driver bcm590xx_ponkey_driver = {
	.driver		= {
		.name	= "bcm590xx-ponkey",
		.of_match_table = of_match_ptr(bcm590xx_ponkey_match),
	},
	.probe		= bcm590xx_ponkey_probe,
	.remove		= bcm590xx_ponkey_remove,
};
module_platform_driver(bcm590xx_ponkey_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Artur Weber <aweber.kernel@gmail.com>");
MODULE_DESCRIPTION("Broadcom BCM590XX power-on key (PONKEY) driver");
