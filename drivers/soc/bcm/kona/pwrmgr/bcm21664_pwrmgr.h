// SPDX-License-Identifier: GPL-2.0-only

#ifndef __BCM_KONA_BCM21664_PWRMGR_H__
#define __BCM_KONA_BCM21664_PWRMGR_H__

static struct event_table bcm21664_event_table[] = {
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
		.policy_arm_sub	= PI_POLICY_ECO,
		.policy_aon	= PI_POLICY_ECO,
		.policy_hub	= PI_POLICY_ECO,
		.policy_mm	= PI_POLICY_OFF,
	},

	{
		.event_id	= VREQ_NONZERO_PI_MODEM_EVENT,
		.trig_type	= EVENT_TRIG_POS_EDGE,
		.policy_modem	= PI_POLICY_DFS,
		.policy_arm	= PI_POLICY_OFF,
		.policy_arm_sub	= PI_POLICY_RET,
		.policy_aon	= PI_POLICY_DFS,
		.policy_hub	= PI_POLICY_DFS,
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

static const struct pi_state bcm21664_pi_states_mm[] = {
	{
		.state_id = PI_STATE_ACTIVE,
		.policy = PI_POLICY_DFS,
		.wakeup_latency = 0,
	},

	{
		.state_id = PI_STATE_RETENTION,
		.policy = PI_POLICY_RET,
		.wakeup_latency = 10,
	},

	{
		.state_id = PI_STATE_SHUTDOWN,
		.policy = PI_POLICY_OFF,
		.wakeup_latency = 100,
	},
};

static const struct kona_pi_info bcm21664_pi_info_mm = {
	.name = "mm",
	.states = bcm21664_pi_states_mm,

	.policy_reg_offset = 0x1000,
	.ac_shift = 12,
	.atl_shift = 11,
	.pm_policy_shift = 8,

	.wakeup_override_shift = 2,
};

static const struct pi_state bcm21664_pi_states_hub[] = {
	{
		.state_id = PI_STATE_ACTIVE,
		.policy = PI_POLICY_DFS,
		.wakeup_latency = 0,
	},

	{
		.state_id = PI_STATE_RETENTION,
		.policy = PI_POLICY_RET,
		.wakeup_latency = 100,
	},
};

static const struct kona_pi_info bcm21664_pi_info_hub_switchable = {
	.name = "hub_switchable",
	.states = bcm21664_pi_states_hub,

	.policy_reg_offset = 0x1800,
	.ac_shift = 4,
	.atl_shift = 3,
	.pm_policy_shift = 0,

	.wakeup_override_shift = 4,
};

static const struct kona_pi_info bcm21664_pi_info_hub_aon = {
	.name = "hub_aon",
	.states = bcm21664_pi_states_hub,

	.policy_reg_offset = 0x1800,
	.ac_shift = 12,
	.atl_shift = 11,
	.pm_policy_shift = 8,

	.wakeup_override_shift = 5,
};

static const struct pi_state bcm21664_pi_states_arm_core[] = {
	{
		.state_id = ARM_CORE_STATE_ACTIVE,
		.policy = PI_POLICY_DFS,
		.wakeup_latency = 0,
	},

	{
		.state_id = ARM_CORE_STATE_SUSPEND,
		.policy = PI_POLICY_DFS,
		.wakeup_latency = 10,
	},

	{
		.state_id = ARM_CORE_STATE_DORMANT,
		.policy = PI_POLICY_RET,
		.wakeup_latency = 100,
	},
};

static const struct kona_pi_info bcm21664_pi_info_arm_core = {
	.name = "arm_core",
	.states = bcm21664_pi_states_arm_core,

	.policy_reg_offset = 0x0800,
	.ac_shift = 4,
	.atl_shift = 3,
	.pm_policy_shift = 0,

	.wakeup_override_shift = 0,
};

static const struct pi_state bcm21664_pi_states_arm_subsystem[] = {
	{
		.state_id = PI_STATE_ACTIVE,
		.policy = PI_POLICY_DFS,
		.wakeup_latency = 0,
	},

	{
		.state_id = PI_STATE_RETENTION,
		.policy = PI_POLICY_RET,
		.wakeup_latency = 100,
	},
};

static const struct kona_pi_info bcm21664_pi_info_arm_subsystem = {
	.name = "arm_subsystem",
	.states = bcm21664_pi_states_arm_subsystem,

	.policy_reg_offset = 0x2000,
	.ac_shift = 4,
	.atl_shift = 3,
	.pm_policy_shift = 0,

	.wakeup_override_shift = 3,
};

static const struct pi_state bcm21664_pi_states_modem[] = {
	{
		.state_id = PI_STATE_ACTIVE,
		.policy = PI_POLICY_DFS,
		.wakeup_latency = 0,
	},
};

static const struct kona_pi_info bcm21664_pi_info_modem = {
	.name = "modem",
	.states = bcm21664_pi_states_modem,

	.policy_reg_offset = 0x0800,
	.ac_shift = 9,
	.atl_shift = 8,
	.pm_policy_shift = 5,

	.wakeup_override_shift = 6,
};

static struct kona_pi_info bcm21664_pi_info[] = {
	[BCMKONA_POWER_DOMAIN_MM] = bcm21664_pi_info_mm,
	[BCMKONA_POWER_DOMAIN_HUB_SWITCHABLE] = bcm21664_pi_info_hub_switchable,
	[BCMKONA_POWER_DOMAIN_HUB_AON] = bcm21664_pi_info_hub_aon,
	[BCMKONA_POWER_DOMAIN_ARM_CORE] = bcm21664_pi_info_arm_core,
	[BCMKONA_POWER_DOMAIN_ARM_SUBSYSTEM] = bcm21664_pi_info_arm_subsystem,
	[BCMKONA_POWER_DOMAIN_MODEM] = bcm21664_pi_info_modem,
};

static struct i2c_seq_vo_cmd_data bcm21664_dummy_seq_v0_data = {
	.set2_val = VLT_ID_WAKEUP,
	.set2_ptr = 18,
	.set1_val = VLT_ID_RETN,
	.set1_ptr = 21,
	.zerov_ptr = 21,
	.other_ptr = 2,
};

static struct i2c_seq_vo_cmd_data bcm21664_dummy_seq_v1_data = {
	.set2_val = VLT_ID_WAKEUP,
	.set2_ptr = 24,
	.set1_val = VLT_ID_RETN,
	.set1_ptr = 28,
	.zerov_ptr = 28,
	.other_ptr = 2,
};

#endif /* __BCM_KONA_BCM21664_PWRMGR_H__ */
