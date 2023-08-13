// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2014 Broadcom Corporation
 * Copyright 2014 Linaro Limited
 */

#include "clk-kona.h"
#include "dt-bindings/clock/bcm21664.h"

#define BCM21664_CCU_COMMON(_name, _capname) \
	KONA_CCU_COMMON(BCM21664, _name, _capname)

/* Root CCU */

static struct peri_clk_data frac_1m_data = {
	.gate		= HW_SW_GATE(0x214, 16, 0, 1),
	.clocks		= CLOCKS("ref_crystal"),
};

static struct ccu_data root_ccu_data = {
	BCM21664_CCU_COMMON(root, ROOT),
	/* no policy control */
	.kona_clks	= {
		[BCM21664_ROOT_CCU_FRAC_1M] =
			KONA_CLK(root, frac_1m, peri),
		[BCM21664_ROOT_CCU_CLOCK_COUNT] = LAST_KONA_CLK,
	},
};

/* AON CCU */

static struct peri_clk_data hub_timer_data = {
	.policy		= POLICY(0x0010, 6),
	.gate		= HW_SW_GATE(0x0414, 16, 0, 1),
	.hyst		= HYST(0x0414, 8, 9),
	.clocks		= CLOCKS("bbl_32k",
				 "frac_1m",
				 "dft_19_5m"),
	.sel		= SELECTOR(0x0a10, 0, 2),
	.trig		= TRIGGER(0x0a40, 4),
};

static struct bus_clk_data pmu_bsc_apb_data = {
	.gate		= HW_SW_GATE(0x0418, 18, 2, 3),
	.hyst		= HYST(0x0418, 10, 11),
};

static struct peri_clk_data pmu_bsc_data = {
	.policy		= POLICY(0x0010, 8),
	.gate		= HW_SW_GATE(0x0418, 16, 0, 1),
	.hyst		= HYST(0x0418, 8, 9),
	.clocks		= CLOCKS("ref_crystal",
				 "pmu_bsc_var",
				 "bbl_32k"),
	.sel		= SELECTOR(0x0a04, 0, 2),
	.div		= DIVIDER(0x0a04, 3, 4),
	.trig		= TRIGGER(0x0a40, 0),
};

static u32 aon_ccu_freq_tbl0[] = { 26000000, 26000000 };
static u32 aon_ccu_freq_tbl1[] = { 52000000, 52000000 };
static u32 aon_ccu_freq_tbl2[] = { 78000000, 78000000 };
static u32 aon_ccu_freq_tbl3[] = { 104000000, 52000000 };
static u32 aon_ccu_freq_tbl4[] = { 156000000, 78000000 };

static struct ccu_data aon_ccu_data = {
	BCM21664_CCU_COMMON(aon, AON),
	.policy		= {
		.enable		= CCU_LVM_EN(0x0034, 0),
		.control	= CCU_POLICY_CTL(0x000c, 0, 1, 2),
		.freq		= {
			.offset = 0x0008,

			.policy_count = 5,
			.table_size = 2,
			.tables = {
				aon_ccu_freq_tbl0,
				aon_ccu_freq_tbl1,
				aon_ccu_freq_tbl2,
				aon_ccu_freq_tbl3,
				aon_ccu_freq_tbl4,
			},
		},
	},
	.kona_clks	= {
		[BCM21664_AON_CCU_HUB_TIMER] =
			KONA_CLK(aon, hub_timer, peri),
		[BCM21664_AON_CCU_PMU_BSC_APB] =
			KONA_CLK(aon, pmu_bsc_apb, bus),
		[BCM21664_AON_CCU_PMU_BSC] =
			KONA_CLK(aon, pmu_bsc, peri),
		[BCM21664_AON_CCU_CLOCK_COUNT] = LAST_KONA_CLK,
	},
};

/* Master CCU */

static struct bus_clk_data sdio1_ahb_data = {
	.policy		= POLICY(0x0010, 6),
	.gate		= HW_SW_GATE(0x0358, 16, 0, 1),
};

static struct bus_clk_data sdio2_ahb_data = {
	.policy		= POLICY(0x0010, 5),
	.gate		= HW_SW_GATE(0x035c, 16, 0, 1),
};

static struct bus_clk_data sdio3_ahb_data = {
	.policy		= POLICY(0x0010, 3),
	.gate		= HW_SW_GATE(0x0364, 16, 0, 1),
};

static struct bus_clk_data sdio4_ahb_data = {
	.policy		= POLICY(0x0010, 4),
	.gate		= HW_SW_GATE(0x0360, 16, 0, 1),
};

static struct peri_clk_data sdio1_data = {
	.policy		= POLICY(0x0010, 6),
	.gate		= HW_SW_GATE(0x0358, 18, 2, 3),
	.clocks		= CLOCKS("ref_crystal",
				 "var_52m",
				 "ref_52m",
				 "var_96m",
				 "ref_96m"),
	.sel		= SELECTOR(0x0a28, 0, 3),
	.div		= DIVIDER(0x0a28, 4, 14),
	.trig		= TRIGGER(0x0afc, 9),
};

static struct peri_clk_data sdio2_data = {
	.policy		= POLICY(0x0010, 5),
	.gate		= HW_SW_GATE(0x035c, 18, 2, 3),
	.clocks		= CLOCKS("ref_crystal",
				 "var_52m",
				 "ref_52m",
				 "var_96m",
				 "ref_96m"),
	.sel		= SELECTOR(0x0a2c, 0, 3),
	.div		= DIVIDER(0x0a2c, 4, 14),
	.trig		= TRIGGER(0x0afc, 10),
};

static struct peri_clk_data sdio3_data = {
	.policy		= POLICY(0x0010, 3),
	.gate		= HW_SW_GATE(0x0364, 18, 2, 3),
	.clocks		= CLOCKS("ref_crystal",
				 "var_52m",
				 "ref_52m",
				 "var_96m",
				 "ref_96m"),
	.sel		= SELECTOR(0x0a34, 0, 3),
	.div		= DIVIDER(0x0a34, 4, 14),
	.trig		= TRIGGER(0x0afc, 12),
};

static struct peri_clk_data sdio4_data = {
	.policy		= POLICY(0x0010, 4),
	.gate		= HW_SW_GATE(0x0360, 18, 2, 3),
	.clocks		= CLOCKS("ref_crystal",
				 "var_52m",
				 "ref_52m",
				 "var_96m",
				 "ref_96m"),
	.sel		= SELECTOR(0x0a30, 0, 3),
	.div		= DIVIDER(0x0a30, 4, 14),
	.trig		= TRIGGER(0x0afc, 11),
};

static struct peri_clk_data sdio1_sleep_data = {
	.clocks		= CLOCKS("ref_32k"),	/* Verify */
	.gate		= HW_SW_GATE(0x0358, 18, 2, 3),
};

static struct peri_clk_data sdio2_sleep_data = {
	.clocks		= CLOCKS("ref_32k"),	/* Verify */
	.gate		= HW_SW_GATE(0x035c, 18, 2, 3),
};

static struct peri_clk_data sdio3_sleep_data = {
	.clocks		= CLOCKS("ref_32k"),	/* Verify */
	.gate		= HW_SW_GATE(0x0364, 18, 2, 3),
};

static struct peri_clk_data sdio4_sleep_data = {
	.clocks		= CLOCKS("ref_32k"),	/* Verify */
	.gate		= HW_SW_GATE(0x0360, 18, 2, 3),
};

static struct bus_clk_data usb_otg_ahb_data = {
	.policy		= POLICY(0x0010, 11),
	.gate		= HW_SW_GATE(0x0348, 16, 0, 1),
};

static u32 master_ccu_freq_tbl0[] = { 26000000, 26000000, 26000000 };
static u32 master_ccu_freq_tbl1[] = { 52000000, 52000000, 26000000 };
static u32 master_ccu_freq_tbl2[] = { 104000000, 52000000, 26000000 };
static u32 master_ccu_freq_tbl3[] = { 156000000, 52000000, 26000000 };
static u32 master_ccu_freq_tbl4[] = { 156000000, 78000000, 39000000 };
static u32 master_ccu_freq_tbl5[] = { 208000000, 104000000, 52000000 };
static u32 master_ccu_freq_tbl6[] = { 312000000, 104000000, 52000000 };
static u32 master_ccu_freq_tbl7[] = { 312000000, 156000000, 78000000 };

static struct ccu_data master_ccu_data = {
	BCM21664_CCU_COMMON(master, MASTER),
	.policy		= {
		.enable		= CCU_LVM_EN(0x0034, 0),
		.control	= CCU_POLICY_CTL(0x000c, 0, 1, 2),
		.freq		= {
			.offset = 0x0008,

			.policy_count = 8,
			.table_size = 3,
			.tables = {
				master_ccu_freq_tbl0,
				master_ccu_freq_tbl1,
				master_ccu_freq_tbl2,
				master_ccu_freq_tbl3,
				master_ccu_freq_tbl4,
				master_ccu_freq_tbl5,
				master_ccu_freq_tbl6,
				master_ccu_freq_tbl7,
			},
		},
	},
	.kona_clks	= {
		[BCM21664_MASTER_CCU_SDIO1_AHB] =
			KONA_CLK(master, sdio1_ahb, bus),
		[BCM21664_MASTER_CCU_SDIO2_AHB] =
			KONA_CLK(master, sdio2_ahb, bus),
		[BCM21664_MASTER_CCU_SDIO3_AHB] =
			KONA_CLK(master, sdio3_ahb, bus),
		[BCM21664_MASTER_CCU_SDIO4_AHB] =
			KONA_CLK(master, sdio4_ahb, bus),
		[BCM21664_MASTER_CCU_SDIO1] =
			KONA_CLK(master, sdio1, peri),
		[BCM21664_MASTER_CCU_SDIO2] =
			KONA_CLK(master, sdio2, peri),
		[BCM21664_MASTER_CCU_SDIO3] =
			KONA_CLK(master, sdio3, peri),
		[BCM21664_MASTER_CCU_SDIO4] =
			KONA_CLK(master, sdio4, peri),
		[BCM21664_MASTER_CCU_SDIO1_SLEEP] =
			KONA_CLK(master, sdio1_sleep, peri),
		[BCM21664_MASTER_CCU_SDIO2_SLEEP] =
			KONA_CLK(master, sdio2_sleep, peri),
		[BCM21664_MASTER_CCU_SDIO3_SLEEP] =
			KONA_CLK(master, sdio3_sleep, peri),
		[BCM21664_MASTER_CCU_SDIO4_SLEEP] =
			KONA_CLK(master, sdio4_sleep, peri),
		[BCM21664_MASTER_CCU_USB_OTG_AHB] =
			KONA_CLK(master, usb_otg_ahb, bus),
		[BCM21664_MASTER_CCU_CLOCK_COUNT] = LAST_KONA_CLK,
	},
};

/* Slave CCU */

static struct bus_clk_data uartb_apb_data = {
	.gate		= HW_SW_GATE_AUTO(0x0400, 16, 0, 1),
};

static struct bus_clk_data uartb2_apb_data = {
	.gate		= HW_SW_GATE_AUTO(0x0404, 16, 0, 1),
};

static struct bus_clk_data uartb3_apb_data = {
	.gate		= HW_SW_GATE_AUTO(0x0408, 16, 0, 1),
};

static struct peri_clk_data uartb_data = {
	.policy		= POLICY(0x0010, 20),
	.gate		= HW_SW_GATE(0x0400, 18, 2, 3),
	.clocks		= CLOCKS("ref_crystal",
				 "var_156m",
				 "ref_156m"),
	.sel		= SELECTOR(0x0a10, 0, 2),
	.div		= FRAC_DIVIDER(0x0a10, 4, 12, 8),
	.trig		= TRIGGER(0x0afc, 2),
};

static struct peri_clk_data uartb2_data = {
	.policy		= POLICY(0x0010, 19),
	.gate		= HW_SW_GATE(0x0404, 18, 2, 3),
	.clocks		= CLOCKS("ref_crystal",
				 "var_156m",
				 "ref_156m"),
	.sel		= SELECTOR(0x0a14, 0, 2),
	.div		= FRAC_DIVIDER(0x0a14, 4, 12, 8),
	.trig		= TRIGGER(0x0afc, 3),
};

static struct peri_clk_data uartb3_data = {
	.policy		= POLICY(0x0010, 18),
	.gate		= HW_SW_GATE(0x0408, 18, 2, 3),
	.clocks		= CLOCKS("ref_crystal",
				 "var_156m",
				 "ref_156m"),
	.sel		= SELECTOR(0x0a18, 0, 2),
	.div		= FRAC_DIVIDER(0x0a18, 4, 12, 8),
	.trig		= TRIGGER(0x0afc, 4),
};

static struct bus_clk_data bsc1_apb_data = {
	.gate = HW_SW_GATE_AUTO(0x0458, 16, 0, 1),
};

static struct bus_clk_data bsc2_apb_data = {
	.gate = HW_SW_GATE_AUTO(0x045c, 16, 0, 1),
};

static struct bus_clk_data bsc3_apb_data = {
	.gate = HW_SW_GATE_AUTO(0x0470, 16, 0, 1),
};

static struct bus_clk_data bsc4_apb_data = {
	.gate = HW_SW_GATE_AUTO(0x0474, 16, 0, 1),
};

static struct peri_clk_data bsc1_data = {
	.policy		= POLICY(0x0010, 24),
	.gate		= HW_SW_GATE(0x0458, 18, 2, 3),
	.clocks		= CLOCKS("ref_crystal",
				 "var_104m",
				 "ref_104m",
				 "var_13m",
				 "ref_13m"),
	.sel		= SELECTOR(0x0a64, 0, 3),
	.trig		= TRIGGER(0x0afc, 23),
};

static struct peri_clk_data bsc2_data = {
	.policy		= POLICY(0x0010, 23),
	.gate		= HW_SW_GATE(0x045c, 18, 2, 3),
	.clocks		= CLOCKS("ref_crystal",
				 "var_104m",
				 "ref_104m",
				 "var_13m",
				 "ref_13m"),
	.sel		= SELECTOR(0x0a68, 0, 3),
	.trig		= TRIGGER(0x0afc, 24),
};

static struct peri_clk_data bsc3_data = {
	.policy		= POLICY(0x0010, 29),
	.gate		= HW_SW_GATE(0x0470, 18, 2, 3),
	.clocks		= CLOCKS("ref_crystal",
				 "var_104m",
				 "ref_104m",
				 "var_13m",
				 "ref_13m"),
	.sel		= SELECTOR(0x0a7c, 0, 3),
	.trig		= TRIGGER(0x0afc, 18),
};

static struct peri_clk_data bsc4_data = {
	.policy		= POLICY(0x0010, 30),
	.gate		= HW_SW_GATE(0x0474, 18, 2, 3),
	.clocks		= CLOCKS("ref_crystal",
				 "var_104m",
				 "ref_104m",
				 "var_13m",
				 "ref_13m"),
	.sel		= SELECTOR(0x0a80, 0, 3),
	.trig		= TRIGGER(0x0afc, 19),
};

static u32 slave_ccu_freq_tbl0[] = \
	{ 26000000, 26000000, 26000000, 26000000, 26000000 };
static u32 slave_ccu_freq_tbl1[] = \
	{ 52000000, 26000000, 26000000, 26000000, 26000000 };
static u32 slave_ccu_freq_tbl2[] = \
	{ 78000000, 39000000, 39000000, 39000000, 39000000 };
static u32 slave_ccu_freq_tbl3[] = \
	{ 104000000, 52000000, 52000000, 52000000, 52000000 };
static u32 slave_ccu_freq_tbl4[] = \
	{ 156000000, 52000000, 52000000, 52000000, 52000000 };
static u32 slave_ccu_freq_tbl5[] = \
	{ 156000000, 78000000, 78000000, 78000000, 78000000 };

static struct ccu_data slave_ccu_data = {
	BCM21664_CCU_COMMON(slave, SLAVE),
       .policy		= {
		.enable		= CCU_LVM_EN(0x0034, 0),
		.control	= CCU_POLICY_CTL(0x000c, 0, 1, 2),
		.freq		= {
			.offset = 0x0008,

			.policy_count = 6,
			.table_size = 5,
			.tables = {
				slave_ccu_freq_tbl0,
				slave_ccu_freq_tbl1,
				slave_ccu_freq_tbl2,
				slave_ccu_freq_tbl3,
				slave_ccu_freq_tbl4,
				slave_ccu_freq_tbl5,
			},
		},
	},
	.kona_clks	= {
		[BCM21664_SLAVE_CCU_UARTB_APB] =
			KONA_CLK(slave, uartb_apb, bus),
		[BCM21664_SLAVE_CCU_UARTB2_APB] =
			KONA_CLK(slave, uartb2_apb, bus),
		[BCM21664_SLAVE_CCU_UARTB3_APB] =
			KONA_CLK(slave, uartb3_apb, bus),
		[BCM21664_SLAVE_CCU_UARTB] =
			KONA_CLK(slave, uartb, peri),
		[BCM21664_SLAVE_CCU_UARTB2] =
			KONA_CLK(slave, uartb2, peri),
		[BCM21664_SLAVE_CCU_UARTB3] =
			KONA_CLK(slave, uartb3, peri),
		[BCM21664_SLAVE_CCU_BSC1_APB] =
			KONA_CLK(slave, bsc1_apb, bus),
		[BCM21664_SLAVE_CCU_BSC2_APB] =
			KONA_CLK(slave, bsc2_apb, bus),
		[BCM21664_SLAVE_CCU_BSC3_APB] =
			KONA_CLK(slave, bsc3_apb, bus),
		[BCM21664_SLAVE_CCU_BSC4_APB] =
			KONA_CLK(slave, bsc4_apb, bus),
		[BCM21664_SLAVE_CCU_BSC1] =
			KONA_CLK(slave, bsc1, peri),
		[BCM21664_SLAVE_CCU_BSC2] =
			KONA_CLK(slave, bsc2, peri),
		[BCM21664_SLAVE_CCU_BSC3] =
			KONA_CLK(slave, bsc3, peri),
		[BCM21664_SLAVE_CCU_BSC4] =
			KONA_CLK(slave, bsc4, peri),
		[BCM21664_SLAVE_CCU_CLOCK_COUNT] = LAST_KONA_CLK,
	},
};

/* Device tree match table callback functions */

static void __init kona_dt_root_ccu_setup(struct device_node *node)
{
	kona_dt_ccu_setup(&root_ccu_data, node);
}

static void __init kona_dt_aon_ccu_setup(struct device_node *node)
{
	kona_dt_ccu_setup(&aon_ccu_data, node);
}

static void __init kona_dt_master_ccu_setup(struct device_node *node)
{
	kona_dt_ccu_setup(&master_ccu_data, node);
}

static void __init kona_dt_slave_ccu_setup(struct device_node *node)
{
	kona_dt_ccu_setup(&slave_ccu_data, node);
}

CLK_OF_DECLARE(bcm21664_root_ccu, BCM21664_DT_ROOT_CCU_COMPAT,
			kona_dt_root_ccu_setup);
CLK_OF_DECLARE(bcm21664_aon_ccu, BCM21664_DT_AON_CCU_COMPAT,
			kona_dt_aon_ccu_setup);
CLK_OF_DECLARE(bcm21664_master_ccu, BCM21664_DT_MASTER_CCU_COMPAT,
			kona_dt_master_ccu_setup);
CLK_OF_DECLARE(bcm21664_slave_ccu, BCM21664_DT_SLAVE_CCU_COMPAT,
			kona_dt_slave_ccu_setup);
