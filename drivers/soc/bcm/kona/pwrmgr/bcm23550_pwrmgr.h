// SPDX-License-Identifier: GPL-2.0-only

#ifndef __BCM_KONA_BCM23550_PWRMGR_H__
#define __BCM_KONA_BCM23550_PWRMGR_H__

struct event_table bcm23550_event_table[] = {
	{
		.event_id	= SOFTWARE_0_EVENT,
		.trig_type	= EVENT_TRIG_BOTH_EDGE,
		.policy_modem	= PI_POLICY_RET,
		.policy_arm	= PI_POLICY_DFS,
		.policy_arm_sub	= PI_POLICY_DFS,
		.policy_aon	= PI_POLICY_DFS,
		.policy_hub	= PI_POLICY_DFS,
		.policy_mm	= PI_POLICY_OFF,
	},

	{
		.event_id	= SOFTWARE_1_EVENT,
		.trig_type	= EVENT_TRIG_NONE,
		.policy_modem	= PI_POLICY_RET,
		.policy_arm	= PI_POLICY_OFF,
		.policy_arm_sub	= PI_POLICY_RET,
		.policy_aon	= PI_POLICY_RET,
		.policy_hub	= PI_POLICY_RET,
		.policy_mm	= PI_POLICY_OFF,
	},

	{
		.event_id	= SOFTWARE_2_EVENT,
		.trig_type	= EVENT_TRIG_BOTH_EDGE,
		.policy_modem	= PI_POLICY_RET,
		.policy_arm	= PI_POLICY_DFS,
		.policy_arm_sub	= PI_POLICY_6,
		.policy_aon	= PI_POLICY_6,
		.policy_hub	= PI_POLICY_6,
		.policy_mm	= PI_POLICY_OFF,
	},

	{
		.event_id	= VREQ_NONZERO_PI_MODEM_EVENT,
		.trig_type	= EVENT_TRIG_POS_EDGE,
		.policy_modem	= PI_POLICY_DFS,
		.policy_arm	= PI_POLICY_OFF,
		.policy_arm_sub	= PI_POLICY_RET,
		.policy_aon	= PI_POLICY_6,
		.policy_hub	= PI_POLICY_6,
		.policy_mm	= PI_POLICY_OFF,
	},

	{
		.event_id	= COMMON_INT_TO_AC_EVENT,
		.trig_type	= EVENT_TRIG_POS_EDGE,
		.policy_modem	= PI_POLICY_RET,
		.policy_arm	= PI_POLICY_DFS,
		.policy_arm_sub	= PI_POLICY_DFS,
		.policy_aon	= PI_POLICY_DFS,
		.policy_hub	= PI_POLICY_DFS,
		.policy_mm	= PI_POLICY_OFF,
	},

	{
		.event_id	= COMMON_TIMER_0_EVENT,
		.trig_type	= EVENT_TRIG_POS_EDGE,
		.policy_modem	= PI_POLICY_RET,
		.policy_arm	= PI_POLICY_DFS,
		.policy_arm_sub	= PI_POLICY_DFS,
		.policy_aon	= PI_POLICY_DFS,
		.policy_hub	= PI_POLICY_DFS,
		.policy_mm	= PI_POLICY_OFF,
	},

	{
		.event_id	= COMMON_TIMER_1_EVENT,
		.trig_type	= EVENT_TRIG_POS_EDGE,
		.policy_modem	= PI_POLICY_RET,
		.policy_arm	= PI_POLICY_DFS,
		.policy_arm_sub	= PI_POLICY_DFS,
		.policy_aon	= PI_POLICY_DFS,
		.policy_hub	= PI_POLICY_DFS,
		.policy_mm	= PI_POLICY_OFF,
	},

	{
		.event_id	= COMMON_TIMER_2_EVENT,
		.trig_type	= EVENT_TRIG_POS_EDGE,
		.policy_modem	= PI_POLICY_RET,
		.policy_arm	= PI_POLICY_DFS,
		.policy_arm_sub	= PI_POLICY_DFS,
		.policy_aon	= PI_POLICY_DFS,
		.policy_hub	= PI_POLICY_DFS,
		.policy_mm	= PI_POLICY_OFF,
	},

	{
		.event_id	= COMMON_TIMER_3_EVENT,
		.trig_type	= EVENT_TRIG_POS_EDGE,
		.policy_modem	= PI_POLICY_RET,
		.policy_arm	= PI_POLICY_DFS,
		.policy_arm_sub	= PI_POLICY_DFS,
		.policy_aon	= PI_POLICY_DFS,
		.policy_hub	= PI_POLICY_DFS,
		.policy_mm	= PI_POLICY_OFF,
	},

	{
		.event_id	= COMMON_TIMER_4_EVENT,
		.trig_type	= EVENT_TRIG_POS_EDGE,
		.policy_modem	= PI_POLICY_RET,
		.policy_arm	= PI_POLICY_DFS,
		.policy_arm_sub	= PI_POLICY_DFS,
		.policy_aon	= PI_POLICY_DFS,
		.policy_hub	= PI_POLICY_DFS,
		.policy_mm	= PI_POLICY_OFF,
	},

	{
		.event_id	= UBRX_EVENT,
		.trig_type	= EVENT_TRIG_NEG_EDGE,
		.policy_modem	= PI_POLICY_RET,
		.policy_arm	= PI_POLICY_DFS,
		.policy_arm_sub	= PI_POLICY_DFS,
		.policy_aon	= PI_POLICY_DFS,
		.policy_hub	= PI_POLICY_DFS,
		.policy_mm	= PI_POLICY_OFF,
	},

	{
		.event_id	= UB2RX_EVENT,
		.trig_type	= EVENT_TRIG_NEG_EDGE,
		.policy_modem	= PI_POLICY_RET,
		.policy_arm	= PI_POLICY_DFS,
		.policy_arm_sub	= PI_POLICY_DFS,
		.policy_aon	= PI_POLICY_DFS,
		.policy_hub	= PI_POLICY_DFS,
		.policy_mm	= PI_POLICY_OFF,
	},

	{
		.event_id	= SIMDET_EVENT,
		.trig_type	= EVENT_TRIG_BOTH_EDGE,
		.policy_modem	= PI_POLICY_RET,
		.policy_arm	= PI_POLICY_DFS,
		.policy_arm_sub	= PI_POLICY_DFS,
		.policy_aon	= PI_POLICY_DFS,
		.policy_hub	= PI_POLICY_DFS,
		.policy_mm	= PI_POLICY_OFF,
	},

	{
		.event_id	= SIM2DET_EVENT,
		.trig_type	= EVENT_TRIG_BOTH_EDGE,
		.policy_modem	= PI_POLICY_RET,
		.policy_arm	= PI_POLICY_DFS,
		.policy_arm_sub	= PI_POLICY_DFS,
		.policy_aon	= PI_POLICY_DFS,
		.policy_hub	= PI_POLICY_DFS,
		.policy_mm	= PI_POLICY_OFF,
	},

	{
		.event_id	= KEY_R0_EVENT,
		.trig_type	= EVENT_TRIG_BOTH_EDGE,
		.policy_modem	= PI_POLICY_RET,
		.policy_arm	= PI_POLICY_DFS,
		.policy_arm_sub	= PI_POLICY_DFS,
		.policy_aon	= PI_POLICY_DFS,
		.policy_hub	= PI_POLICY_DFS,
		.policy_mm	= PI_POLICY_OFF,
	},

	{
		.event_id	= KEY_R1_EVENT,
		.trig_type	= EVENT_TRIG_BOTH_EDGE,
		.policy_modem	= PI_POLICY_RET,
		.policy_arm	= PI_POLICY_DFS,
		.policy_arm_sub	= PI_POLICY_DFS,
		.policy_aon	= PI_POLICY_DFS,
		.policy_hub	= PI_POLICY_DFS,
		.policy_mm	= PI_POLICY_OFF,
	},

	{
		.event_id	= KEY_R2_EVENT,
		.trig_type	= EVENT_TRIG_BOTH_EDGE,
		.policy_modem	= PI_POLICY_RET,
		.policy_arm	= PI_POLICY_DFS,
		.policy_arm_sub	= PI_POLICY_DFS,
		.policy_aon	= PI_POLICY_DFS,
		.policy_hub	= PI_POLICY_DFS,
		.policy_mm	= PI_POLICY_OFF,
	},

	{
		.event_id	= KEY_R3_EVENT,
		.trig_type	= EVENT_TRIG_BOTH_EDGE,
		.policy_modem	= PI_POLICY_RET,
		.policy_arm	= PI_POLICY_DFS,
		.policy_arm_sub	= PI_POLICY_DFS,
		.policy_aon	= PI_POLICY_DFS,
		.policy_hub	= PI_POLICY_DFS,
		.policy_mm	= PI_POLICY_OFF,
	},

	{
		.event_id	= KEY_R4_EVENT,
		.trig_type	= EVENT_TRIG_BOTH_EDGE,
		.policy_modem	= PI_POLICY_RET,
		.policy_arm	= PI_POLICY_DFS,
		.policy_arm_sub	= PI_POLICY_DFS,
		.policy_aon	= PI_POLICY_DFS,
		.policy_hub	= PI_POLICY_DFS,
		.policy_mm	= PI_POLICY_OFF,
	},

	{
		.event_id	= KEY_R5_EVENT,
		.trig_type	= EVENT_TRIG_BOTH_EDGE,
		.policy_modem	= PI_POLICY_RET,
		.policy_arm	= PI_POLICY_DFS,
		.policy_arm_sub	= PI_POLICY_DFS,
		.policy_aon	= PI_POLICY_DFS,
		.policy_hub	= PI_POLICY_DFS,
		.policy_mm	= PI_POLICY_OFF,
	},

	{
		.event_id	= KEY_R6_EVENT,
		.trig_type	= EVENT_TRIG_BOTH_EDGE,
		.policy_modem	= PI_POLICY_RET,
		.policy_arm	= PI_POLICY_DFS,
		.policy_arm_sub	= PI_POLICY_DFS,
		.policy_aon	= PI_POLICY_DFS,
		.policy_hub	= PI_POLICY_DFS,
		.policy_mm	= PI_POLICY_OFF,
	},

	{
		.event_id	= KEY_R7_EVENT,
		.trig_type	= EVENT_TRIG_BOTH_EDGE,
		.policy_modem	= PI_POLICY_RET,
		.policy_arm	= PI_POLICY_DFS,
		.policy_arm_sub	= PI_POLICY_DFS,
		.policy_aon	= PI_POLICY_DFS,
		.policy_hub	= PI_POLICY_DFS,
		.policy_mm	= PI_POLICY_OFF,
	},

	{
		.event_id	= GPIO29_A_EVENT,
		.trig_type	= EVENT_TRIG_BOTH_EDGE,
		.policy_modem	= PI_POLICY_RET,
		.policy_arm	= PI_POLICY_DFS,
		.policy_arm_sub	= PI_POLICY_DFS,
		.policy_aon	= PI_POLICY_DFS,
		.policy_hub	= PI_POLICY_DFS,
		.policy_mm	= PI_POLICY_OFF,
	},

	{
		.event_id	= GPIO71_A_EVENT,
		.trig_type	= EVENT_TRIG_BOTH_EDGE,
		.policy_modem	= PI_POLICY_RET,
		.policy_arm	= PI_POLICY_DFS,
		.policy_arm_sub	= PI_POLICY_DFS,
		.policy_aon	= PI_POLICY_DFS,
		.policy_hub	= PI_POLICY_DFS,
		.policy_mm	= PI_POLICY_OFF,
	},

	{
		.event_id	= GPIO74_A_EVENT,
		.trig_type	= EVENT_TRIG_BOTH_EDGE,
		.policy_modem	= PI_POLICY_RET,
		.policy_arm	= PI_POLICY_DFS,
		.policy_arm_sub	= PI_POLICY_DFS,
		.policy_aon	= PI_POLICY_DFS,
		.policy_hub	= PI_POLICY_DFS,
		.policy_mm	= PI_POLICY_OFF,
	},
		{
		.event_id	= GPIO111_A_EVENT,
		.trig_type	= EVENT_TRIG_BOTH_EDGE,
		.policy_modem	= PI_POLICY_RET,
		.policy_arm	= PI_POLICY_DFS,
		.policy_arm_sub	= PI_POLICY_DFS,
		.policy_aon	= PI_POLICY_DFS,
		.policy_hub	= PI_POLICY_DFS,
		.policy_mm	= PI_POLICY_OFF,
	},

	{
		.event_id	= MMC1D1_EVENT,
		.trig_type	= EVENT_TRIG_NEG_EDGE,
		.policy_modem	= PI_POLICY_RET,
		.policy_arm	= PI_POLICY_DFS,
		.policy_arm_sub	= PI_POLICY_DFS,
		.policy_aon	= PI_POLICY_DFS,
		.policy_hub	= PI_POLICY_DFS,
		.policy_mm	= PI_POLICY_OFF,
	},

	{
		.event_id	= DBR_IRQ_EVENT,
		.trig_type	= EVENT_TRIG_POS_EDGE,
		.policy_modem	= PI_POLICY_RET,
		.policy_arm	= PI_POLICY_DFS,
		.policy_arm_sub	= PI_POLICY_DFS,
		.policy_aon	= PI_POLICY_DFS,
		.policy_hub	= PI_POLICY_DFS,
		.policy_mm	= PI_POLICY_OFF,
	},

	{
		.event_id	= ACI_EVENT,
		.trig_type	= EVENT_TRIG_POS_EDGE,
		.policy_modem	= PI_POLICY_RET,
		.policy_arm	= PI_POLICY_DFS,
		.policy_arm_sub	= PI_POLICY_DFS,
		.policy_aon	= PI_POLICY_DFS,
		.policy_hub	= PI_POLICY_DFS,
		.policy_mm	= PI_POLICY_OFF,
	},
};

#endif /* __BCM_KONA_BCM23550_PWRMGR_H__ */
