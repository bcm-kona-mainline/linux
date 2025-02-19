// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2013 Broadcom Corporation
 * Copyright 2013 Linaro Limited
 */

#include "clk-kona.h"
#include "dt-bindings/clock/bcm281xx.h"

#define BCM281XX_CCU_COMMON(_name, _ucase_name) \
	KONA_CCU_COMMON(BCM281XX, _name, _ucase_name)

/* Root CCU */

static struct peri_clk_data frac_1m_data = {
	.gate		= HW_SW_GATE(0x214, 16, 0, 1),
	.trig		= TRIGGER(0x0e04, 0),
	.div		= FRAC_DIVIDER(0x0e00, 0, 22, 16),
	.clocks		= CLOCKS("ref_crystal"),
};

static struct ccu_data root_ccu_data = {
	BCM281XX_CCU_COMMON(root, ROOT),
	.kona_clks	= {
		[BCM281XX_ROOT_CCU_FRAC_1M] =
			KONA_CLK(root, frac_1m, peri),
		[BCM281XX_ROOT_CCU_CLOCK_COUNT] = LAST_KONA_CLK,
	},
};

/* AON CCU */

static struct peri_clk_data hub_timer_data = {
	.policy		= POLICY(0x0014, 6),
	.gate		= HW_SW_GATE(0x0414, 16, 0, 1),
	.clocks		= CLOCKS("bbl_32k",
				 "frac_1m",
				 "dft_19_5m"),
	.sel		= SELECTOR(0x0a10, 0, 2),
	.trig		= TRIGGER(0x0a40, 4),
};

static struct peri_clk_data pmu_bsc_data = {
	.policy		= POLICY(0x0014, 8),
	.gate		= HW_SW_GATE(0x0418, 16, 0, 1),
	.clocks		= CLOCKS("ref_crystal",
				 "pmu_bsc_var",
				 "bbl_32k"),
	.sel		= SELECTOR(0x0a04, 0, 2),
	.div		= DIVIDER(0x0a04, 3, 4),
	.trig		= TRIGGER(0x0a40, 0),
};

static struct peri_clk_data pmu_bsc_var_data = {
	.policy		= POLICY(0x0014, 8),
	.clocks		= CLOCKS("var_312m",
				 "ref_312m"),
	.sel		= SELECTOR(0x0a00, 0, 2),
	.div		= DIVIDER(0x0a00, 4, 5),
	.trig		= TRIGGER(0x0a40, 2),
};

static struct bus_clk_data hub_timer_apb_data = {
	.policy		= POLICY(0x0014, 6),
	.gate		= HW_SW_GATE(0x0414, 18, 3, 2),
	.hyst		= HYST(0x0414, 10, 11),
};

static struct bus_clk_data pmu_bsc_apb_data = {
	.policy		= POLICY(0x0014, 8),
	.gate		= HW_SW_GATE(0x0418, 18, 3, 2),
	.hyst		= HYST(0x0418, 10, 11),
};

static struct ccu_data aon_ccu_data = {
	BCM281XX_CCU_COMMON(aon, AON),
	.kona_clks	= {
		[BCM281XX_AON_CCU_HUB_TIMER] =
			KONA_CLK_PREREQ(aon, hub_timer, peri, hub_timer_apb),
		[BCM281XX_AON_CCU_PMU_BSC] =
			KONA_CLK_PREREQ(aon, pmu_bsc, peri, pmu_bsc_apb),
		[BCM281XX_AON_CCU_PMU_BSC_VAR] =
			KONA_CLK(aon, pmu_bsc_var, peri),
		[BCM281XX_AON_CCU_HUB_TIMER_APB] =
			KONA_CLK(aon, hub_timer_apb, bus),
		[BCM281XX_AON_CCU_PMU_BSC_APB] =
			KONA_CLK(aon, pmu_bsc_apb, bus),
		[BCM281XX_AON_CCU_CLOCK_COUNT] = LAST_KONA_CLK,
	},
};

/* Hub CCU */

static struct peri_clk_data tmon_1m_data = {
	.gate		= HW_SW_GATE(0x04a4, 18, 2, 3),
	.clocks		= CLOCKS("ref_crystal",
				 "frac_1m"),
	.sel		= SELECTOR(0x0e74, 0, 2),
	.trig		= TRIGGER(0x0e84, 1),
};

static struct ccu_data hub_ccu_data = {
	BCM281XX_CCU_COMMON(hub, HUB),
	.kona_clks	= {
		[BCM281XX_HUB_CCU_TMON_1M] =
			KONA_CLK(hub, tmon_1m, peri),
		[BCM281XX_HUB_CCU_CLOCK_COUNT] = LAST_KONA_CLK,
	},
};

/* Master CCU */

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

static struct peri_clk_data usb_ic_data = {
	.policy		= POLICY(0x0010, 9),
	.gate		= HW_SW_GATE(0x0354, 18, 2, 3),
	.clocks		= CLOCKS("ref_crystal",
				 "var_96m",
				 "ref_96m"),
	.div		= FIXED_DIVIDER(2),
	.sel		= SELECTOR(0x0a24, 0, 2),
	.trig		= TRIGGER(0x0afc, 7),
};

/* also called usbh_48m */
static struct peri_clk_data hsic2_48m_data = {
	.policy		= POLICY(0x0010, 16),
	.gate		= HW_SW_GATE(0x0370, 18, 2, 3),
	.clocks		= CLOCKS("ref_crystal",
				 "var_96m",
				 "ref_96m"),
	.sel		= SELECTOR(0x0a38, 0, 2),
	.div		= FIXED_DIVIDER(2),
	.trig		= TRIGGER(0x0afc, 5),
};

/* also called usbh_12m */
static struct peri_clk_data hsic2_12m_data = {
	.policy		= POLICY(0x0010, 16),
	.gate		= HW_SW_GATE(0x0370, 20, 4, 5),
	.div		= DIVIDER(0x0a38, 12, 2),
	.clocks		= CLOCKS("ref_crystal",
				 "var_96m",
				 "ref_96m"),
	.pre_div	= FIXED_DIVIDER(2),
	.sel		= SELECTOR(0x0a38, 0, 2),
	.trig		= TRIGGER(0x0afc, 5),
};

static struct bus_clk_data sdio1_ahb_data = {
	.policy		= POLICY(0x0010, 6),
	.gate		= HW_SW_GATE(0x0358, 16, 1, 0),
};

static struct bus_clk_data sdio2_ahb_data = {
	.policy		= POLICY(0x0010, 5),
	.gate		= HW_SW_GATE(0x035c, 16, 1, 0),
};

static struct bus_clk_data sdio3_ahb_data = {
	.policy		= POLICY(0x0010, 3),
	.gate		= HW_SW_GATE(0x0364, 16, 1, 0),
};

static struct bus_clk_data sdio4_ahb_data = {
	.policy		= POLICY(0x0010, 4),
	.gate		= HW_SW_GATE(0x0360, 16, 1, 0),
};

static struct bus_clk_data usb_ic_ahb_data = {
	.policy		= POLICY(0x0010, 9),
	.gate		= HW_SW_GATE(0x0354, 16, 1, 0),
};

/* also called usbh_ahb */
static struct bus_clk_data hsic2_ahb_data = {
	.policy		= POLICY(0x0010, 16),
	.gate		= HW_SW_GATE(0x0370, 16, 1, 0),
};

static struct bus_clk_data usb_otg_ahb_data = {
	.policy		= POLICY(0x0010, 11),
	.gate		= HW_SW_GATE(0x0348, 16, 1, 0),
};

static struct ccu_data master_ccu_data = {
	BCM281XX_CCU_COMMON(master, MASTER),
	.kona_clks	= {
		[BCM281XX_MASTER_CCU_SDIO1] =
			KONA_CLK_PREREQ(master, sdio1, peri, sdio1_ahb),
		[BCM281XX_MASTER_CCU_SDIO2] =
			KONA_CLK_PREREQ(master, sdio2, peri, sdio2_ahb),
		[BCM281XX_MASTER_CCU_SDIO3] =
			KONA_CLK_PREREQ(master, sdio3, peri, sdio3_ahb),
		[BCM281XX_MASTER_CCU_SDIO4] =
			KONA_CLK_PREREQ(master, sdio4, peri, sdio4_ahb),
		[BCM281XX_MASTER_CCU_USB_IC] =
			KONA_CLK_PREREQ(master, usb_ic, peri, usb_ic_ahb),
		[BCM281XX_MASTER_CCU_HSIC2_48M] =
			KONA_CLK_PREREQ(master, hsic2_48m, peri, hsic2_ahb),
		[BCM281XX_MASTER_CCU_HSIC2_12M] =
			KONA_CLK_PREREQ(master, hsic2_12m, peri, hsic2_ahb),
		[BCM281XX_MASTER_CCU_SDIO1_AHB] =
			KONA_CLK(master, sdio1_ahb, bus),
		[BCM281XX_MASTER_CCU_SDIO2_AHB] =
			KONA_CLK(master, sdio2_ahb, bus),
		[BCM281XX_MASTER_CCU_SDIO3_AHB] =
			KONA_CLK(master, sdio3_ahb, bus),
		[BCM281XX_MASTER_CCU_SDIO4_AHB] =
			KONA_CLK(master, sdio4_ahb, bus),
		[BCM281XX_MASTER_CCU_USB_IC_AHB] =
			KONA_CLK(master, usb_ic_ahb, bus),
		[BCM281XX_MASTER_CCU_HSIC2_AHB] =
			KONA_CLK(master, hsic2_ahb, bus),
		[BCM281XX_MASTER_CCU_USB_OTG_AHB] =
			KONA_CLK(master, usb_otg_ahb, bus),
		[BCM281XX_MASTER_CCU_CLOCK_COUNT] = LAST_KONA_CLK,
	},
};

/* Slave CCU */

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

static struct peri_clk_data uartb4_data = {
	.policy		= POLICY(0x0010, 17),
	.gate		= HW_SW_GATE(0x0408, 18, 2, 3),
	.clocks		= CLOCKS("ref_crystal",
				 "var_156m",
				 "ref_156m"),
	.sel		= SELECTOR(0x0a1c, 0, 2),
	.div		= FRAC_DIVIDER(0x0a1c, 4, 12, 8),
	.trig		= TRIGGER(0x0afc, 5),
};

static struct peri_clk_data ssp0_data = {
	.policy		= POLICY(0x0010, 16),
	.gate		= HW_SW_GATE(0x0410, 18, 2, 3),
	.clocks		= CLOCKS("ref_crystal",
				 "var_104m",
				 "ref_104m",
				 "var_96m",
				 "ref_96m"),
	.sel		= SELECTOR(0x0a20, 0, 3),
	.div		= DIVIDER(0x0a20, 4, 14),
	.trig		= TRIGGER(0x0afc, 6),
};

static struct peri_clk_data ssp2_data = {
	.policy		= POLICY(0x0010, 14),
	.gate		= HW_SW_GATE(0x0418, 18, 2, 3),
	.clocks		= CLOCKS("ref_crystal",
				 "var_104m",
				 "ref_104m",
				 "var_96m",
				 "ref_96m"),
	.sel		= SELECTOR(0x0a28, 0, 3),
	.div		= DIVIDER(0x0a28, 4, 14),
	.trig		= TRIGGER(0x0afc, 8),
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
	.clocks	= CLOCKS("ref_crystal",
				 "var_104m",
				 "ref_104m",
				 "var_13m",
				 "ref_13m"),
	.sel		= SELECTOR(0x0a68, 0, 3),
	.trig		= TRIGGER(0x0afc, 24),
};

static struct peri_clk_data bsc3_data = {
	.policy		= POLICY(0x0018, 4),
	.gate		= HW_SW_GATE(0x0484, 18, 2, 3),
	.clocks		= CLOCKS("ref_crystal",
				 "var_104m",
				 "ref_104m",
				 "var_13m",
				 "ref_13m"),
	.sel		= SELECTOR(0x0a84, 0, 3),
	.trig		= TRIGGER(0x0b00, 2),
};

static struct peri_clk_data pwm_data = {
	.policy		= POLICY(0x0010, 21),
	.gate		= HW_SW_GATE(0x0468, 18, 2, 3),
	.clocks		= CLOCKS("ref_crystal",
				 "var_104m"),
	.sel		= SELECTOR(0x0a70, 0, 2),
	.div		= DIVIDER(0x0a70, 4, 3),
	.trig		= TRIGGER(0x0afc, 15),
};

static struct bus_clk_data uartb_apb_data = {
	.policy		= POLICY(0x0010, 20),
	.gate		= HW_SW_GATE(0x0400, 16, 1, 0),
};

static struct bus_clk_data uartb2_apb_data = {
	.policy		= POLICY(0x0010, 19),
	.gate		= HW_SW_GATE(0x0404, 16, 1, 0),
};

static struct bus_clk_data uartb3_apb_data = {
	.policy		= POLICY(0x0010, 18),
	.gate		= HW_SW_GATE(0x0408, 16, 1, 0),
};

static struct bus_clk_data uartb4_apb_data = {
	.policy		= POLICY(0x0010, 17),
	.gate		= HW_SW_GATE(0x040c, 16, 1, 0),
};

static struct bus_clk_data ssp0_apb_data = {
	.policy		= POLICY(0x0010, 16),
	.gate		= HW_SW_GATE(0x0410, 16, 1, 0),
};

static struct bus_clk_data ssp2_apb_data = {
	.policy		= POLICY(0x0010, 14),
	.gate		= HW_SW_GATE(0x0418, 16, 1, 0),
};

static struct bus_clk_data bsc1_apb_data = {
	.policy		= POLICY(0x0010, 24),
	.gate		= HW_SW_GATE(0x0458, 16, 1, 0),
	.hyst		= HYST(0x0458, 8, 9),
};

static struct bus_clk_data bsc2_apb_data = {
	.policy		= POLICY(0x0010, 23),
	.gate		= HW_SW_GATE(0x045c, 16, 1, 0),
	.hyst		= HYST(0x045c, 8, 9),
};

static struct bus_clk_data bsc3_apb_data = {
	.policy		= POLICY(0x0018, 4),
	.gate		= HW_SW_GATE(0x0484, 16, 1, 0),
	.hyst		= HYST(0x0484, 8, 9),
};

static struct bus_clk_data pwm_apb_data = {
	.policy		= POLICY(0x0010, 21),
	.gate		= HW_SW_GATE(0x0468, 16, 1, 0),
	.hyst		= HYST(0x0468, 8, 9),
};

static struct ccu_data slave_ccu_data = {
	BCM281XX_CCU_COMMON(slave, SLAVE),
	.kona_clks	= {
		[BCM281XX_SLAVE_CCU_UARTB] =
			KONA_CLK_PREREQ(slave, uartb, peri, uartb_apb),
		[BCM281XX_SLAVE_CCU_UARTB2] =
			KONA_CLK_PREREQ(slave, uartb2, peri, uartb2_apb),
		[BCM281XX_SLAVE_CCU_UARTB3] =
			KONA_CLK_PREREQ(slave, uartb3, peri, uartb3_apb),
		[BCM281XX_SLAVE_CCU_UARTB4] =
			KONA_CLK_PREREQ(slave, uartb4, peri, uartb4_apb),
		[BCM281XX_SLAVE_CCU_SSP0] =
			KONA_CLK_PREREQ(slave, ssp0, peri, ssp0_apb),
		[BCM281XX_SLAVE_CCU_SSP2] =
			KONA_CLK_PREREQ(slave, ssp2, peri, ssp2_apb),
		[BCM281XX_SLAVE_CCU_BSC1] =
			KONA_CLK_PREREQ(slave, bsc1, peri, bsc1_apb),
		[BCM281XX_SLAVE_CCU_BSC2] =
			KONA_CLK_PREREQ(slave, bsc2, peri, bsc2_apb),
		[BCM281XX_SLAVE_CCU_BSC3] =
			KONA_CLK_PREREQ(slave, bsc3, peri, bsc3_apb),
		[BCM281XX_SLAVE_CCU_PWM] =
			KONA_CLK_PREREQ(slave, pwm, peri, pwm_apb),
		[BCM281XX_SLAVE_CCU_UARTB_APB] =
			KONA_CLK(slave, uartb_apb, bus),
		[BCM281XX_SLAVE_CCU_UARTB2_APB] =
			KONA_CLK(slave, uartb2_apb, bus),
		[BCM281XX_SLAVE_CCU_UARTB3_APB] =
			KONA_CLK(slave, uartb3_apb, bus),
		[BCM281XX_SLAVE_CCU_UARTB4_APB] =
			KONA_CLK(slave, uartb4_apb, bus),
		[BCM281XX_SLAVE_CCU_SSP0_APB] =
			KONA_CLK(slave, ssp0_apb, bus),
		[BCM281XX_SLAVE_CCU_SSP2_APB] =
			KONA_CLK(slave, ssp2_apb, bus),
		[BCM281XX_SLAVE_CCU_BSC1_APB] =
			KONA_CLK(slave, bsc1_apb, bus),
		[BCM281XX_SLAVE_CCU_BSC2_APB] =
			KONA_CLK(slave, bsc2_apb, bus),
		[BCM281XX_SLAVE_CCU_BSC3_APB] =
			KONA_CLK(slave, bsc3_apb, bus),
		[BCM281XX_SLAVE_CCU_PWM_APB] =
			KONA_CLK(slave, pwm_apb, bus),
		[BCM281XX_SLAVE_CCU_CLOCK_COUNT] = LAST_KONA_CLK,
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

static void __init kona_dt_hub_ccu_setup(struct device_node *node)
{
	kona_dt_ccu_setup(&hub_ccu_data, node);
}

static void __init kona_dt_master_ccu_setup(struct device_node *node)
{
	kona_dt_ccu_setup(&master_ccu_data, node);
}

static void __init kona_dt_slave_ccu_setup(struct device_node *node)
{
	kona_dt_ccu_setup(&slave_ccu_data, node);
}

CLK_OF_DECLARE(bcm281xx_root_ccu, BCM281XX_DT_ROOT_CCU_COMPAT,
			kona_dt_root_ccu_setup);
CLK_OF_DECLARE(bcm281xx_aon_ccu, BCM281XX_DT_AON_CCU_COMPAT,
			kona_dt_aon_ccu_setup);
CLK_OF_DECLARE(bcm281xx_hub_ccu, BCM281XX_DT_HUB_CCU_COMPAT,
			kona_dt_hub_ccu_setup);
CLK_OF_DECLARE(bcm281xx_master_ccu, BCM281XX_DT_MASTER_CCU_COMPAT,
			kona_dt_master_ccu_setup);
CLK_OF_DECLARE(bcm281xx_slave_ccu, BCM281XX_DT_SLAVE_CCU_COMPAT,
			kona_dt_slave_ccu_setup);
