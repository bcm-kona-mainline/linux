// SPDX-License-Identifier: GPL-2.0-only
/*
 * phy-bcm-kona-usb2.c - Broadcom Kona USB2 Phy Driver
 *
 * Copyright (C) 2013 Linaro Limited
 * Matt Porter <mporter@linaro.org>
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <linux/printk.h>

#define OTGCTL			(0)
#define OTGCTL_OTGSTAT2		BIT(31)
#define OTGCTL_OTGSTAT1		BIT(30)
#define OTGCTL_PRST_N_SW	BIT(11)
#define OTGCTL_HRESET_N		BIT(10)
#define OTGCTL_UTMI_LINE_STATE1	BIT(9)
#define OTGCTL_UTMI_LINE_STATE0	BIT(8)

#define PHYCFG			(4)
#define PHYCFG_IDDQ_I   (1)

#define P1CTL			(8)
#define P1CTL_USB11_OEB_IS_TXEB	BIT(15)
#define P1CTL_PHY_MODE			BIT(2)
#define P1CTL_SOFT_RESET		BIT(1)
#define P1CTL_NON_DRIVING		BIT(0)

#define PHY_MODE_OTG 2

struct bcm_kona_usb {
	unsigned int clear_bit_15:1;
	void __iomem *regs;
};

static void bcm_kona_usb_phy_power(struct bcm_kona_usb *phy, int on)
{
	u32 val;

	val = readl(phy->regs + OTGCTL);
	if (on) {
		/* Configure and power PHY */
		val &= ~(OTGCTL_OTGSTAT2 | OTGCTL_OTGSTAT1 |
			 OTGCTL_UTMI_LINE_STATE1 | OTGCTL_UTMI_LINE_STATE0);
		val |= OTGCTL_PRST_N_SW | OTGCTL_HRESET_N;
	} else {
		val &= ~(OTGCTL_PRST_N_SW | OTGCTL_HRESET_N);
	}
	writel(val, phy->regs + OTGCTL);
}

static int bcm_kona_usb_phy_init(struct phy *gphy)
{
	struct bcm_kona_usb *phy = phy_get_drvdata(gphy);
	u32 val;

	printk("usb-phy init");

	/* Clear P1CTL bit 15. Needed on some platforms. */
	if (phy->clear_bit_15) {
		val = readl(phy->regs + P1CTL);
		val &= ~P1CTL_USB11_OEB_IS_TXEB;
		writel(val, phy->regs + P1CTL);
		mdelay(2);
	}

	/* Soft reset PHY */
	val = readl(phy->regs + P1CTL);
	val &= ~P1CTL_NON_DRIVING;
	val |= P1CTL_SOFT_RESET;
	writel(val, phy->regs + P1CTL);
	writel(val & ~P1CTL_SOFT_RESET, phy->regs + P1CTL);
	/* Reset needs to be asserted for 2ms */
	mdelay(2);
	writel(val | P1CTL_SOFT_RESET, phy->regs + P1CTL);

	/* Set OTG mode */
	val |= PHY_MODE_OTG << P1CTL_PHY_MODE;
	writel(val, phy->regs + P1CTL);
	mdelay(2);

	/* PLL init..? TODO: figure out what this does */
	val = readl(phy->regs + PHYCFG);
	val |= PHYCFG_IDDQ_I;
	writel(val, phy->regs + PHYCFG);
	mdelay(2);

	return 0;
}

static int bcm_kona_usb_phy_power_on(struct phy *gphy)
{
	struct bcm_kona_usb *phy = phy_get_drvdata(gphy);

	printk("usb-phy poweron");

	bcm_kona_usb_phy_power(phy, 1);

	return 0;
}

static int bcm_kona_usb_phy_power_off(struct phy *gphy)
{
	struct bcm_kona_usb *phy = phy_get_drvdata(gphy);

	bcm_kona_usb_phy_power(phy, 0);

	return 0;
}

static const struct phy_ops ops = {
	.init		= bcm_kona_usb_phy_init,
	.power_on	= bcm_kona_usb_phy_power_on,
	.power_off	= bcm_kona_usb_phy_power_off,
	.owner		= THIS_MODULE,
};

static int bcm_kona_usb2_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct bcm_kona_usb *phy;
	struct phy *gphy;
	struct phy_provider *phy_provider;

	phy = devm_kzalloc(dev, sizeof(*phy), GFP_KERNEL);
	if (!phy)
		return -ENOMEM;

	phy->regs = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(phy->regs))
		return PTR_ERR(phy->regs);

	phy->clear_bit_15 = of_property_read_bool(dev->of_node,
			                           "brcm,clear-bit-15");

	platform_set_drvdata(pdev, phy);

	gphy = devm_phy_create(dev, NULL, &ops);
	if (IS_ERR(gphy))
		return PTR_ERR(gphy);

	/* The Kona PHY supports an 8-bit wide UTMI interface */
	phy_set_bus_width(gphy, 8);

	phy_set_drvdata(gphy, phy);

	phy_provider = devm_of_phy_provider_register(dev,
			of_phy_simple_xlate);

	return PTR_ERR_OR_ZERO(phy_provider);
}

static const struct of_device_id bcm_kona_usb2_dt_ids[] = {
	{ .compatible = "brcm,kona-usb2-phy" },
	{ /* sentinel */ }
};

MODULE_DEVICE_TABLE(of, bcm_kona_usb2_dt_ids);

static struct platform_driver bcm_kona_usb2_driver = {
	.probe		= bcm_kona_usb2_probe,
	.driver		= {
		.name	= "bcm-kona-usb2",
		.of_match_table = bcm_kona_usb2_dt_ids,
	},
};

module_platform_driver(bcm_kona_usb2_driver);

MODULE_ALIAS("platform:bcm-kona-usb2");
MODULE_AUTHOR("Matt Porter <mporter@linaro.org>");
MODULE_DESCRIPTION("BCM Kona USB 2.0 PHY driver");
MODULE_LICENSE("GPL v2");
