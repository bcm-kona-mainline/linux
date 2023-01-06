// SPDX-License-Identifier: GPL-2.0-only

#ifndef __BCM_KONA_PWRMGR_H__
#define __BCM_KONA_PWRMGR_H__

#define PWRMGR_REV00 0
#define PWRMGR_REV02 2

/* Offsets */

#define EVENT_OFFSET_FOR_ID(event_id)	(event_id * 4)

#define PWRMGR_VO0_I2C_CMD_PTR_OFFSET	0x4008 /* V01 - 0x400C, V02 - 0x4010 */
#define PWRMGR_VO0_I2C_CMD_ADDL_PTR_OFFSET	0x41C0 /* V01 - 0x41C4, V02 - 0x41C8 */

#define PWRMGR_PC_PIN_OVERRIDE_CTRL_OFFSET 0x4028
#define PWRMGR_I2C_ENABLE_OFFSET	0x4100
#define PWRMGR_SEQ_CMD_BANK0_OFFSET	0x4104
#define PWRMGR_SEQ_CMD_BANK1_OFFSET	0x4280

#define SEQ_CMD_OFFSET_TO_BANK(x)		((x) >> 6)

/* Pin control */

#define PC_SW_VALUE_MASK_FOR(x)		(0x00010000 << (x))
#define PC_SW_ENABLE_MASK_FOR(x)	(0x00001000 << (x))
#define PC_CLKREQ_VALUE_MASK_FOR(x)	(0x00000100 << (x))
#define PC_CLKREQ_ENABLE_MASK_FOR(x)	(0x00000010 << (x))

/* Power Islands */

struct kona_pi_info {
	const char *name;

	const struct pi_state *states;

	u32 policy_reg_offset;
	u32 ac_shift;
	u32 atl_shift;
	u32 pm_policy_shift;
};

struct pi_state {
	u32 state_id;
	u32 policy;
	u32 wakeup_latency;
};

enum pi_opp {
	OPP_XTAL,
	OPP_ECONOMY,
	OPP_NORMAL,
	OPP_TURBO,
	OPP_SUPER_TURBO,
	OPP_MAX
};

#define PI_POLICY_OFF	0
#define PI_POLICY_RET	1
#define PI_POLICY_ECO	4
#define PI_POLICY_DFS	5
#define PI_POLICY_6	6
#define PI_POLICY_WKP	7

enum {
	PI_STATE_ACTIVE,
	PI_STATE_RETENTION,
	PI_STATE_SHUTDOWN,
};

enum {
	ARM_CORE_STATE_ACTIVE,
	ARM_CORE_STATE_SUSPEND,
	ARM_CORE_STATE_RETENTION,
	ARM_CORE_STATE_DORMANT,
};

#define PI_POLICY_MASK	0x0700

/* Events */

enum {
	LCDTE_EVENT = 0,
	SSP2SYN_EVENT,
	SSP2DI_EVENT,
	SSP2CK_EVENT,
	SSP1SYN_EVENT,
	SSP1DI_EVENT,
	SSP1CK_EVENT,
	SSP0SYN_EVENT,
	SSP0DI_EVENT,
	SSP0CK_EVENT,
	DIGCLKREQ_EVENT,
	ANA_SYS_REQ_EVENT,
	SYSCLKREQ_EVENT,
	UBRX_EVENT,
	UBCTSN_EVENT,
	UB2RX_EVENT,
	UB2CTSN_EVENT,
	SIMDET_EVENT,
	SIM2DET_EVENT,
	MMC0D3_EVENT,
	MMC0D1_EVENT,
	MMC1D3_EVENT,
	MMC1D1_EVENT,
	SDDAT3_EVENT,
	SDDAT1_EVENT,
	SLB1CLK_EVENT,
	SLB1DAT_EVENT,
	SWCLKTCK_EVENT,
	SWDIOTMS_EVENT,
	KEY_R0_EVENT,
	KEY_R1_EVENT,
	KEY_R2_EVENT,
	KEY_R3_EVENT,
	KEY_R4_EVENT,
	KEY_R5_EVENT,
	KEY_R6_EVENT,
	KEY_R7_EVENT,
	DUMMY_EVENT1,	/* Missing event */
	DUMMY_EVENT2,	/* Missing event */
	MISC_WKP_EVENT,
	BATRM_EVENT,
	USBDP_EVENT,
	USBDN_EVENT,
	RXD_EVENT,
	GPIO29_A_EVENT,
	GPIO32_A_EVENT,
	GPIO33_A_EVENT,
	GPIO43_A_EVENT,
	GPIO44_A_EVENT,
	GPIO45_A_EVENT,
	GPIO46_A_EVENT,
	GPIO47_A_EVENT,
	GPIO48_A_EVENT,
	GPIO71_A_EVENT,
	GPIO72_A_EVENT,
	GPIO73_A_EVENT,
	GPIO74_A_EVENT,
	GPIO95_A_EVENT,
	GPIO96_A_EVENT,
	GPIO99_A_EVENT,
	GPIO100_A_EVENT,
	GPIO111_A_EVENT,
	GPIO18_A_EVENT,
	GPIO19_A_EVENT,
	GPIO20_A_EVENT,
	GPIO89_A_EVENT,
	GPIO90_A_EVENT,
	GPIO91_A_EVENT,
	GPIO92_A_EVENT,
	GPIO93_A_EVENT,
	GPIO18_B_EVENT,
	GPIO19_B_EVENT,
	GPIO20_B_EVENT,
	GPIO89_B_EVENT,
	GPIO90_B_EVENT,
	GPIO91_B_EVENT,
	GPIO92_B_EVENT,
	GPIO93_B_EVENT,
	GPIO29_B_EVENT,
	GPIO32_B_EVENT,
	GPIO33_B_EVENT,
	GPIO43_B_EVENT,
	GPIO44_B_EVENT,
	GPIO45_B_EVENT,
	GPIO46_B_EVENT,
	GPIO47_B_EVENT,
	GPIO48_B_EVENT,
	GPIO71_B_EVENT,
	GPIO72_B_EVENT,
	GPIO73_B_EVENT,
	GPIO74_B_EVENT,
	GPIO95_B_EVENT,
	GPIO96_B_EVENT,
	GPIO99_B_EVENT,
	GPIO100_B_EVENT,
	GPIO111_B_EVENT,
	COMMON_TIMER_0_EVENT,
	COMMON_TIMER_1_EVENT,
	COMMON_TIMER_2_EVENT,
	COMMON_TIMER_3_EVENT,
	COMMON_TIMER_4_EVENT,
	COMMON_INT_TO_AC_EVENT,
	TZCFG_INT_TO_AC_EVENT,
	DMA_REQUEST_EVENT,
	MODEM1_EVENT,
	MODEM2_EVENT,
	SD1DAT1DAT3_EVENT,
	BRIDGE_TO_AC_EVENT,
	BRIDGE_TO_MODEM_EVENT,
	VREQ_NONZERO_PI_MODEM_EVENT,
	DUMMY_EVENT3,	/* Missing event */
	USBOTG_EVENT,
	GPIO_EXP_IRQ_EVENT,
	DBR_IRQ_EVENT,
	ACI_EVENT,
	PHY_RESUME_EVENT,
	MODEMBUS_ACTIVE_EVENT,
	SOFTWARE_0_EVENT,
	SOFTWARE_1_EVENT,
	SOFTWARE_2_EVENT,
	PWRMGR_NUM_EVENTS /* Total - 120 events */
};

#define EVENT_CONDITION_ACTIVE_MASK		0x01
#define EVENT_NEG_EDGE_CONDITION_ENABLE_MASK	0x02
#define EVENT_POS_EDGE_CONDITION_ENABLE_MASK	0x04

#define EVENT_TRIG_NONE 	0
#define EVENT_TRIG_POS_EDGE 	1
#define EVENT_TRIG_NEG_EDGE 	2
#define EVENT_TRIG_BOTH_EDGE	EVENT_TRIG_NEG_EDGE | EVENT_TRIG_POS_EDGE

struct event_table {
	u32 event_id;
	u32 trig_type;
	u32 policy_modem;
	u32 policy_arm;
	u32 policy_arm_sub;
	u32 policy_aon;
	u32 policy_hub;
	u32 policy_mm;
};

/* Sequencer */

/** Sequencer commands **/
/* There are more, this is just the bare minimum needed to get started */

#define SEQ_REG_ADDR 		0x0 /* Set address for data read/write. Also used as a NOOP. */
#define SEQ_WAIT_TIMER 		0x6 /* Waits for the amount of clock cycles(?) provided in the payload */
#define SEQ_END 		0x7 /* Marks the end of a sequence */
#define SEQ_SET_PC_PINS 	0x8 /* Value determines mask used in set request, see PC masks */
#define SEQ_JUMP_VOLTAGE 	0xE /* Jump based on voltage request */

/** Values for set1/set2 VO commands for the sequencer **/

#define VLT_ID_OFF		0x0
#define VLT_ID_RETN		0x1
#define VLT_ID_WAKEUP		0x2
#define VLT_ID_A9_SYSPLL_WFI	0x7
#define VLT_ID_A9_ECO		0x8
#define VLT_ID_OTHER_ECO	0x9
#define VLT_ID_A9_NORMAL	0xA
#define VLT_ID_OTHER_NORMAL	0xB
#define VLT_ID_A9_TURBO		0xC
#define VLT_ID_OTHER_TURBO	0xD
#define VLT_ID_A9_SUPER_TURBO	0xE

struct i2c_seq_vo_cmd_data { /* Struct of values/pointers needed by the sequencer: */
	u8 set2_val;
	u8 set2_ptr;
	u8 set1_val;
	u8 set1_ptr;
	u8 zerov_ptr;
	u8 other_ptr;
};

struct i2c_seq_cmd {
	u8 cmd;		/* 4-bit command */
	u8 cmd_data;	/* 8-bit command data */
};

/* TODO: rename these or rewrite so we don't conflict with downstream */
enum pc_pin {
	PC0,
	PC1,
	PC2,
	PC3
};

enum v_set {
	VOLT0,
	VOLT1,
	VOLT2,
	VOLT_MAX
};

#define SET_PC_PIN_CMD_PC0_PIN_VALUE_MASK	(0x01)
#define SET_PC_PIN_CMD_PC1_PIN_VALUE_MASK	(0x02)
#define SET_PC_PIN_CMD_PC2_PIN_VALUE_MASK	(0x04)
#define SET_PC_PIN_CMD_PC3_PIN_VALUE_MASK	(0x08)
#define SET_PC_PIN_CMD_PC0_PIN_OVERRIDE_MASK	(0x10)
#define SET_PC_PIN_CMD_PC1_PIN_OVERRIDE_MASK	(0x20)
#define SET_PC_PIN_CMD_PC2_PIN_OVERRIDE_MASK	(0x40)
#define SET_PC_PIN_CMD_PC3_PIN_OVERRIDE_MASK	(0x80)

#define SET_PC_PIN_CMD(pc_pin)			\
	(SET_PC_PIN_CMD_##pc_pin##_PIN_VALUE_MASK|\
	 SET_PC_PIN_CMD_##pc_pin##_PIN_OVERRIDE_MASK)

#define CLEAR_PC_PIN_CMD(pc_pin)		\
	SET_PC_PIN_CMD_##pc_pin##_PIN_OVERRIDE_MASK

/* Rampup time for CSR/MSR from off state to Vout */
#define SR_VLT_SOFT_START_DELAY	200

/* I2C commands, as written to the raw registers
 * See kona_pwrmgr_seq_write_cmds for documentation */
#define I2C_CMD0_DATA_SHIFT	0
#define I2C_CMD0_DATA_MASK	0x000000FF
#define I2C_CMD0_SHIFT		8
#define I2C_CMD0_MASK		0x00000F00
#define I2C_CMD1_DATA_SHIFT	12
#define I2C_CMD1_DATA_MASK	0x000FF000
#define I2C_CMD1_SHIFT		20
#define I2C_CMD1_MASK		0x00F00000

#define I2C_COMMAND_WORD(cmd1, cmd1_data, cmd0, cmd0_data) \
			(((((u32)(cmd0)) << I2C_CMD0_SHIFT) & I2C_CMD0_MASK) |\
				((((u32)(cmd0_data)) << I2C_CMD0_DATA_SHIFT) & I2C_CMD0_DATA_MASK) |\
				((((u32)(cmd1)) << I2C_CMD1_SHIFT) & I2C_CMD1_MASK) |\
				((((u32)(cmd1_data))  << I2C_CMD1_DATA_SHIFT) & I2C_CMD1_DATA_MASK))
/* ENDTODO */

/* VO settings, as written to the raw registers
 * See kona_pwrmgr_seq_write_vo_ptr_data for documentation */
#define CMDPTR_SET2_VAL_SHIFT	28
#define CMDPTR_SET2_VAL_MASK	0xF0000000
#define CMDPTR_SET2_PTR_SHIFT	22
#define CMDPTR_SET2_PTR_MASK	0x0FC00000
#define CMDPTR_SET1_VAL_SHIFT	18
#define CMDPTR_SET1_VAL_MASK	0x003C0000
#define CMDPTR_SET1_PTR_SHIFT	12
#define CMDPTR_SET1_PTR_MASK	0x0003F000
#define CMDPTR_ZEROV_PTR_SHIFT	6
#define CMDPTR_ZEROV_PTR_MASK	0x00000FC0
#define CMDPTR_OTHER_PTR_SHIFT	0
#define CMDPTR_OTHER_PTR_MASK	0x0000003F

#define VO_PTR_WORD(set2_val, set2_ptr, set1_val, set1_ptr, \
	zerov_ptr, other_ptr) \
		(((u32)(set2_val) << CMDPTR_SET2_VAL_SHIFT) & CMDPTR_SET2_VAL_MASK) |\
		(((u32)(set2_ptr) << CMDPTR_SET2_PTR_SHIFT) & CMDPTR_SET2_PTR_MASK) |\
		(((u32)(set1_val) << CMDPTR_SET1_VAL_SHIFT) & CMDPTR_SET1_VAL_MASK) |\
		(((u32)(set1_ptr) << CMDPTR_SET1_PTR_SHIFT) & CMDPTR_SET1_PTR_MASK) |\
		(((u32)(zerov_ptr) << CMDPTR_ZEROV_PTR_SHIFT) & CMDPTR_ZEROV_PTR_MASK) |\
		(((u32)(other_ptr) << CMDPTR_OTHER_PTR_SHIFT) & CMDPTR_OTHER_PTR_MASK)

#define CMDPTR_ADDL_MASK		0x00000040
#define CMDPTR_ADDL_SHIFT		6
#define CMDPTR_SET2_PTR_ADDL_MASK	0x00001000
#define CMDPTR_SET2_PTR_ADDL_SHIFT	12
#define CMDPTR_SET1_PTR_ADDL_MASK	0x00000100
#define CMDPTR_SET1_PTR_ADDL_SHIFT	8
#define CMDPTR_ZEROV_PTR_ADDL_MASK	0x00000010
#define CMDPTR_ZEROV_PTR_ADDL_SHIFT	4
#define CMDPTR_OTHER_PTR_ADDL_MASK	0x00000001
#define CMDPTR_OTHER_PTR_ADDL_SHIFT	0

#define VO_ADDL_PTR_WORD(set2_ptr, set1_ptr, zerov_ptr, other_ptr) \
	(((((u32)(set2_ptr) & CMDPTR_ADDL_MASK) >> CMDPTR_ADDL_SHIFT) \
		<< CMDPTR_SET2_PTR_ADDL_SHIFT) & CMDPTR_SET2_PTR_ADDL_MASK) |\
	(((((u32)(set1_ptr) & CMDPTR_ADDL_MASK) >> CMDPTR_ADDL_SHIFT) \
		<< CMDPTR_SET1_PTR_ADDL_SHIFT) & CMDPTR_SET1_PTR_ADDL_MASK) |\
	(((((u32)(zerov_ptr) & CMDPTR_ADDL_MASK) >> CMDPTR_ADDL_SHIFT) \
		<< CMDPTR_ZEROV_PTR_ADDL_SHIFT) & CMDPTR_ZEROV_PTR_ADDL_MASK) |\
	(((((u32)(other_ptr) & CMDPTR_ADDL_MASK) >> CMDPTR_ADDL_SHIFT) \
		<< CMDPTR_OTHER_PTR_ADDL_SHIFT) & CMDPTR_OTHER_PTR_ADDL_MASK)

static struct i2c_seq_cmd i2c_dummy_seq_cmds[] = {
	{SEQ_REG_ADDR, 0},
	{SEQ_JUMP_VOLTAGE, 0},
	{SEQ_REG_ADDR, 0},
	{SEQ_REG_ADDR, 0},
	{SEQ_REG_ADDR, 0},
	{SEQ_REG_ADDR, 0},
	{SEQ_REG_ADDR, 0},
	{SEQ_REG_ADDR, 0},
	{SEQ_REG_ADDR, 0},
	{SEQ_REG_ADDR, 0},
	{SEQ_REG_ADDR, 0},
	{SEQ_REG_ADDR, 0},
	{SEQ_REG_ADDR, 0},
	{SEQ_REG_ADDR, 0},
	{SEQ_REG_ADDR, 0},
	{SEQ_REG_ADDR, 0},
	{SEQ_REG_ADDR, 0},
	{SEQ_END, 0},
	{SEQ_SET_PC_PINS, SET_PC_PIN_CMD(PC1)},
	{SEQ_END, 0},
	{SEQ_REG_ADDR, 0},
	{SEQ_SET_PC_PINS, CLEAR_PC_PIN_CMD(PC1) |\
		CLEAR_PC_PIN_CMD(PC0)},
	{SEQ_END, 0},
	{SEQ_REG_ADDR, 0},
	{SEQ_SET_PC_PINS, SET_PC_PIN_CMD(PC2)},
	{SEQ_WAIT_TIMER, SR_VLT_SOFT_START_DELAY},
	{SEQ_END, 0},
	{SEQ_REG_ADDR, 0},
	{SEQ_SET_PC_PINS, CLEAR_PC_PIN_CMD(PC2) |\
		CLEAR_PC_PIN_CMD(PC0)},
	{SEQ_END, 0},
	{SEQ_REG_ADDR, 0},
	{SEQ_REG_ADDR, 0},
	{SEQ_REG_ADDR, 0},
	{SEQ_REG_ADDR, 0},
	{SEQ_REG_ADDR, 0},
	{SEQ_REG_ADDR, 0},
	{SEQ_REG_ADDR, 0},
	{SEQ_REG_ADDR, 0},
	{SEQ_REG_ADDR, 0},
	{SEQ_REG_ADDR, 0},
	{SEQ_REG_ADDR, 0},
	{SEQ_REG_ADDR, 0},
	{SEQ_REG_ADDR, 0},
	{SEQ_REG_ADDR, 0},
	{SEQ_REG_ADDR, 0},
	{SEQ_REG_ADDR, 0},
	{SEQ_REG_ADDR, 0},
	{SEQ_REG_ADDR, 0},
	{SEQ_REG_ADDR, 0},
	{SEQ_REG_ADDR, 0},
	{SEQ_REG_ADDR, 0},
	{SEQ_REG_ADDR, 0},
	{SEQ_REG_ADDR, 0},
	{SEQ_REG_ADDR, 0},
	{SEQ_REG_ADDR, 0},
	{SEQ_REG_ADDR, 0},
	{SEQ_REG_ADDR, 0},
	{SEQ_REG_ADDR, 0},
	{SEQ_REG_ADDR, 0},
	{SEQ_REG_ADDR, 0},
	{SEQ_REG_ADDR, 0},
	{SEQ_REG_ADDR, 0},
	{SEQ_REG_ADDR, 0},
	{SEQ_REG_ADDR, 0},
	{SEQ_END, 0}, /* TODO: this is the 64th element...? */
};

int i2c_dummy_seq_num_cmds = 64;

/* Platform-specific data */

#include "bcm21664_pwrmgr.h"
#include "bcm23550_pwrmgr.h"

struct kona_pwrmgr_info {
	unsigned int revision;

	struct i2c_seq_vo_cmd_data *dummy_seq_v0_data;
	struct i2c_seq_vo_cmd_data *dummy_seq_v1_data;

	struct event_table *event_table;
	int event_table_length;

	struct kona_pi_info *pi_info;
};

struct kona_pwrmgr_info bcm21664_pwrmgr_info = {
	.revision = PWRMGR_REV02,

	.dummy_seq_v0_data = &bcm21664_dummy_seq_v0_data,
	.dummy_seq_v1_data = &bcm21664_dummy_seq_v1_data,

	.event_table = bcm21664_event_table,
	.event_table_length = 27,

	.pi_info = bcm21664_pi_info,
};

struct kona_pwrmgr_info bcm23550_pwrmgr_info = {
	.revision = PWRMGR_REV02,

	.dummy_seq_v0_data = &bcm21664_dummy_seq_v0_data,
	.dummy_seq_v1_data = &bcm21664_dummy_seq_v1_data,

	.event_table = bcm23550_event_table,
	.event_table_length = 29,

	.pi_info = bcm21664_pi_info,
};

#endif /* __BCM_KONA_PWRMGR_H__ */
