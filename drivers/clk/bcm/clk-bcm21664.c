// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2014 Broadcom Corporation
 * Copyright 2014 Linaro Limited
 */

#include "clk-kona.h"
#include "dt-bindings/clock/bcm21664.h"

#define BCM21664_CCU_COMMON(_name, _capname) \
	KONA_CCU_COMMON(BCM21664, _name, _capname)

#define BCM23550_CCU_COMMON(_name, _capname) \
	KONA_CCU_COMMON(BCM23550, _name, _capname)

/* Proc CCU */

static struct pll_reg_data a7_pll_data = {
	.cfg		= {
		PLL_CFG_OFFSET(0x0c18, 0, 28),

		.tholds		= {FREQ_MHZ(1750), PLL_CFG_THOLD_MAX},
		.cfg_values	= {0x8000000, 0x8002000},
		.n_tholds	= 2,
	},

	.pwrdwn		= PLL_PWRDWN(0x0c00, 3, 4),
	.reset		= PLL_RESET(0x0c00, 0, 1),
	.lock		= PLL_LOCK(0x0c00, 28),

	.pdiv		= PLL_DIV(0x0c00, 24, 3),
	.ndiv		= PLL_DIV(0x0c00, 8, 9),
	.nfrac		= PLL_NFRAC(0x0c04, 0, 20),

	.desense	= PLL_DESENSE_BOTH(0xc24, -14500000),
	.flags		= FLAG(PLL, AUTOGATE)|FLAG(PLL, DELAYED_LOCK),

	.xtal_name	= "ref_crystal",
};

static struct pll_chnl_reg_data a7_pll_chnl0_data = {
	.enable		= PLL_CHNL_ENABLE(0x0c08, 9),
	.load		= PLL_CHNL_LOAD(0x0c08, 11),
	.mdiv		= PLL_CHNL_MDIV(0x0c08, 0, 8),
	.parent_name	= "a7_pll",
};

static struct pll_chnl_reg_data a7_pll_chnl1_data = {
	.enable		= PLL_CHNL_ENABLE(0x0c20, 9),
	.load		= PLL_CHNL_LOAD(0x0c20, 11),
	.mdiv		= PLL_CHNL_MDIV(0x0c20, 0, 8),
	.parent_name	= "a7_pll",
};

static struct clk_reg_data arm_switch_data = {
	.gate		= HW_SW_GATE(0x0210, 16, 0, 1),
	.hyst		= HYST(0x0210, 9, 8),
};

static struct clk_reg_data cci_data = {
	.gate		= HW_SW_GATE(0x0400, 16, 0, 1),
	.hyst		= HYST(0x0400, 9, 8),
};

static struct ccu_data bcm23550_proc_ccu_data = {
	BCM23550_CCU_COMMON(bcm23550_proc, PROC),
   .policy		= {
		.enable		= CCU_LVM_EN(0x0034, 0),
		.control	= CCU_POLICY_CTL(0x000c, 0, 1, 2),
		.mask		= CCU_POLICY_MASK(0x0010, 0),
	},
	.voltage	= {
		CCU_VOLTAGE_OFFSET(0x0040, 0x0044),
		.voltage_table = {
			CCU_VOLTAGE_A9_ECO,
			CCU_VOLTAGE_A9_ECO,
			CCU_VOLTAGE_A9_ECO,
			CCU_VOLTAGE_A9_ECO,
			CCU_VOLTAGE_A9_TURBO,
			CCU_VOLTAGE_A9_NORMAL,
			CCU_VOLTAGE_A9_TURBO,
			CCU_VOLTAGE_A9_SUPER_TURBO,
		},
		.voltage_table_len = 8,
	},
	.freq_policy	= {
		.offset = 0x0008,
		.freq_policy_table = {
			4, 4, 4, 7 /* ECO, ECO, ECO, SUPER_TURBO */
		},
		.freq_policy_table_len = 4,
	},
	.interrupt		= {
		.enable_offset = 0x0020,
		.status_offset = 0x0024,
	},
	.kona_clks	= {
		[BCM23550_PROC_CCU_A7_PLL] =
			KONA_CLK(bcm23550_proc, a7_pll, pll),
		[BCM23550_PROC_CCU_A7_PLL_CHNL0] =
			KONA_CLK(bcm23550_proc, a7_pll_chnl0, pll_chnl),
		[BCM23550_PROC_CCU_A7_PLL_CHNL1] =
			KONA_CLK(bcm23550_proc, a7_pll_chnl1, pll_chnl),
		[BCM23550_PROC_CCU_ARM_SWITCH] =
			KONA_CLK(bcm23550_proc, arm_switch, bus),
		[BCM23550_PROC_CCU_CCI] =
			KONA_CLK(bcm23550_proc, cci, bus),
		[BCM23550_PROC_CCU_CLOCK_COUNT] = LAST_KONA_CLK,
	},
};

/* Root CCU */

static struct clk_reg_data frac_1m_data = {
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

static struct clk_reg_data hub_timer_apb_data = {
	.gate		= HW_SW_GATE(0x0414, 18, 2, 3),
	.hyst		= HYST(0x0414, 10, 11),
};

static struct clk_reg_data hub_timer_data = {
	.gate		= HW_SW_GATE(0x0414, 16, 0, 1),
	.hyst		= HYST(0x0414, 8, 9),
	.clocks		= CLOCKS("bbl_32k",
				 "frac_1m",
				 "dft_19_5m"),
	.sel		= SELECTOR(0x0a10, 0, 3),
	.trig		= TRIGGER(0x0a40, 4),
};

static struct clk_reg_data pmu_bsc_apb_data = {
	.gate		= HW_SW_GATE(0x0418, 18, 2, 3),
	.hyst		= HYST(0x0418, 10, 11),
};

static struct clk_reg_data pmu_bsc_data = {
	.gate		= HW_SW_GATE(0x0418, 16, 0, 1),
	.hyst		= HYST(0x0418, 8, 9),
	.clocks		= CLOCKS("ref_crystal",
				 "pmu_bsc_var",
				 "bbl_32k"),
	.sel		= SELECTOR(0x0a04, 0, 3),
	.div		= DIVIDER(0x0a04, 3, 4),
	.trig		= TRIGGER(0x0a40, 0),
};

static struct ccu_data aon_ccu_data = {
	BCM21664_CCU_COMMON(aon, AON),
	.policy		= {
		.enable		= CCU_LVM_EN(0x0034, 0),
		.control	= CCU_POLICY_CTL(0x000c, 0, 1, 2),
		.mask		= CCU_POLICY_MASK(0x0010, 0),
	},
	.voltage	= {
		CCU_VOLTAGE_OFFSET(0x0040, 0x0044),
		.voltage_table = {
			CCU_VOLTAGE_ECO,
			CCU_VOLTAGE_ECO,
			CCU_VOLTAGE_ECO,
			CCU_VOLTAGE_ECO,
			CCU_VOLTAGE_ECO,
		},
		.voltage_table_len = 5,
	},
	.peri_volt	= {
		.offset = 0x0030,
		.peri_volt_table = {
			CCU_PERI_VOLT_NORMAL,
			CCU_PERI_VOLT_HIGH,
		},
		.peri_volt_table_len = 2,
	},
	.freq_policy	= {
		.offset = 0x0008,
		.freq_policy_table = {
			2, 2, 2, 2 /* ECO, ECO, NORMAL, NORMAL */
		},
		.freq_policy_table_len = 4,
	},
	.interrupt		= {
		.enable_offset = 0x0020,
		.status_offset = 0x0024,
	},
	.kona_clks	= {
		[BCM21664_AON_CCU_HUB_TIMER_APB] =
			KONA_CLK(aon, hub_timer_apb, bus),
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

static struct clk_reg_data sdio1_ahb_data = {
	.gate		= HW_SW_GATE(0x0358, 16, 0, 1),
};

static struct clk_reg_data sdio2_ahb_data = {
	.gate		= HW_SW_GATE(0x035c, 16, 0, 1),
};

static struct clk_reg_data sdio3_ahb_data = {
	.gate		= HW_SW_GATE(0x0364, 16, 0, 1),
};

static struct clk_reg_data sdio4_ahb_data = {
	.gate		= HW_SW_GATE(0x0360, 16, 0, 1),
};

static struct clk_reg_data sdio1_data = {
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

static struct clk_reg_data sdio2_data = {
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

static struct clk_reg_data sdio3_data = {
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

static struct clk_reg_data sdio4_data = {
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

static struct clk_reg_data sdio1_sleep_data = {
	.clocks		= CLOCKS("ref_32k"),	/* Verify */
	.gate		= HW_SW_GATE(0x0358, 18, 2, 3),
};

static struct clk_reg_data sdio2_sleep_data = {
	.clocks		= CLOCKS("ref_32k"),	/* Verify */
	.gate		= HW_SW_GATE(0x035c, 18, 2, 3),
};

static struct clk_reg_data sdio3_sleep_data = {
	.clocks		= CLOCKS("ref_32k"),	/* Verify */
	.gate		= HW_SW_GATE(0x0364, 18, 2, 3),
};

static struct clk_reg_data sdio4_sleep_data = {
	.clocks		= CLOCKS("ref_32k"),	/* Verify */
	.gate		= HW_SW_GATE(0x0360, 18, 2, 3),
};

static struct clk_reg_data usb_otg_ahb_data = {
	.gate		= HW_SW_GATE(0x0348, 16, 0, 1),
};

static struct ccu_data master_ccu_data = {
	BCM21664_CCU_COMMON(master, MASTER),
	.policy		= {
		.enable		= CCU_LVM_EN(0x0034, 0),
		.control	= CCU_POLICY_CTL(0x000c, 0, 1, 2),
		.mask		= CCU_POLICY_MASK(0x0010, 0),
	},
	.voltage	= {
		CCU_VOLTAGE_OFFSET(0x0040, 0x0044),
		.voltage_table = {
			CCU_VOLTAGE_ECO,
			CCU_VOLTAGE_ECO,
			CCU_VOLTAGE_ECO,
			CCU_VOLTAGE_ECO,
			CCU_VOLTAGE_ECO,
			CCU_VOLTAGE_ECO,
			CCU_VOLTAGE_ECO,
			CCU_VOLTAGE_ECO,
		},
		.voltage_table_len = 8,
	},
	.peri_volt	= {
		.offset = 0x0030,
		.peri_volt_table = {
			CCU_PERI_VOLT_NORMAL,
			CCU_PERI_VOLT_HIGH,
		},
		.peri_volt_table_len = 2,
	},
	.freq_policy	= {
		.offset = 0x0008,
		.freq_policy_table = {
			3, 3, 3, 3 /* ECO, ECO, NORMAL, NORMAL */
		},
		.freq_policy_table_len = 4,
	},
	.interrupt		= {
		.enable_offset = 0x0020,
		.status_offset = 0x0024,
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

static struct clk_reg_data uartb_apb_data = {
	.gate		= HW_SW_GATE_AUTO(0x0400, 16, 0, 1),
};

static struct clk_reg_data uartb2_apb_data = {
	.gate		= HW_SW_GATE_AUTO(0x0404, 16, 0, 1),
};

static struct clk_reg_data uartb3_apb_data = {
	.gate		= HW_SW_GATE_AUTO(0x0408, 16, 0, 1),
};

static struct clk_reg_data uartb_data = {
	.gate		= HW_SW_GATE(0x0400, 18, 2, 3),
	.clocks		= CLOCKS("ref_crystal",
				 "var_156m",
				 "ref_156m"),
	.sel		= SELECTOR(0x0a10, 0, 2),
	.div		= FRAC_DIVIDER(0x0a10, 4, 12, 8),
	.trig		= TRIGGER(0x0afc, 2),
};

static struct clk_reg_data uartb2_data = {
	.gate		= HW_SW_GATE(0x0404, 18, 2, 3),
	.clocks		= CLOCKS("ref_crystal",
				 "var_156m",
				 "ref_156m"),
	.sel		= SELECTOR(0x0a14, 0, 2),
	.div		= FRAC_DIVIDER(0x0a14, 4, 12, 8),
	.trig		= TRIGGER(0x0afc, 3),
};

static struct clk_reg_data uartb3_data = {
	.gate		= HW_SW_GATE(0x0408, 18, 2, 3),
	.clocks		= CLOCKS("ref_crystal",
				 "var_156m",
				 "ref_156m"),
	.sel		= SELECTOR(0x0a18, 0, 2),
	.div		= FRAC_DIVIDER(0x0a18, 4, 12, 8),
	.trig		= TRIGGER(0x0afc, 4),
};

static struct clk_reg_data bsc1_apb_data = {
	.gate = HW_SW_GATE_AUTO(0x0458, 16, 0, 1),
};

static struct clk_reg_data bsc2_apb_data = {
	.gate = HW_SW_GATE_AUTO(0x045c, 16, 0, 1),
};

static struct clk_reg_data bsc3_apb_data = {
	.gate = HW_SW_GATE_AUTO(0x0470, 16, 0, 1),
};

static struct clk_reg_data bsc4_apb_data = {
	.gate = HW_SW_GATE_AUTO(0x0474, 16, 0, 1),
};

static struct clk_reg_data bsc1_data = {
	.gate		= HW_SW_GATE(0x0458, 18, 2, 3),
	.clocks		= CLOCKS("ref_crystal",
				 "var_104m",
				 "ref_104m",
				 "var_13m",
				 "ref_13m"),
	.sel		= SELECTOR(0x0a64, 0, 3),
	.trig		= TRIGGER(0x0afc, 23),
};

static struct clk_reg_data bsc2_data = {
	.gate		= HW_SW_GATE(0x045c, 18, 2, 3),
	.clocks		= CLOCKS("ref_crystal",
				 "var_104m",
				 "ref_104m",
				 "var_13m",
				 "ref_13m"),
	.sel		= SELECTOR(0x0a68, 0, 3),
	.trig		= TRIGGER(0x0afc, 24),
};

static struct clk_reg_data bsc3_data = {
	.gate		= HW_SW_GATE(0x0470, 18, 2, 3),
	.clocks		= CLOCKS("ref_crystal",
				 "var_104m",
				 "ref_104m",
				 "var_13m",
				 "ref_13m"),
	.sel		= SELECTOR(0x0a7c, 0, 3),
	.trig		= TRIGGER(0x0afc, 18),
};

static struct clk_reg_data bsc4_data = {
	.gate		= HW_SW_GATE(0x0474, 18, 2, 3),
	.clocks		= CLOCKS("ref_crystal",
				 "var_104m",
				 "ref_104m",
				 "var_13m",
				 "ref_13m"),
	.sel		= SELECTOR(0x0a80, 0, 3),
	.trig		= TRIGGER(0x0afc, 19),
};

static struct ccu_data slave_ccu_data = {
	BCM21664_CCU_COMMON(slave, SLAVE),
   .policy		= {
		.enable		= CCU_LVM_EN(0x0034, 0),
		.control	= CCU_POLICY_CTL(0x000c, 0, 1, 2),
		.mask		= CCU_POLICY_MASK(0x0010, 0),
	},
	.voltage	= {
		CCU_VOLTAGE_OFFSET(0x0040, 0x0044),
		.voltage_table = {
			CCU_VOLTAGE_ECO,
			CCU_VOLTAGE_ECO,
			CCU_VOLTAGE_ECO,
			CCU_VOLTAGE_ECO,
			CCU_VOLTAGE_ECO,
			CCU_VOLTAGE_ECO,
		},
		.voltage_table_len = 6,
	},
	.peri_volt	= {
		.offset = 0x0030,
		.peri_volt_table = {
			CCU_PERI_VOLT_NORMAL,
			CCU_PERI_VOLT_HIGH,
		},
		.peri_volt_table_len = 2,
	},
	.freq_policy	= {
		.offset = 0x0008,
		.freq_policy_table = {
			3, 3, 3, 3 /* ECO, ECO, NORMAL, NORMAL */
		},
		.freq_policy_table_len = 4,
	},
	.interrupt		= {
		.enable_offset = 0x0020,
		.status_offset = 0x0024,
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

static void __init bcm23550_dt_proc_ccu_setup(struct device_node *node)
{
	kona_dt_ccu_setup(&bcm23550_proc_ccu_data, node);
}

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

CLK_OF_DECLARE(bcm23550_proc_ccu, BCM23550_DT_PROC_CCU_COMPAT,
			bcm23550_dt_proc_ccu_setup);
CLK_OF_DECLARE(bcm21664_root_ccu, BCM21664_DT_ROOT_CCU_COMPAT,
			kona_dt_root_ccu_setup);
CLK_OF_DECLARE(bcm21664_aon_ccu, BCM21664_DT_AON_CCU_COMPAT,
			kona_dt_aon_ccu_setup);
CLK_OF_DECLARE(bcm21664_master_ccu, BCM21664_DT_MASTER_CCU_COMPAT,
			kona_dt_master_ccu_setup);
CLK_OF_DECLARE(bcm21664_slave_ccu, BCM21664_DT_SLAVE_CCU_COMPAT,
			kona_dt_slave_ccu_setup);
