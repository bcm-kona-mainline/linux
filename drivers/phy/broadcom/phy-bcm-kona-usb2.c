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

#define OTGCTL			0x00
#define OTGCTL_OTGSTAT2		BIT(31)
#define OTGCTL_OTGSTAT1		BIT(30)
#define OTGCTL_REG_OTGSTAT2	BIT(29)
#define OTGCTL_REG_OTGSTAT1	BIT(28)
#define OTGCTL_UTMIOTG_IDDIG_SW	BIT(26)
#define OTGCTL_PHY_ISO_I	BIT(24)
#define OTGCTL_PRST_N_SW	BIT(11)
#define OTGCTL_HRESET_N		BIT(10)
#define OTGCTL_UTMI_LINE_STATE1	BIT(9)
#define OTGCTL_UTMI_LINE_STATE0	BIT(8)
#define OTGCTL_SOFT_LDO_PWRDN	BIT(5)
#define OTGCTL_SOFT_DLDO_PWRDN	BIT(3)
#define OTGCTL_SOFT_ALDO_PWRDN	BIT(2)

#define PHYCFG			0x04
#define PHYCFG_IDDQ_I		BIT(1)

#define P1CTL			0x08
#define P1CTL_CLK_REQUEST_CLEAR	BIT(31)
#define P1CTL_CLK_REQUEST	BIT(30)
#define P1CTL_SOFT_RESET	BIT(1)
#define P1CTL_NON_DRIVING	BIT(0)

#define BCCFG				0x10
#define BCCFG_SW_OVERWRITE_KEY_MASK	GENMASK(31, 17)
#define BCCFG_SW_OVERWRITE_KEY		0x55560000
#define BCCFG_SW_OVERWRITE_EN		BIT(16)

#define PHYCTL			0x1c
#define PHYCTL_SUSPEND		BIT(11)

struct bcm_kona_usb {
	void __iomem *regs;
	struct device *dev;

	struct clk *otg_clk;
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
	int ret;
	u32 val;

	pr_info("bcm_kona_usb_phy_init called\n");

	ret = clk_prepare_enable(phy->otg_clk);
	if (ret) {
		dev_err(&gphy->dev, "Failed to enable OTG clock: %d\n", ret);
	}

	/* Enable software control of PHY-PM */
	val = readl(phy->regs + OTGCTL);
	val |= OTGCTL_SOFT_LDO_PWRDN;
	writel(val, phy->regs + OTGCTL);

	/* Soft reset PHY */
	val = readl(phy->regs + P1CTL);
	val &= ~P1CTL_SOFT_RESET;
	writel(val, phy->regs + P1CTL);

	/* Reset PHY and AHB clock domains */
	val = readl(phy->regs + OTGCTL);
	val &= ~(OTGCTL_PRST_N_SW | OTGCTL_HRESET_N);
	writel(val, phy->regs + OTGCTL);

	/* Deassert clock domain reset */
	val = readl(phy->regs + OTGCTL);
	val |= OTGCTL_PRST_N_SW | OTGCTL_HRESET_N;
	writel(val, phy->regs + OTGCTL);
	mdelay(2);

	/* Power up ALDO/DLDO */
	val = readl(phy->regs + OTGCTL);
	val |= OTGCTL_SOFT_ALDO_PWRDN | OTGCTL_SOFT_DLDO_PWRDN;
	writel(val, phy->regs + OTGCTL);
	mdelay(1);

	/* Enable pad, internal PLL */
	val = readl(phy->regs + PHYCFG);
	val &= ~PHYCFG_IDDQ_I;
	writel(val, phy->regs + PHYCFG);

	/* Set LDO suspend mask */
	val = readl(phy->regs + PHYCTL);
	val |= PHYCTL_SUSPEND;
	writel(val, phy->regs + PHYCTL);

	/* Remove PHY isolation */
	val = readl(phy->regs + OTGCTL);
	val &= ~OTGCTL_PHY_ISO_I;
	writel(val, phy->regs + OTGCTL);
	mdelay(1);

	/* PHY clock reset */
	val = readl(phy->regs + P1CTL);
	val |= P1CTL_CLK_REQUEST;
	writel(val, phy->regs + P1CTL);

	val = readl(phy->regs + P1CTL);
	val |= P1CTL_CLK_REQUEST_CLEAR;
	writel(val, phy->regs + P1CTL);
	mdelay(2);

	/* Bring PHY out of reset state */
	writel(val | P1CTL_SOFT_RESET, phy->regs + P1CTL);

	/* Set correct ID value */
	val = readl(phy->regs + OTGCTL);
	val |= OTGCTL_UTMIOTG_IDDIG_SW;
	writel(val, phy->regs + OTGCTL);

	/* Set VBUS valid state */
	val = readl(phy->regs + OTGCTL);
	val |= OTGCTL_REG_OTGSTAT1 | OTGCTL_REG_OTGSTAT2;
	writel(val, phy->regs + OTGCTL);
	mdelay(200);

	/* Enable SW overwrite */
	val = readl(phy->regs + BCCFG);
	val &= ~BCCFG_SW_OVERWRITE_KEY_MASK;
	val |= (BCCFG_SW_OVERWRITE_KEY | BCCFG_SW_OVERWRITE_EN);
	writel(val, phy->regs + BCCFG);
	mdelay(2);

	/* Clear non-driving bit */
	val = readl(phy->regs + P1CTL);
	val &= ~P1CTL_NON_DRIVING;
	writel(val, phy->regs + P1CTL);

	return 0;
}

static int bcm_kona_usb_phy_power_on(struct phy *gphy)
{
	struct bcm_kona_usb *phy = phy_get_drvdata(gphy);

	pr_info("bcm_kona_usb_phy_power_on called\n");

	bcm_kona_usb_phy_power(phy, 1);

	return 0;
}

static int bcm_kona_usb_phy_power_off(struct phy *gphy)
{
	struct bcm_kona_usb *phy = phy_get_drvdata(gphy);

	pr_info("bcm_kona_usb_phy_power_off called\n");

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

	phy->otg_clk = devm_clk_get(dev, "otg");
	if (IS_ERR(phy->otg_clk))
		return dev_err_probe(dev, PTR_ERR(phy->otg_clk),
				     "Failed to get OTG clock\n");

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
