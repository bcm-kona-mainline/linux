// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2014 Broadcom Corporation

#include <asm/mach/arch.h>

#include "kona_l2_cache.h"
#include <linux/irqchip.h>
#include <soc/bcmkona-pwrmgr.h>

static void __init bcm21664_irq_init(void)
{
	kona_pwrmgr_early_init();
	irqchip_init();
}

static void __init bcm21664_init(void)
{
	kona_l2_cache_init();
}

static const char * const bcm21664_dt_compat[] = {
	"brcm,bcm21664",
	NULL,
};

DT_MACHINE_START(BCM21664_DT, "BCM21664 Broadcom Application Processor")
	.init_irq = bcm21664_irq_init,
	.init_machine = bcm21664_init,
	.dt_compat = bcm21664_dt_compat,
MACHINE_END
