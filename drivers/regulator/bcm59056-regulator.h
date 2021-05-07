/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * bcm59056-regulator.h - Regulator definitions for BCM59056
 * (bcm590xx-regulator driver)
 */

#ifndef __BCM59056_REGISTERS_H__
#define __BCM59056_REGISTERS_H__

#include <linux/mfd/bcm590xx.h>

/* I2C slave 0 registers */
#define BCM59056_RFLDOPMCTRL1	0x60
#define BCM59056_IOSR1PMCTRL1	0x7a
#define BCM59056_IOSR2PMCTRL1	0x7c
#define BCM59056_CSRPMCTRL1	0x7e
#define BCM59056_SDSR1PMCTRL1	0x82
#define BCM59056_SDSR2PMCTRL1	0x86
#define BCM59056_MSRPMCTRL1	0x8a
#define BCM59056_VSRPMCTRL1	0x8e
#define BCM59056_RFLDOCTRL	0x96
#define BCM59056_CSRVOUT1	0xc0

/* I2C slave 1 registers */
#define BCM59056_GPLDO5PMCTRL1	0x16
#define BCM59056_GPLDO6PMCTRL1	0x18
#define BCM59056_GPLDO1CTRL	0x1a
#define BCM59056_GPLDO2CTRL	0x1b
#define BCM59056_GPLDO3CTRL	0x1c
#define BCM59056_GPLDO4CTRL	0x1d
#define BCM59056_GPLDO5CTRL	0x1e
#define BCM59056_GPLDO6CTRL	0x1f
#define BCM59056_OTG_CTRL	0x40
#define BCM59056_GPLDO1PMCTRL1	0x57
#define BCM59056_GPLDO2PMCTRL1	0x59
#define BCM59056_GPLDO3PMCTRL1	0x5b
#define BCM59056_GPLDO4PMCTRL1	0x5d

/*
 * RFLDO to VSR regulators are
 * accessed via I2C slave 0
 */

/* LDO regulator IDs */
#define BCM59056_REG_RFLDO	0
#define BCM59056_REG_CAMLDO1	1
#define BCM59056_REG_CAMLDO2	2
#define BCM59056_REG_SIMLDO1	3
#define BCM59056_REG_SIMLDO2	4
#define BCM59056_REG_SDLDO	5
#define BCM59056_REG_SDXLDO	6
#define BCM59056_REG_MMCLDO1	7
#define BCM59056_REG_MMCLDO2	8
#define BCM59056_REG_AUDLDO	9
#define BCM59056_REG_MICLDO	10
#define BCM59056_REG_USBLDO	11
#define BCM59056_REG_VIBLDO	12

/* DCDC regulator IDs */
#define BCM59056_REG_CSR	13
#define BCM59056_REG_IOSR1	14
#define BCM59056_REG_IOSR2	15
#define BCM59056_REG_MSR	16
#define BCM59056_REG_SDSR1	17
#define BCM59056_REG_SDSR2	18
#define BCM59056_REG_VSR	19

/*
 * GPLDO1 to VBUS regulators are
 * accessed via I2C slave 1
 */

#define BCM59056_REG_GPLDO1	20
#define BCM59056_REG_GPLDO2	21
#define BCM59056_REG_GPLDO3	22
#define BCM59056_REG_GPLDO4	23
#define BCM59056_REG_GPLDO5	24
#define BCM59056_REG_GPLDO6	25
#define BCM59056_REG_VBUS	26

#define BCM59056_NUM_REGS	27

#define BCM59056_REG_IS_LDO(n)	(n < BCM59056_REG_CSR)
#define BCM59056_REG_IS_GPLDO(n) \
	((n > BCM59056_REG_VSR) && (n < BCM59056_REG_VBUS))
#define BCM59056_REG_IS_VBUS(n)	(n == BCM59056_REG_VBUS)

static struct bcm590xx_info bcm59056_regs[] = {
	BCM590XX_REG_TABLE(rfldo, ldo_a_table),
	BCM590XX_REG_TABLE(camldo1, ldo_c_table),
	BCM590XX_REG_TABLE(camldo2, ldo_c_table),
	BCM590XX_REG_TABLE(simldo1, ldo_a_table),
	BCM590XX_REG_TABLE(simldo2, ldo_a_table),
	BCM590XX_REG_TABLE(sdldo, ldo_c_table),
	BCM590XX_REG_TABLE(sdxldo, ldo_a_table),
	BCM590XX_REG_TABLE(mmcldo1, ldo_a_table),
	BCM590XX_REG_TABLE(mmcldo2, ldo_a_table),
	BCM590XX_REG_TABLE(audldo, ldo_a_table),
	BCM590XX_REG_TABLE(micldo, ldo_a_table),
	BCM590XX_REG_TABLE(usbldo, ldo_a_table),
	BCM590XX_REG_TABLE(vibldo, ldo_c_table),
	BCM590XX_REG_RANGES(csr, dcdc_csr_ranges),
	BCM590XX_REG_RANGES(iosr1, dcdc_iosr1_ranges),
	BCM590XX_REG_RANGES(iosr2, dcdc_iosr1_ranges),
	BCM590XX_REG_RANGES(msr, dcdc_iosr1_ranges),
	BCM590XX_REG_RANGES(sdsr1, dcdc_sdsr1_ranges),
	BCM590XX_REG_RANGES(sdsr2, dcdc_iosr1_ranges),
	BCM590XX_REG_RANGES(vsr, dcdc_iosr1_ranges),
	BCM590XX_REG_TABLE(gpldo1, ldo_a_table),
	BCM590XX_REG_TABLE(gpldo2, ldo_a_table),
	BCM590XX_REG_TABLE(gpldo3, ldo_a_table),
	BCM590XX_REG_TABLE(gpldo4, ldo_a_table),
	BCM590XX_REG_TABLE(gpldo5, ldo_a_table),
	BCM590XX_REG_TABLE(gpldo6, ldo_a_table),
	BCM590XX_REG_TABLE(vbus, ldo_vbus),
};

#endif /* __BCM59056_REGISTERS_H__ */
