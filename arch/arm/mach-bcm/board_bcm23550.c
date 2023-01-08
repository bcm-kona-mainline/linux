// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2016 Broadcom

#include <linux/of_platform.h>
#include <linux/printk.h>
#include <linux/irqchip.h>

#include <asm/mach/arch.h>

#include <soc/bcmkona-pwrmgr.h>

static void __init bcm23550_init(void)
{
	pr_info("in bcm23550_init");
	kona_pwrmgr_early_init();
	irqchip_init();
}

static const char * const bcm23550_dt_compat[] = {
	"brcm,bcm23550",
	NULL,
};

DT_MACHINE_START(BCM23550_DT, "BCM23550 Broadcom Application Processor")
	.init_irq = bcm23550_init,
	.dt_compat = bcm23550_dt_compat,
MACHINE_END
