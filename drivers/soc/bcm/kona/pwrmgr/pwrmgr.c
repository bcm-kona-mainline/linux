// SPDX-License-Identifier: GPL-2.0-only
/*
 * Broadcom Kona Power Manager/Power Islands driver
 * Copyright (c) 2022 Artur Weber <aweber@dithernet.org>
 *
 * NOTE: this driver is *very* early work-in-progress. PI setup
 * is not 100% implemented yet. This driver only does the basic
 * initialization.
 */

#include <dt-bindings/soc/brcm,kona-pi.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/err.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/pm_domain.h>
#include <linux/slab.h>

#include "pwrmgr.h"

struct kona_pi {
	int domain_id;
	struct generic_pm_domain pm_domain;
	struct kona_pwrmgr *parent;
	struct kona_pi_info info;
};

struct kona_pwrmgr {
	struct device *dev;
	void __iomem *base;

	const struct kona_pwrmgr_info *info;

	struct genpd_onecell_data kona_pi_onecell;
	struct kona_pi domains[BCMKONA_POWER_DOMAIN_COUNT];
};

static struct kona_pwrmgr *pwrmgr = &(struct kona_pwrmgr) {
	.base = NULL,
};

static u32 kona_pwrmgr_readl(struct kona_pwrmgr *pwrmgr, u32 offset)
{
	return readl(pwrmgr->base + offset);
}

static void kona_pwrmgr_writel(struct kona_pwrmgr *pwrmgr, u32 value, u32 offset)
{
	writel(value, pwrmgr->base + offset);
}

/* Power Islands */

static void kona_pwrmgr_pi_set_wakeup_override(int pi_id, bool clear)
{
	struct kona_pi_info pi_info = pwrmgr->info->pi_info[pi_id];
	u32 val;

	val = kona_pwrmgr_readl(pwrmgr, PWRMGR_PI_DEFAULT_POWER_STATE_OFFSET);

	if (clear)
		val &= ~(1 << pi_info.wakeup_override_shift);
	else
		val |= (1 << pi_info.wakeup_override_shift);

	kona_pwrmgr_writel(pwrmgr, val, PWRMGR_PI_DEFAULT_POWER_STATE_OFFSET);
}

/* Events */

static void kona_pwrmgr_event_set_active(int event_id, bool active)
{
	u32 val, offset;

	BUG_ON(event_id >= PWRMGR_NUM_EVENTS);

	offset = EVENT_OFFSET_FOR_ID(event_id);

	/* TODO-IRQSAVE */

	val = kona_pwrmgr_readl(pwrmgr, offset);

	if (active)
		val |= EVENT_CONDITION_ACTIVE_MASK;
	else
		val &= ~EVENT_CONDITION_ACTIVE_MASK;
	kona_pwrmgr_writel(pwrmgr, val, offset);

	/* TODO-IRQREMOVE */
}

static void kona_pwrmgr_clear_events(int event_start, int event_end)
{
	u32 val, offset;
	int i;

	BUG_ON(event_start > event_end);
	BUG_ON(event_start >= PWRMGR_NUM_EVENTS);
	BUG_ON(event_end >= PWRMGR_NUM_EVENTS);

	/* TODO-IRQSAVE */

	for (i = event_start; i <= event_end; i++) {
		if (i == DUMMY_EVENT1 || i == DUMMY_EVENT2 || i == DUMMY_EVENT3)
			continue;

		offset = EVENT_OFFSET_FOR_ID(i);

		/* The event offset is 0, so we don't need to add to anything */
		val = kona_pwrmgr_readl(pwrmgr, offset);
		if (val & EVENT_CONDITION_ACTIVE_MASK) {
			val &= ~EVENT_CONDITION_ACTIVE_MASK;
			kona_pwrmgr_writel(pwrmgr, val, offset);
		}
	}

	/* TODO-IRQREMOVE */
}

static void kona_pwrmgr_event_set_trig_type(int event_id, int event_trig_type)
{
	u32 val, offset;

	BUG_ON(event_id >= PWRMGR_NUM_EVENTS);
	offset = EVENT_OFFSET_FOR_ID(event_id);

	/* TODO-IRQSAVE */

	val = kona_pwrmgr_readl(pwrmgr, offset);

	/* Clear value */
	val &= ~EVENT_NEG_EDGE_CONDITION_ENABLE_MASK;
	val &= ~EVENT_POS_EDGE_CONDITION_ENABLE_MASK;

	if (event_trig_type & EVENT_TRIG_POS_EDGE)
		val |= EVENT_POS_EDGE_CONDITION_ENABLE_MASK;
	if (event_trig_type & EVENT_TRIG_NEG_EDGE)
		val |= EVENT_NEG_EDGE_CONDITION_ENABLE_MASK;

	kona_pwrmgr_writel(pwrmgr, val, offset);

	/* TODO-IRQREMOVE */
}

static void kona_pwrmgr_event_set_pi_policy(int event_id, int pi_id,
				bool ac, bool atl, int policy)
{
	struct kona_pi_info pi_info = pwrmgr->info->pi_info[pi_id];
	u32 policy_offset;
	u32 val;

	BUG_ON(event_id >= PWRMGR_NUM_EVENTS);
	policy_offset = pi_info.policy_reg_offset;

	/* TODO-IRQSAVE */

	val = kona_pwrmgr_readl(pwrmgr, policy_offset);

	if (ac)
		val |= (1 << pi_info.ac_shift);
	else
		val &= ~(1 << pi_info.ac_shift);

	if (atl)
		val |= (1 << pi_info.atl_shift);
	else
		val &= ~(1 << pi_info.atl_shift);

	val &= ~(PI_POLICY_MASK << pi_info.pm_policy_shift);
	val |= (policy & PI_POLICY_MASK) << pi_info.pm_policy_shift;

	kona_pwrmgr_writel(pwrmgr, val, policy_offset);

	/* TODO-IRQREMOVE */
}

/* Pin control */

static void kona_pwrmgr_pc_set_sw_override(int pc_pin, bool enable, int value)
{
	u32 reg_value = 0;

	/* TODO-IRQSAVE */

	reg_value = kona_pwrmgr_readl(pwrmgr, PWRMGR_PC_PIN_OVERRIDE_CTRL_OFFSET);
	if (enable)
		reg_value = reg_value | (PC_SW_VALUE_MASK_FOR(pc_pin) | PC_SW_ENABLE_MASK_FOR(pc_pin));
	else
		reg_value = reg_value & (~PC_SW_ENABLE_MASK_FOR(pc_pin));
	kona_pwrmgr_writel(pwrmgr, reg_value, PWRMGR_PC_PIN_OVERRIDE_CTRL_OFFSET);

	/* TODO-IRQREMOVE */
}

static void kona_pwrmgr_pc_set_clkreq_override(int pc_pin, bool enable, int value)
{
	u32 reg_value = 0;

	/* TODO-IRQSAVE */

	reg_value = kona_pwrmgr_readl(pwrmgr, PWRMGR_PC_PIN_OVERRIDE_CTRL_OFFSET);
	if (enable)
		reg_value = reg_value | (PC_CLKREQ_VALUE_MASK_FOR(pc_pin) | PC_CLKREQ_ENABLE_MASK_FOR(pc_pin));
	else
		reg_value = reg_value & (~PC_CLKREQ_ENABLE_MASK_FOR(pc_pin));
	kona_pwrmgr_writel(pwrmgr, reg_value, PWRMGR_PC_PIN_OVERRIDE_CTRL_OFFSET);

	/* TODO-IRQREMOVE */
}

/* Power Islands/Power domains control */

static int kona_pwrmgr_pi_power_on(struct generic_pm_domain *domain)
{
	struct kona_pi *pi = container_of(domain, struct kona_pi, pm_domain);

	pr_info("kona-pwrmgr: enabling domain %s", pi->info.name);

	kona_pwrmgr_event_set_pi_policy(SOFTWARE_0_EVENT, pi->domain_id, true, false, PI_POLICY_RET);

	return 0;
}

static int kona_pwrmgr_pi_power_off(struct generic_pm_domain *domain)
{
	/* TODO */
	return 0;
}

static void kona_pwrmgr_pi_init(int pi_id)
{
	struct kona_pi *domain = &pwrmgr->domains[pi_id];

	kona_pwrmgr_pi_set_wakeup_override(pi_id, false);

	domain->domain_id = pi_id;
	domain->info = pwrmgr->info->pi_info[pi_id];

	domain->pm_domain.name = domain->info.name;
	domain->pm_domain.power_on = kona_pwrmgr_pi_power_on;
	domain->pm_domain.power_off = kona_pwrmgr_pi_power_off;

	pm_genpd_init(&domain->pm_domain, NULL, true);
}

/* Sequencer */

static void kona_pwrmgr_seq_enable(bool enable)
{
	/* TODO-IRQSAVE */
	u32 value = kona_pwrmgr_readl(pwrmgr, PWRMGR_I2C_ENABLE_OFFSET);

	if (enable)
		value |= 0x01;
	else
		value &= ~0x01;

	kona_pwrmgr_writel(pwrmgr, value, PWRMGR_I2C_ENABLE_OFFSET);
	/* TODO-IRQREMOVE */
}

static void kona_pwrmgr_seq_write_cmds(struct i2c_seq_cmd *i2c_cmds, int num_cmds)
{
	u32 value;
	u8 max_banks;
	u8 bank_no;
	u8 i;

	u32 cmd0, cmd1;
	u32 cmd0_data, cmd1_data;
	u32 cmd_offset, bank_offset;

	if (pwrmgr->info->revision == PWRMGR_REV02)
		max_banks = 1; /* REV2 - 2 banks */
	else
		max_banks = 0; /* REV0 - 1 bank */

	/* TODO-IRQSAVE */

	/*
	 * The commands are packed into 32-bit values which follow the following
	 * structure:
	 *
	 * 00000000 0000 00000000 0000 00000000
	 * (unused) CMD1 CMD1DATA CMD0 CMD0DATA
	 */

	for (i = 0; i < num_cmds / 2; i++) {
		bank_no = SEQ_CMD_OFFSET_TO_BANK(i * 2);
		if (bank_no > max_banks) {
			BUG_ON(bank_no > max_banks);
			break;
		}

		if (bank_no == 0)
			bank_offset = PWRMGR_SEQ_CMD_BANK0_OFFSET;
		else if (bank_no == 1)
			bank_offset = PWRMGR_SEQ_CMD_BANK1_OFFSET;
		cmd_offset = bank_offset + (4 * (i - (bank_no * 32)));

		cmd0 = i2c_cmds[i*2].cmd;
		cmd0_data = i2c_cmds[i*2].cmd_data;

		if ((2*i + 1) < num_cmds) {
			cmd1 = i2c_cmds[i*2].cmd;
			cmd1_data = i2c_cmds[i*2].cmd_data;
		} else {
			cmd1 = 0x0000;
			cmd1_data = 0x0000;
		}

		value = I2C_COMMAND_WORD(cmd1, cmd1_data, cmd0, cmd0_data);
		kona_pwrmgr_writel(pwrmgr, value, cmd_offset);
	}

	/* TODO-IRQRESTORE */
}

static void kona_pwrmgr_seq_write_vo_ptr_data(int v0x,
		struct i2c_seq_vo_cmd_data *v0_ptr_data,
		struct i2c_seq_vo_cmd_data *v1_ptr_data)
{
	u32 offset = PWRMGR_VO0_I2C_CMD_PTR_OFFSET + 4 * v0x;
	u32 offset_addl = PWRMGR_VO0_I2C_CMD_ADDL_PTR_OFFSET + 4 * v0x;
	u32 val, val_addl;

	/* TODO-IRQSAVE */

	/*
	 * VO0 has data for VOLT0, VOLT1, VOLT2. The data is packed into
	 * 32-bit values which follow the following structure:
	 *
	 *   0000     000000     0000     000000     000000    000000
	 * set2_val  set2_ptr  set1_val  set1_ptr  zerov_ptr  other_ptr
	 */
	val = VO_PTR_WORD(v0_ptr_data->set2_val, v0_ptr_data->set2_ptr,
		v0_ptr_data->set1_val, v0_ptr_data->set1_ptr,
		v0_ptr_data->zerov_ptr, v0_ptr_data->other_ptr);
	kona_pwrmgr_writel(pwrmgr, val, offset);

	/*
	 * REV02 power managers have additional registers for voltage control.
	 * The data is packed into 32-bit values which follow the following
	 * structure:
	 *
	 *   0000     000000     0000     000000     000000    000000
	 * set2_val  set2_ptr  set1_val  set1_ptr  zerov_ptr  other_ptr
	 */
	if (pwrmgr->info->revision == PWRMGR_REV02) {
		val_addl = VO_ADDL_PTR_WORD(v1_ptr_data->set2_ptr, v1_ptr_data->set1_ptr,
			v1_ptr_data->zerov_ptr, v1_ptr_data->other_ptr);
		kona_pwrmgr_writel(pwrmgr, val_addl, offset_addl);
	}

	/* TODO-IRQRESTORE */
}

static int kona_pwrmgr_seq_init(struct i2c_seq_cmd *i2c_cmds, int num_cmds,
				struct i2c_seq_vo_cmd_data *v0_ptr_data,
				struct i2c_seq_vo_cmd_data *v1_ptr_data)
{
	u8 i;

	/* Disable sequencer before applying changes */
	kona_pwrmgr_seq_enable(false);

	kona_pwrmgr_seq_write_cmds(i2c_cmds, num_cmds);
	for (i = VOLT0; i < VOLT_MAX; i++) {
		kona_pwrmgr_seq_write_vo_ptr_data(i, v0_ptr_data, v1_ptr_data);
	}

	return 0;
};

static const struct of_device_id kona_pwrmgr_match[] = {
	{ .compatible = "brcm,bcm21664-pwrmgr", .data = &bcm21664_pwrmgr_info },
	{ .compatible = "brcm,bcm23550-pwrmgr", .data = &bcm23550_pwrmgr_info },
	{ /* sentinel */ }
};

/*
 * Power manager needs to be initialized before SMP (we need to
 * bring up the PIs responsible for CPU cores and basic clocks),
 * so we start initialization in an early initcall.
 */

static int __init kona_pwrmgr_early_init(void)
{
	const struct of_device_id *match;
	struct device_node *np;
	struct resource regs;
	struct event_table event;
	bool ac, atl;
	int i;

	ac = true;
	atl = false;

	/* Map registers */
	np = of_find_matching_node_and_match(NULL, kona_pwrmgr_match, &match);
	if (!np)
		return 0;

	if (of_address_to_resource(np, 0, &regs) < 0) {
		pr_err("failed to get power manager registers");
		of_node_put(np);
		return -ENXIO;
	}

	pwrmgr->base = ioremap(regs.start, resource_size(&regs));
	if (!pwrmgr->base) {
		pr_err("failed to map power manager registers");
		of_node_put(np);
		return -ENXIO;
	}

	if (of_device_is_available(np)) {
		pwrmgr->info = match->data;

		pwrmgr->kona_pi_onecell.domains = kcalloc(ARRAY_SIZE(pwrmgr->domains),
				sizeof(*pwrmgr->domains),
				GFP_KERNEL);
		if (!pwrmgr->kona_pi_onecell.domains)
			return -ENOMEM;
		pwrmgr->kona_pi_onecell.num_domains = ARRAY_SIZE(pwrmgr->domains);

		/* Initialize sequencer with dummy values */
		kona_pwrmgr_seq_init(i2c_dummy_seq_cmds, i2c_dummy_seq_num_cmds,
				pwrmgr->info->dummy_seq_v0_data,
				pwrmgr->info->dummy_seq_v1_data);

		/* Clear all events */
		kona_pwrmgr_clear_events(LCDTE_EVENT, PWRMGR_NUM_EVENTS - 1);

		kona_pwrmgr_event_set_active(SOFTWARE_2_EVENT, true);
		kona_pwrmgr_event_set_active(SOFTWARE_0_EVENT, true);

		/* Prepare PC pins for sequencer */
		kona_pwrmgr_pc_set_sw_override(PC0, false, 0);
		kona_pwrmgr_pc_set_clkreq_override(PC0, true, 1);

		kona_pwrmgr_pc_set_sw_override(PC1, false, 0);
		kona_pwrmgr_pc_set_sw_override(PC2, false, 0);
		kona_pwrmgr_pc_set_sw_override(PC3, false, 0);
		kona_pwrmgr_pc_set_clkreq_override(PC1, false, 0);
		kona_pwrmgr_pc_set_clkreq_override(PC2, false, 0);
		kona_pwrmgr_pc_set_clkreq_override(PC3, false, 0);

		/* Enable sequencer */
		kona_pwrmgr_seq_enable(true);

		/* Initialize event table */
		for (i = 0; i < pwrmgr->info->event_table_length; i++) {
			event = pwrmgr->info->event_table[i];

			kona_pwrmgr_event_set_trig_type(event.event_id, event.trig_type);

			kona_pwrmgr_event_set_pi_policy(event.event_id, BCMKONA_POWER_DOMAIN_MODEM,
					ac, atl, event.policy_modem);
			kona_pwrmgr_event_set_pi_policy(event.event_id, BCMKONA_POWER_DOMAIN_ARM_CORE,
					ac, atl, event.policy_arm);
			kona_pwrmgr_event_set_pi_policy(event.event_id, BCMKONA_POWER_DOMAIN_ARM_SUBSYSTEM,
					ac, atl, event.policy_arm_sub);
			kona_pwrmgr_event_set_pi_policy(event.event_id, BCMKONA_POWER_DOMAIN_HUB_AON,
					ac, atl, event.policy_aon);
			kona_pwrmgr_event_set_pi_policy(event.event_id, BCMKONA_POWER_DOMAIN_HUB_SWITCHABLE,
					ac, atl, event.policy_hub);
			kona_pwrmgr_event_set_pi_policy(event.event_id, BCMKONA_POWER_DOMAIN_MM,
					ac, atl, event.policy_mm);
		}

		/* Initialize power domains */
		for (i = 0; i < BCMKONA_POWER_DOMAIN_COUNT; i++) {
			kona_pwrmgr_pi_init(i);
			/* TODO: error checking */
		}

		/* HACK: enable all power domains */
		for (i = 0; i < BCMKONA_POWER_DOMAIN_COUNT; i++) {
			kona_pwrmgr_event_set_active(SOFTWARE_0_EVENT, false);
			kona_pwrmgr_event_set_pi_policy(SOFTWARE_0_EVENT, i, true, false, PI_POLICY_RET);
			kona_pwrmgr_event_set_active(SOFTWARE_0_EVENT, true);
		}

		pr_info("kona-pwrmgr: initialized");
		of_node_put(np);
	}

	return 0;
}
early_initcall(kona_pwrmgr_early_init);
