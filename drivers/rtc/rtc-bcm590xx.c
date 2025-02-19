// SPDX-License-Identifier: GPL-2.0-only
/*
 * Broadcom BCM590XX PMU real-time clock driver
 *
 * Copyright (c) 2025 Artur Weber <aweber.kernel@gmail.com>
 */

#include "linux/regmap.h"
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/rtc.h>
#include <linux/delay.h>
#include <linux/mfd/bcm590xx.h>

/*
 * Offsets from time base reg/alarm base reg to registers for each component
 * of the date.
 */
enum bcm590xx_rtc_time_reg_offset {
	BCM590XX_RTC_SECOND_OFFSET = 0,
	BCM590XX_RTC_MINUTE_OFFSET,
	BCM590XX_RTC_HOUR_OFFSET,
	BCM590XX_RTC_WEEKDAY_OFFSET,
	BCM590XX_RTC_DAY_OFFSET,
	BCM590XX_RTC_MONTH_OFFSET,
	BCM590XX_RTC_YEAR_OFFSET,
	BCM590XX_RTC_OFFSET_COUNT,
};

/*
 * Model-specific data struct
 *
 * @regmap		Which regmap to use for RTC data (primary/secondary)
 *
 * @time_base_reg	Base address for time data
 * @alarm_base_reg	Base address for alarm data
 *
 * @alarm_irq		PMU IRQ ID to use for alarm notifications
 * @sec_irq		PMU IRQ ID to use for second change notifications
 * @adj_irq		PMU IRQ ID to use for RTC adjustment interrupt
 */
struct bcm590xx_rtc_data {
	enum bcm590xx_regmap_type regmap;

	u8 time_base_reg;
	u8 alarm_base_reg;

	int alarm_irq;
	int sec_irq;
	int adj_irq;
};

struct bcm590xx_rtc_data bcm59054_rtc_data = {
	.regmap = BCM590XX_REGMAP_SEC,

	.time_base_reg = 0xe0,
	.alarm_base_reg = 0xe7,

	.alarm_irq = BCM59054_IRQ_RTC_ALARM,
	.sec_irq = BCM59054_IRQ_RTC_SEC,
	.adj_irq = BCM59054_IRQ_RTCADJ,
};

struct bcm590xx_rtc_data bcm59056_rtc_data = {
	.regmap = BCM590XX_REGMAP_SEC,

	.time_base_reg = 0xe0,
	.alarm_base_reg = 0xe7,

	.alarm_irq = BCM59056_IRQ_RTC_ALARM,
	.sec_irq = BCM59056_IRQ_RTC_SEC,
	.adj_irq = BCM59056_IRQ_RTCADJ,
};

struct bcm590xx_rtc {
	struct bcm590xx *mfd;
	struct rtc_device *rtc_dev;
	struct regmap *regmap;
	const struct bcm590xx_rtc_data *data;

	int alarm_irq;
	int sec_irq;
	int adj_irq;
};

static irqreturn_t bcm590xx_rtc_sec_irq_handler(int irq, void *data)
{
	struct bcm590xx_rtc *rtc = data;

	rtc_update_irq(rtc->rtc_dev, 1, RTC_UF);

	return IRQ_HANDLED;
}

static irqreturn_t bcm590xx_rtc_alarm_irq_handler(int irq, void *data)
{
	struct bcm590xx_rtc *rtc = data;

	rtc_update_irq(rtc->rtc_dev, 1, RTC_AF);

	return IRQ_HANDLED;
}

/*
 * Since both RTC time and alarm time are stored in the same way (just with a
 * different register offset), we can use the same function for reading/writing
 * both, with a custom offset.
 */

/* Read a time value into an rtc_time struct from the specified register. */
static int
_bcm590xx_read_time_from_reg(struct bcm590xx_rtc *rtc, u8 reg, struct rtc_time *tm)
{
	u8 time[BCM590XX_RTC_OFFSET_COUNT];
	int ret;

	ret = regmap_bulk_read(rtc->regmap, reg, &time,
			       BCM590XX_RTC_OFFSET_COUNT);
	if (ret)
		return ret;

	tm->tm_sec = time[BCM590XX_RTC_SECOND_OFFSET];
	tm->tm_min = time[BCM590XX_RTC_MINUTE_OFFSET];
	tm->tm_hour = time[BCM590XX_RTC_HOUR_OFFSET];
	tm->tm_wday = time[BCM590XX_RTC_WEEKDAY_OFFSET];
	tm->tm_mday = time[BCM590XX_RTC_DAY_OFFSET];
	if (time[BCM590XX_RTC_MONTH_OFFSET] > 0)
		tm->tm_mon = time[BCM590XX_RTC_MONTH_OFFSET] - 1;
	else
		tm->tm_mon = 0;
	tm->tm_year = time[BCM590XX_RTC_YEAR_OFFSET] + 100;

	return 0;
}

static int
_bcm590xx_write_time_to_reg(struct bcm590xx_rtc *rtc, u8 reg, struct rtc_time *tm)
{
	u8 time[BCM590XX_RTC_OFFSET_COUNT];
	int ret;

	time[BCM590XX_RTC_SECOND_OFFSET] = tm->tm_sec;
	time[BCM590XX_RTC_MINUTE_OFFSET] = tm->tm_min;
	time[BCM590XX_RTC_HOUR_OFFSET] = tm->tm_hour;
	time[BCM590XX_RTC_WEEKDAY_OFFSET] = tm->tm_wday;
	time[BCM590XX_RTC_DAY_OFFSET] = tm->tm_mday;
	time[BCM590XX_RTC_MONTH_OFFSET] = tm->tm_mon + 1;
	time[BCM590XX_RTC_YEAR_OFFSET] = (u8)(tm->tm_year - 100);

	ret = regmap_bulk_write(rtc->regmap, reg, &time,
				BCM590XX_RTC_OFFSET_COUNT);

	return ret;
}

static int bcm590xx_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	struct bcm590xx_rtc *rtc = dev_get_drvdata(dev);
	int ret;

	ret = _bcm590xx_read_time_from_reg(rtc, rtc->data->time_base_reg, tm);
	if (ret)
		dev_err(dev, "Failed to read time regs: %d\n", ret);

	return ret;
}

static int bcm590xx_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	struct bcm590xx_rtc *rtc = dev_get_drvdata(dev);
	int ret;

	ret = _bcm590xx_write_time_to_reg(rtc, rtc->data->time_base_reg, tm);
	if (ret)
		dev_err(dev, "Failed to write time regs: %d\n", ret);

	return ret;
}

static irqreturn_t bcm590xx_rtc_adj_irq_handler(int irq, void *data)
{
	struct bcm590xx_rtc *rtc = data;
	struct rtc_time init_time;

	dev_warn(&rtc->rtc_dev->dev, "RTC reports invalid value; resetting to default");

	init_time.tm_sec = 0;
	init_time.tm_min = 0;
	init_time.tm_hour = 0;
	init_time.tm_wday = 1;
	init_time.tm_mday = 1;
	init_time.tm_mon = 1;
	init_time.tm_year = 107;

	bcm590xx_rtc_set_time(&rtc->rtc_dev->dev, &init_time);

	return IRQ_HANDLED;
}

static int bcm590xx_rtc_read_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct bcm590xx_rtc *rtc = dev_get_drvdata(dev);
	int ret;

	ret = _bcm590xx_read_time_from_reg(rtc, rtc->data->alarm_base_reg,
					   &alrm->time);
	if (ret)
		dev_err(dev, "Failed to read alarm time regs: %d\n", ret);

	return ret;
}

static int bcm590xx_rtc_set_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct bcm590xx_rtc *rtc = dev_get_drvdata(dev);
	int ret;

	ret = _bcm590xx_write_time_to_reg(rtc, rtc->data->alarm_base_reg,
					  &alrm->time);
	if (ret)
		dev_err(dev, "Failed to write alarm time regs: %d\n", ret);

	return ret;
}

static int bcm590xx_rtc_alarm_irq_enable(struct device *dev, unsigned int enabled)
{
	struct bcm590xx_rtc *rtc = dev_get_drvdata(dev);
	int ret;

	if (enabled) {
		ret = bcm590xx_devm_request_irq(dev, rtc->mfd,
						rtc->data->alarm_irq,
						bcm590xx_rtc_alarm_irq_handler,
						0, "rtc", rtc);

		if (ret) {
			dev_err(dev, "Failed to request alarm IRQ: %d\n", ret);
			return ret;
		}

		rtc->alarm_irq = ret;
	} else {
		if (rtc->alarm_irq < 0)
			return 0;

		devm_free_irq(dev, rtc->alarm_irq, rtc);
		rtc->alarm_irq = -1;
	}

	return 0;
}

static const struct rtc_class_ops bcm590xx_rtc_ops = {
	.read_time	= bcm590xx_rtc_read_time,
	.set_time	= bcm590xx_rtc_set_time,
	.read_alarm	= bcm590xx_rtc_read_alarm,
	.set_alarm	= bcm590xx_rtc_set_alarm,
	.alarm_irq_enable = bcm590xx_rtc_alarm_irq_enable,
};

static int bcm590xx_rtc_probe(struct platform_device *pdev)
{
	struct bcm590xx *bcm590xx = dev_get_drvdata(pdev->dev.parent);
	struct bcm590xx_rtc *rtc;
	struct rtc_time init_time;
	int ret;

	rtc = devm_kzalloc(&pdev->dev, sizeof(struct bcm590xx_rtc),
			   GFP_KERNEL);
	if (!rtc) {
		dev_err(&pdev->dev, "Failed to allocate memory\n");
		return -ENOMEM;
	}

	dev_set_drvdata(&pdev->dev, rtc);

	rtc->mfd = bcm590xx;
	switch (bcm590xx->pmu_id) {
	case BCM590XX_PMUID_BCM59054:
		rtc->data = &bcm59054_rtc_data;
		break;
	case BCM590XX_PMUID_BCM59056:
		rtc->data = &bcm59056_rtc_data;
		break;
	default:
		return -ENODEV;
	}

	switch (rtc->data->regmap) {
	case BCM590XX_REGMAP_PRI:
		rtc->regmap = rtc->mfd->regmap_pri;
		break;
	case BCM590XX_REGMAP_SEC:
		rtc->regmap = rtc->mfd->regmap_sec;
		break;
	default:
		dev_err(&pdev->dev,
			"Invalid regmap value; this is a driver bug!\n");
		return -EINVAL;
	}

	rtc->rtc_dev = devm_rtc_allocate_device(&pdev->dev);
	if (IS_ERR(rtc->rtc_dev))
		return dev_err_probe(&pdev->dev, PTR_ERR(rtc->rtc_dev),
				     "Failed to allocate RTC device");

	/*
	* When the RTC resets, the date is set to an invalid value (0th January
	* 2007), which causes the rtc core to complain about invalid time.
	* Correct the invalid value if we notice it.
	*/
	ret = bcm590xx_rtc_read_time(&pdev->dev, &init_time);
	if (ret)
		return ret;

	if (init_time.tm_mday == 0) {
		dev_info(&pdev->dev, "Detected RTC reset, correcting value");
		init_time.tm_mday = 1;
		bcm590xx_rtc_set_time(&pdev->dev, &init_time);
	}

	/* Alarm IRQ is requested in bcm590xx_rtc_alarm_irq_enable */
	rtc->alarm_irq = -1;

	rtc->sec_irq = bcm590xx_devm_request_irq(&pdev->dev, bcm590xx,
						 rtc->data->sec_irq,
						 bcm590xx_rtc_sec_irq_handler,
						 0, "rtc-sec", rtc);
	if (rtc->sec_irq < 0) {
		dev_err(&pdev->dev, "Failed to request second update IRQ: %d\n",
			rtc->sec_irq);
		return rtc->sec_irq;
	}

	rtc->adj_irq = bcm590xx_devm_request_irq(&pdev->dev, bcm590xx,
						 rtc->data->adj_irq,
						 bcm590xx_rtc_adj_irq_handler,
						 0, "rtc-sec", rtc);
	if (rtc->adj_irq < 0) {
		dev_err(&pdev->dev, "Failed to request adjustment IRQ: %d\n",
			rtc->adj_irq);
		return rtc->adj_irq;
	}

	rtc->rtc_dev->ops = &bcm590xx_rtc_ops;
	ret = devm_rtc_register_device(rtc->rtc_dev);
	if (ret)
		return dev_err_probe(&pdev->dev, ret,
				     "Failed to register RTC device");

	return 0;
}

static struct platform_driver bcm590xx_rtc_driver = {
	.driver		= {
		.name	= "bcm590xx-rtc",
	},
	.probe		= bcm590xx_rtc_probe,
};

module_platform_driver(bcm590xx_rtc_driver);

MODULE_DESCRIPTION("Broadcom BCM590XX PMU RTC driver");
MODULE_AUTHOR("Artur Weber <aweber.kernel@gmail.com>");
MODULE_LICENSE("GPL");
