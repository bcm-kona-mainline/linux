// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2013 Broadcom Corporation
 * Copyright 2013 Linaro Limited
 */

#include "clk-kona.h"

#include <linux/delay.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/clk-provider.h>
#include <linux/clk.h>

/*
 * "Policies" affect the frequencies of bus clocks provided by a
 * CCU.  (I believe these polices are named "Deep Sleep", "Economy",
 * "Normal", and "Turbo".)  A lower policy number has lower power
 * consumption, and policy 2 is the default.
 */
#define CCU_POLICY_COUNT	4

#define CCU_ACCESS_PASSWORD      0xA5A500
#define CLK_GATE_DELAY_LOOP      2000

/* Bitfield operations */

/* Produces a mask of set bits covering a range of a 32-bit value */
static inline u32 bitfield_mask(u32 shift, u32 width)
{
	return ((1 << width) - 1) << shift;
}

/* Extract the value of a bitfield found within a given register value */
static inline u32 bitfield_extract(u32 reg_val, u32 shift, u32 width)
{
	return (reg_val & bitfield_mask(shift, width)) >> shift;
}

/* Replace the value of a bitfield found within a given register value */
static inline u32 bitfield_replace(u32 reg_val, u32 shift, u32 width, u32 val)
{
	u32 mask = bitfield_mask(shift, width);

	return (reg_val & ~mask) | (val << shift);
}

/* Divider and scaling helpers */

/* Convert a divider into the scaled divisor value it represents. */
static inline u64 scaled_div_value(struct bcm_clk_div *div, u32 reg_div)
{
	return (u64)reg_div + ((u64)1 << div->u.s.frac_width);
}

/*
 * Build a scaled divider value as close as possible to the
 * given whole part (div_value) and fractional part (expressed
 * in billionths).
 */
u64 scaled_div_build(struct bcm_clk_div *div, u32 div_value, u32 billionths)
{
	u64 combined;

	BUG_ON(!div_value);
	BUG_ON(billionths >= BILLION);

	combined = (u64)div_value * BILLION + billionths;
	combined <<= div->u.s.frac_width;

	return DIV_ROUND_CLOSEST_ULL(combined, BILLION);
}

/* The scaled minimum divisor representable by a divider */
static inline u64
scaled_div_min(struct bcm_clk_div *div)
{
	if (divider_is_fixed(div))
		return (u64)div->u.fixed;

	return scaled_div_value(div, 0);
}

/* The scaled maximum divisor representable by a divider */
u64 scaled_div_max(struct bcm_clk_div *div)
{
	u32 reg_div;

	if (divider_is_fixed(div))
		return (u64)div->u.fixed;

	reg_div = ((u32)1 << div->u.s.width) - 1;

	return scaled_div_value(div, reg_div);
}

/*
 * Convert a scaled divisor into its divider representation as
 * stored in a divider register field.
 */
static inline u32
divider(struct bcm_clk_div *div, u64 scaled_div)
{
	BUG_ON(scaled_div < scaled_div_min(div));
	BUG_ON(scaled_div > scaled_div_max(div));

	return (u32)(scaled_div - ((u64)1 << div->u.s.frac_width));
}

/* Return a rate scaled for use when dividing by a scaled divisor. */
static inline u64
scale_rate(struct bcm_clk_div *div, u32 rate)
{
	if (divider_is_fixed(div))
		return (u64)rate;

	return (u64)rate << div->u.s.frac_width;
}

/* CCU access */

/* Read a 32-bit register value from a CCU's address space. */
static inline u32 __ccu_read(struct ccu_data *ccu, u32 reg_offset)
{
	return readl(ccu->base + reg_offset);
}

/* Write a 32-bit register value into a CCU's address space. */
static inline void
__ccu_write(struct ccu_data *ccu, u32 reg_offset, u32 reg_val)
{
	writel(reg_val, ccu->base + reg_offset);
}

static inline unsigned long ccu_lock(struct ccu_data *ccu)
{
	unsigned long flags;

	spin_lock_irqsave(&ccu->lock, flags);

	return flags;
}
static inline void ccu_unlock(struct ccu_data *ccu, unsigned long flags)
{
	spin_unlock_irqrestore(&ccu->lock, flags);
}

/*
 * Enable/disable write access to CCU protected registers.  The
 * WR_ACCESS register for all CCUs is at offset 0.
 */
static inline void __ccu_write_enable(struct ccu_data *ccu)
{
	if (ccu->write_enabled) {
		pr_err("%s: access already enabled for %s\n", __func__,
			ccu->name);
		return;
	}
	ccu->write_enabled = true;
	__ccu_write(ccu, 0, CCU_ACCESS_PASSWORD | 1);
}

static inline void __ccu_write_disable(struct ccu_data *ccu)
{
	if (!ccu->write_enabled) {
		pr_err("%s: access wasn't enabled for %s\n", __func__,
			ccu->name);
		return;
	}

	__ccu_write(ccu, 0, CCU_ACCESS_PASSWORD);
	ccu->write_enabled = false;
}

/*
 * Poll a register in a CCU's address space, returning when the
 * specified bit in that register's value is set (or clear).  Delay
 * a microsecond after each read of the register.  Returns true if
 * successful, or false if we gave up trying.
 *
 * Caller must ensure the CCU lock is held.
 */
static inline bool
__ccu_wait_bit(struct ccu_data *ccu, u32 reg_offset, u32 bit, bool want)
{
	unsigned int tries;
	u32 bit_mask = 1 << bit;

	for (tries = 0; tries < CLK_GATE_DELAY_LOOP; tries++) {
		u32 val;
		bool bit_val;

		val = __ccu_read(ccu, reg_offset);
		bit_val = (val & bit_mask) != 0;
		if (bit_val == want)
			return true;
		udelay(1);
	}
	pr_warn("%s: %s/0x%04x bit %u was never %s\n", __func__,
		ccu->name, reg_offset, bit, want ? "set" : "clear");

	return false;
}

/* Policy operations */

static bool __ccu_policy_engine_start(struct ccu_data *ccu, bool sync)
{
	struct bcm_policy_ctl *control = &ccu->policy.control;
	u32 offset;
	u32 go_bit;
	u32 mask;
	bool ret;

	/* If we don't need to control policy for this CCU, we're done. */
	if (!policy_ctl_exists(control))
		return true;

	offset = control->offset;
	go_bit = control->go_bit;

	/* Ensure we're not busy before we start */
	ret = __ccu_wait_bit(ccu, offset, go_bit, false);
	if (!ret) {
		pr_err("%s: ccu %s policy engine wouldn't go idle\n",
			__func__, ccu->name);
		return false;
	}

	/*
	 * If it's a synchronous request, we'll wait for the voltage
	 * and frequency of the active load to stabilize before
	 * returning.  To do this we select the active load by
	 * setting the ATL bit.
	 *
	 * An asynchronous request instead ramps the voltage in the
	 * background, and when that process stabilizes, the target
	 * load is copied to the active load and the CCU frequency
	 * is switched.  We do this by selecting the target load
	 * (ATL bit clear) and setting the request auto-copy (AC bit
	 * set).
	 *
	 * Note, we do NOT read-modify-write this register.
	 */
	mask = (u32)1 << go_bit;
	if (sync)
		mask |= 1 << control->atl_bit;
	else
		mask |= 1 << control->ac_bit;
	__ccu_write(ccu, offset, mask);

	/* Wait for indication that operation is complete. */
	ret = __ccu_wait_bit(ccu, offset, go_bit, false);
	if (!ret)
		pr_err("%s: ccu %s policy engine never started\n",
			__func__, ccu->name);

	return ret;
}

static bool __ccu_policy_engine_stop(struct ccu_data *ccu)
{
	struct bcm_lvm_en *enable = &ccu->policy.enable;
	u32 offset;
	u32 enable_bit;
	bool ret;

	/* If we don't need to control policy for this CCU, we're done. */
	if (!policy_lvm_en_exists(enable))
		return true;

	/* Ensure we're not busy before we start */
	offset = enable->offset;
	enable_bit = enable->bit;
	ret = __ccu_wait_bit(ccu, offset, enable_bit, false);
	if (!ret) {
		pr_err("%s: ccu %s policy engine already stopped\n",
			__func__, ccu->name);
		return false;
	}

	/* Now set the bit to stop the engine (NO read-modify-write) */
	__ccu_write(ccu, offset, (u32)1 << enable_bit);

	/* Wait for indication that it has stopped. */
	ret = __ccu_wait_bit(ccu, offset, enable_bit, false);
	if (!ret)
		pr_err("%s: ccu %s policy engine never stopped\n",
			__func__, ccu->name);

	return ret;
}

/*
 * A CCU has four operating conditions ("policies"), and some clocks
 * can be disabled or enabled based on which policy is currently in
 * effect.  Such clocks have a bit in a "policy mask" register for
 * each policy indicating whether the clock is enabled for that
 * policy or not.  The bit position for a clock is the same for all
 * four registers, and the 32-bit registers are at consecutive
 * addresses.
 */
static bool policy_init(struct ccu_data *ccu, struct bcm_clk_policy *policy)
{
	u32 offset;
	u32 mask;
	int i;
	bool ret;

	if (!policy_exists(policy))
		return true;

	/*
	 * We need to stop the CCU policy engine to allow update
	 * of our policy bits.
	 */
	if (!__ccu_policy_engine_stop(ccu)) {
		pr_err("%s: unable to stop CCU %s policy engine\n",
			__func__, ccu->name);
		return false;
	}

	/*
	 * For now, if a clock defines its policy bit we just mark
	 * it "enabled" for all four policies.
	 */
	offset = policy->offset;
	mask = (u32)1 << policy->bit;
	for (i = 0; i < CCU_POLICY_COUNT; i++) {
		u32 reg_val;

		reg_val = __ccu_read(ccu, offset);
		reg_val |= mask;
		__ccu_write(ccu, offset, reg_val);
		offset += sizeof(u32);
	}

	/* We're done updating; fire up the policy engine again. */
	ret = __ccu_policy_engine_start(ccu, true);
	if (!ret)
		pr_err("%s: unable to restart CCU %s policy engine\n",
			__func__, ccu->name);

	return ret;
}

/* Gate operations */

/* Determine whether a clock is gated.  CCU lock must be held.  */
static bool
__is_clk_gate_enabled(struct ccu_data *ccu, struct bcm_clk_gate *gate)
{
	u32 bit_mask;
	u32 reg_val;

	/* If there is no gate we can assume it's enabled. */
	if (!gate_exists(gate))
		return true;

	bit_mask = 1 << gate->status_bit;
	reg_val = __ccu_read(ccu, gate->offset);

	return (reg_val & bit_mask) != 0;
}

/* Determine whether a clock is gated. */
static bool
is_clk_gate_enabled(struct ccu_data *ccu, struct bcm_clk_gate *gate)
{
	long flags;
	bool ret;

	/* Avoid taking the lock if we can */
	if (!gate_exists(gate))
		return true;

	flags = ccu_lock(ccu);
	ret = __is_clk_gate_enabled(ccu, gate);
	ccu_unlock(ccu, flags);

	return ret;
}

/*
 * Commit our desired gate state to the hardware.
 * Returns true if successful, false otherwise.
 */
static bool
__gate_commit(struct ccu_data *ccu, struct bcm_clk_gate *gate)
{
	u32 reg_val;
	u32 mask;
	bool enabled = false;

	BUG_ON(!gate_exists(gate));
	if (!gate_is_sw_controllable(gate))
		return true;		/* Nothing we can change */

	reg_val = __ccu_read(ccu, gate->offset);

	/* For a hardware/software gate, set which is in control */
	if (gate_is_hw_controllable(gate)) {
		mask = (u32)1 << gate->hw_sw_sel_bit;
		if (gate_is_sw_managed(gate))
			reg_val |= mask;
		else
			reg_val &= ~mask;
	}

	/*
	 * If software is in control, enable or disable the gate.
	 * If hardware is, clear the enabled bit for good measure.
	 * If a software controlled gate can't be disabled, we're
	 * required to write a 0 into the enable bit (but the gate
	 * will be enabled).
	 */
	mask = (u32)1 << gate->en_bit;
	if (gate_is_sw_managed(gate) && (enabled = gate_is_enabled(gate)) &&
			!gate_is_no_disable(gate))
		reg_val |= mask;
	else
		reg_val &= ~mask;

	__ccu_write(ccu, gate->offset, reg_val);

	/* For a hardware controlled gate, we're done */
	if (!gate_is_sw_managed(gate))
		return true;

	/* Otherwise wait for the gate to be in desired state */
	return __ccu_wait_bit(ccu, gate->offset, gate->status_bit, enabled);
}

/*
 * Initialize a gate.  Our desired state (hardware/software select,
 * and if software, its enable state) is committed to hardware
 * without the usual checks to see if it's already set up that way.
 * Returns true if successful, false otherwise.
 */
static bool gate_init(struct ccu_data *ccu, struct bcm_clk_gate *gate)
{
	if (!gate_exists(gate))
		return true;
	return __gate_commit(ccu, gate);
}

/*
 * Set a gate to enabled or disabled state.  Does nothing if the
 * gate is not currently under software control, or if it is already
 * in the requested state.  Returns true if successful, false
 * otherwise.  CCU lock must be held.
 */
static bool
__clk_gate(struct ccu_data *ccu, struct bcm_clk_gate *gate, bool enable)
{
	bool ret;

	if (!gate_exists(gate) || !gate_is_sw_managed(gate))
		return true;	/* Nothing to do */

	if (!enable && gate_is_no_disable(gate)) {
		pr_warn("%s: invalid gate disable request (ignoring)\n",
			__func__);
		return true;
	}

	if (enable == gate_is_enabled(gate))
		return true;	/* No change */

	gate_flip_enabled(gate);
	ret = __gate_commit(ccu, gate);
	if (!ret)
		gate_flip_enabled(gate);	/* Revert the change */

	return ret;
}

/* Enable or disable a gate.  Returns 0 if successful, -EIO otherwise */
static int clk_gate(struct ccu_data *ccu, const char *name,
			struct bcm_clk_gate *gate, bool enable)
{
	unsigned long flags;
	bool success;

	/*
	 * Avoid taking the lock if we can.  We quietly ignore
	 * requests to change state that don't make sense.
	 */
	if (!gate_exists(gate) || !gate_is_sw_managed(gate))
		return 0;
	if (!enable && gate_is_no_disable(gate))
		return 0;

	flags = ccu_lock(ccu);
	__ccu_write_enable(ccu);

	success = __clk_gate(ccu, gate, enable);

	__ccu_write_disable(ccu);
	ccu_unlock(ccu, flags);

	if (success)
		return 0;

	pr_err("%s: failed to %s gate for %s\n", __func__,
		enable ? "enable" : "disable", name);

	return -EIO;
}

/* Hysteresis operations */

/*
 * If a clock gate requires a turn-off delay it will have
 * "hysteresis" register bits defined.  The first, if set, enables
 * the delay; and if enabled, the second bit determines whether the
 * delay is "low" or "high" (1 means high).  For now, if it's
 * defined for a clock, we set it.
 */
static bool hyst_init(struct ccu_data *ccu, struct bcm_clk_hyst *hyst)
{
	u32 offset;
	u32 reg_val;
	u32 mask;

	if (!hyst_exists(hyst))
		return true;

	offset = hyst->offset;
	mask = (u32)1 << hyst->en_bit;
	mask |= (u32)1 << hyst->val_bit;

	reg_val = __ccu_read(ccu, offset);
	reg_val |= mask;
	__ccu_write(ccu, offset, reg_val);

	return true;
}

/* Trigger operations */

/*
 * Caller must ensure CCU lock is held and access is enabled.
 * Returns true if successful, false otherwise.
 */
static bool __clk_trigger(struct ccu_data *ccu, struct bcm_clk_trig *trig)
{
	/* Trigger the clock and wait for it to finish */
	__ccu_write(ccu, trig->offset, 1 << trig->bit);

	return __ccu_wait_bit(ccu, trig->offset, trig->bit, false);
}

/* Divider operations */

/* Read a divider value and return the scaled divisor it represents. */
static u64 divider_read_scaled(struct ccu_data *ccu, struct bcm_clk_div *div)
{
	unsigned long flags;
	u32 reg_val;
	u32 reg_div;

	if (divider_is_fixed(div))
		return (u64)div->u.fixed;

	flags = ccu_lock(ccu);
	reg_val = __ccu_read(ccu, div->u.s.offset);
	ccu_unlock(ccu, flags);

	/* Extract the full divider field from the register value */
	reg_div = bitfield_extract(reg_val, div->u.s.shift, div->u.s.width);

	/* Return the scaled divisor value it represents */
	return scaled_div_value(div, reg_div);
}

/*
 * Convert a divider's scaled divisor value into its recorded form
 * and commit it into the hardware divider register.
 *
 * Returns 0 on success.  Returns -EINVAL for invalid arguments.
 * Returns -ENXIO if gating failed, and -EIO if a trigger failed.
 */
static int __div_commit(struct ccu_data *ccu, struct bcm_clk_gate *gate,
			struct bcm_clk_div *div, struct bcm_clk_trig *trig)
{
	bool enabled;
	u32 reg_div;
	u32 reg_val;
	int ret = 0;

	BUG_ON(divider_is_fixed(div));

	/*
	 * If we're just initializing the divider, and no initial
	 * state was defined in the device tree, we just find out
	 * what its current value is rather than updating it.
	 */
	if (div->u.s.scaled_div == BAD_SCALED_DIV_VALUE) {
		reg_val = __ccu_read(ccu, div->u.s.offset);
		reg_div = bitfield_extract(reg_val, div->u.s.shift,
						div->u.s.width);
		div->u.s.scaled_div = scaled_div_value(div, reg_div);

		return 0;
	}

	/* Convert the scaled divisor to the value we need to record */
	reg_div = divider(div, div->u.s.scaled_div);

	/* Clock needs to be enabled before changing the rate */
	enabled = __is_clk_gate_enabled(ccu, gate);
	if (!enabled && !__clk_gate(ccu, gate, true)) {
		ret = -ENXIO;
		goto out;
	}

	/* Replace the divider value and record the result */
	reg_val = __ccu_read(ccu, div->u.s.offset);
	reg_val = bitfield_replace(reg_val, div->u.s.shift, div->u.s.width,
					reg_div);
	__ccu_write(ccu, div->u.s.offset, reg_val);

	/* If the trigger fails we still want to disable the gate */
	if (!__clk_trigger(ccu, trig))
		ret = -EIO;

	/* Disable the clock again if it was disabled to begin with */
	if (!enabled && !__clk_gate(ccu, gate, false))
		ret = ret ? ret : -ENXIO;	/* return first error */
out:
	return ret;
}

/*
 * Initialize a divider by committing our desired state to hardware
 * without the usual checks to see if it's already set up that way.
 * Returns true if successful, false otherwise.
 */
static bool div_init(struct ccu_data *ccu, struct bcm_clk_gate *gate,
			struct bcm_clk_div *div, struct bcm_clk_trig *trig)
{
	if (!divider_exists(div) || divider_is_fixed(div))
		return true;
	return !__div_commit(ccu, gate, div, trig);
}

static int divider_write(struct ccu_data *ccu, struct bcm_clk_gate *gate,
			struct bcm_clk_div *div, struct bcm_clk_trig *trig,
			u64 scaled_div)
{
	unsigned long flags;
	u64 previous;
	int ret;

	BUG_ON(divider_is_fixed(div));

	previous = div->u.s.scaled_div;
	if (previous == scaled_div)
		return 0;	/* No change */

	div->u.s.scaled_div = scaled_div;

	flags = ccu_lock(ccu);
	__ccu_write_enable(ccu);

	ret = __div_commit(ccu, gate, div, trig);

	__ccu_write_disable(ccu);
	ccu_unlock(ccu, flags);

	if (ret)
		div->u.s.scaled_div = previous;		/* Revert the change */

	return ret;

}

/* Common clock rate helpers */

/*
 * Implement the common clock framework recalc_rate method, taking
 * into account a divider and an optional pre-divider.  The
 * pre-divider register pointer may be NULL.
 */
static unsigned long clk_recalc_rate(struct ccu_data *ccu,
			struct bcm_clk_div *div, struct bcm_clk_div *pre_div,
			unsigned long parent_rate)
{
	u64 scaled_parent_rate;
	u64 scaled_div;
	u64 result;

	if (!divider_exists(div))
		return parent_rate;

	if (parent_rate > (unsigned long)LONG_MAX)
		return 0;	/* actually this would be a caller bug */

	/*
	 * If there is a pre-divider, divide the scaled parent rate
	 * by the pre-divider value first.  In this case--to improve
	 * accuracy--scale the parent rate by *both* the pre-divider
	 * value and the divider before actually computing the
	 * result of the pre-divider.
	 *
	 * If there's only one divider, just scale the parent rate.
	 */
	if (pre_div && divider_exists(pre_div)) {
		u64 scaled_rate;

		scaled_rate = scale_rate(pre_div, parent_rate);
		scaled_rate = scale_rate(div, scaled_rate);
		scaled_div = divider_read_scaled(ccu, pre_div);
		scaled_parent_rate = DIV_ROUND_CLOSEST_ULL(scaled_rate,
							scaled_div);
	} else  {
		scaled_parent_rate = scale_rate(div, parent_rate);
	}

	/*
	 * Get the scaled divisor value, and divide the scaled
	 * parent rate by that to determine this clock's resulting
	 * rate.
	 */
	scaled_div = divider_read_scaled(ccu, div);
	result = DIV_ROUND_CLOSEST_ULL(scaled_parent_rate, scaled_div);

	return (unsigned long)result;
}

/*
 * Compute the output rate produced when a given parent rate is fed
 * into two dividers.  The pre-divider can be NULL, and even if it's
 * non-null it may be nonexistent.  It's also OK for the divider to
 * be nonexistent, and in that case the pre-divider is also ignored.
 *
 * If scaled_div is non-null, it is used to return the scaled divisor
 * value used by the (downstream) divider to produce that rate.
 */
static long round_rate(struct ccu_data *ccu, struct bcm_clk_div *div,
				struct bcm_clk_div *pre_div,
				unsigned long rate, unsigned long parent_rate,
				u64 *scaled_div)
{
	u64 scaled_parent_rate;
	u64 min_scaled_div;
	u64 max_scaled_div;
	u64 best_scaled_div;
	u64 result;

	BUG_ON(!divider_exists(div));
	BUG_ON(!rate);
	BUG_ON(parent_rate > (u64)LONG_MAX);

	/*
	 * If there is a pre-divider, divide the scaled parent rate
	 * by the pre-divider value first.  In this case--to improve
	 * accuracy--scale the parent rate by *both* the pre-divider
	 * value and the divider before actually computing the
	 * result of the pre-divider.
	 *
	 * If there's only one divider, just scale the parent rate.
	 *
	 * For simplicity we treat the pre-divider as fixed (for now).
	 */
	if (divider_exists(pre_div)) {
		u64 scaled_rate;
		u64 scaled_pre_div;

		scaled_rate = scale_rate(pre_div, parent_rate);
		scaled_rate = scale_rate(div, scaled_rate);
		scaled_pre_div = divider_read_scaled(ccu, pre_div);
		scaled_parent_rate = DIV_ROUND_CLOSEST_ULL(scaled_rate,
							scaled_pre_div);
	} else {
		scaled_parent_rate = scale_rate(div, parent_rate);
	}

	/*
	 * Compute the best possible divider and ensure it is in
	 * range.  A fixed divider can't be changed, so just report
	 * the best we can do.
	 */
	if (!divider_is_fixed(div)) {
		best_scaled_div = DIV_ROUND_CLOSEST_ULL(scaled_parent_rate,
							rate);
		min_scaled_div = scaled_div_min(div);
		max_scaled_div = scaled_div_max(div);
		if (best_scaled_div > max_scaled_div)
			best_scaled_div = max_scaled_div;
		else if (best_scaled_div < min_scaled_div)
			best_scaled_div = min_scaled_div;
	} else {
		best_scaled_div = divider_read_scaled(ccu, div);
	}

	/* OK, figure out the resulting rate */
	result = DIV_ROUND_CLOSEST_ULL(scaled_parent_rate, best_scaled_div);

	if (scaled_div)
		*scaled_div = best_scaled_div;

	return (long)result;
}

/* Common clock parent helpers */

/*
 * For a given parent selector (register field) value, find the
 * index into a selector's parent_sel array that contains it.
 * Returns the index, or BAD_CLK_INDEX if it's not found.
 */
static u8 parent_index(struct bcm_clk_sel *sel, u8 parent_sel)
{
	u8 i;

	BUG_ON(sel->parent_count > (u32)U8_MAX);
	for (i = 0; i < sel->parent_count; i++)
		if (sel->parent_sel[i] == parent_sel)
			return i;
	return BAD_CLK_INDEX;
}

/*
 * Fetch the current value of the selector, and translate that into
 * its corresponding index in the parent array we registered with
 * the clock framework.
 *
 * Returns parent array index that corresponds with the value found,
 * or BAD_CLK_INDEX if the found value is out of range.
 */
static u8 selector_read_index(struct ccu_data *ccu, struct bcm_clk_sel *sel)
{
	unsigned long flags;
	u32 reg_val;
	u32 parent_sel;
	u8 index;

	/* If there's no selector, there's only one parent */
	if (!selector_exists(sel))
		return 0;

	/* Get the value in the selector register */
	flags = ccu_lock(ccu);
	reg_val = __ccu_read(ccu, sel->offset);
	ccu_unlock(ccu, flags);

	parent_sel = bitfield_extract(reg_val, sel->shift, sel->width);

	/* Look up that selector's parent array index and return it */
	index = parent_index(sel, parent_sel);
	if (index == BAD_CLK_INDEX)
		pr_err("%s: out-of-range parent selector %u (%s 0x%04x)\n",
			__func__, parent_sel, ccu->name, sel->offset);

	return index;
}

/*
 * Commit our desired selector value to the hardware.
 *
 * Returns 0 on success.  Returns -EINVAL for invalid arguments.
 * Returns -ENXIO if gating failed, and -EIO if a trigger failed.
 */
static int
__sel_commit(struct ccu_data *ccu, struct bcm_clk_gate *gate,
			struct bcm_clk_sel *sel, struct bcm_clk_trig *trig)
{
	u32 parent_sel;
	u32 reg_val;
	bool enabled;
	int ret = 0;

	BUG_ON(!selector_exists(sel));

	/*
	 * If we're just initializing the selector, and no initial
	 * state was defined in the device tree, we just find out
	 * what its current value is rather than updating it.
	 */
	if (sel->clk_index == BAD_CLK_INDEX) {
		u8 index;

		reg_val = __ccu_read(ccu, sel->offset);
		parent_sel = bitfield_extract(reg_val, sel->shift, sel->width);
		index = parent_index(sel, parent_sel);
		if (index == BAD_CLK_INDEX)
			return -EINVAL;
		sel->clk_index = index;

		return 0;
	}

	BUG_ON((u32)sel->clk_index >= sel->parent_count);
	parent_sel = sel->parent_sel[sel->clk_index];

	/* Clock needs to be enabled before changing the parent */
	enabled = __is_clk_gate_enabled(ccu, gate);
	if (!enabled && !__clk_gate(ccu, gate, true))
		return -ENXIO;

	/* Replace the selector value and record the result */
	reg_val = __ccu_read(ccu, sel->offset);
	reg_val = bitfield_replace(reg_val, sel->shift, sel->width, parent_sel);
	__ccu_write(ccu, sel->offset, reg_val);

	/* If the trigger fails we still want to disable the gate */
	if (!__clk_trigger(ccu, trig))
		ret = -EIO;

	/* Disable the clock again if it was disabled to begin with */
	if (!enabled && !__clk_gate(ccu, gate, false))
		ret = ret ? ret : -ENXIO;	/* return first error */

	return ret;
}

/*
 * Initialize a selector by committing our desired state to hardware
 * without the usual checks to see if it's already set up that way.
 * Returns true if successful, false otherwise.
 */
static bool sel_init(struct ccu_data *ccu, struct bcm_clk_gate *gate,
			struct bcm_clk_sel *sel, struct bcm_clk_trig *trig)
{
	if (!selector_exists(sel))
		return true;
	return !__sel_commit(ccu, gate, sel, trig);
}

/*
 * Write a new value into a selector register to switch to a
 * different parent clock.  Returns 0 on success, or an error code
 * (from __sel_commit()) otherwise.
 */
static int selector_write(struct ccu_data *ccu, struct bcm_clk_gate *gate,
			struct bcm_clk_sel *sel, struct bcm_clk_trig *trig,
			u8 index)
{
	unsigned long flags;
	u8 previous;
	int ret;

	previous = sel->clk_index;
	if (previous == index)
		return 0;	/* No change */

	sel->clk_index = index;

	flags = ccu_lock(ccu);
	__ccu_write_enable(ccu);

	ret = __sel_commit(ccu, gate, sel, trig);

	__ccu_write_disable(ccu);
	ccu_unlock(ccu, flags);

	if (ret)
		sel->clk_index = previous;	/* Revert the change */

	return ret;
}

/* CCU operations */

static void kona_ccu_set_voltage(struct ccu_data *ccu, int voltage_reg_num,
				u8 voltage_policy_id)
{
	u32 offset;
	u32 value;
	u8 shift;

	if (voltage_reg_num <= 3) {
		shift = voltage_reg_num << 3;
		offset = ccu->voltage.offset1;
	} else if ((voltage_reg_num <= 7) && ccu->voltage.offset2) {
		shift = (voltage_reg_num - 4) << 3;
		offset = ccu->voltage.offset2;
	} else {
		BUG();
	}

	value = __ccu_read(ccu, offset);
	value = (value & ~(0xF << shift)) |
		((voltage_policy_id & 0xF) << shift);

	__ccu_write(ccu, offset, value);
}

static void kona_ccu_set_peri_voltage(struct ccu_data *ccu, u8 peri_volt_reg_num,
				u8 peri_volt_policy_id)
{
	u32 value;
	u8 shift;

	shift = peri_volt_reg_num << 3;
	value = __ccu_read(ccu, ccu->peri_volt.offset);
	value = (value & ~(0xF << shift)) |
		((peri_volt_policy_id & 0xF) << shift);

	__ccu_write(ccu, ccu->peri_volt.offset, value);
}

static void kona_ccu_set_freq_policy(struct ccu_data *ccu, u8 freq_policy_reg_num,
				u8 freq_policy_policy_id)
{
	u32 value;
	u8 shift;

	shift = freq_policy_reg_num << 3;
	value = __ccu_read(ccu, ccu->freq_policy.offset);
	value = (value & ~(0x7 << shift)) |
		(freq_policy_policy_id << shift);

	__ccu_write(ccu, ccu->freq_policy.offset, value);
}

static void kona_ccu_interrupt_enable(struct ccu_data *ccu, u8 int_type,
				bool enable)
{
	u32 value;

	value = __ccu_read(ccu, ccu->interrupt.enable_offset);

	/* int_type doubles as a shift value. */
	if (enable)
		value |= (1 << int_type);
	else
		value &= ~(1 << int_type);

	__ccu_write(ccu, ccu->interrupt.enable_offset, value);
}

/* Peripheral clock operations */

static int kona_peri_clk_enable(struct clk_hw *hw)
{
	struct kona_clk *bcm_clk = to_kona_clk(hw);
	struct bcm_clk_gate *gate = &bcm_clk->u.reg_data->gate;

	return clk_gate(bcm_clk->ccu, bcm_clk->init_data.name, gate, true);
}

static void kona_peri_clk_disable(struct clk_hw *hw)
{
	struct kona_clk *bcm_clk = to_kona_clk(hw);
	struct bcm_clk_gate *gate = &bcm_clk->u.reg_data->gate;

	(void)clk_gate(bcm_clk->ccu, bcm_clk->init_data.name, gate, false);
}

static int kona_peri_clk_is_enabled(struct clk_hw *hw)
{
	struct kona_clk *bcm_clk = to_kona_clk(hw);
	struct bcm_clk_gate *gate = &bcm_clk->u.reg_data->gate;

	return is_clk_gate_enabled(bcm_clk->ccu, gate) ? 1 : 0;
}

static unsigned long kona_peri_clk_recalc_rate(struct clk_hw *hw,
			unsigned long parent_rate)
{
	struct kona_clk *bcm_clk = to_kona_clk(hw);
	struct clk_reg_data *data = bcm_clk->u.reg_data;

	return clk_recalc_rate(bcm_clk->ccu, &data->div, &data->pre_div,
				parent_rate);
}

static long kona_peri_clk_round_rate(struct clk_hw *hw, unsigned long rate,
			unsigned long *parent_rate)
{
	struct kona_clk *bcm_clk = to_kona_clk(hw);
	struct bcm_clk_div *div = &bcm_clk->u.reg_data->div;

	if (!divider_exists(div))
		return clk_hw_get_rate(hw);

	/* Quietly avoid a zero rate */
	return round_rate(bcm_clk->ccu, div, &bcm_clk->u.reg_data->pre_div,
				rate ? rate : 1, *parent_rate, NULL);
}

static int kona_peri_clk_determine_rate(struct clk_hw *hw,
					struct clk_rate_request *req)
{
	struct kona_clk *bcm_clk = to_kona_clk(hw);
	struct clk_hw *current_parent;
	unsigned long parent_rate;
	unsigned long best_delta;
	unsigned long best_rate;
	u32 parent_count;
	long rate;
	u32 which;

	/*
	 * If there is no other parent to choose, use the current one.
	 * Note:  We don't honor (or use) CLK_SET_RATE_NO_REPARENT.
	 */
	WARN_ON_ONCE(bcm_clk->init_data.flags & CLK_SET_RATE_NO_REPARENT);
	parent_count = (u32)bcm_clk->init_data.num_parents;
	if (parent_count < 2) {
		rate = kona_peri_clk_round_rate(hw, req->rate,
						&req->best_parent_rate);
		if (rate < 0)
			return rate;

		req->rate = rate;
		return 0;
	}

	/* Unless we can do better, stick with current parent */
	current_parent = clk_hw_get_parent(hw);
	parent_rate = clk_hw_get_rate(current_parent);
	best_rate = kona_peri_clk_round_rate(hw, req->rate, &parent_rate);
	best_delta = abs(best_rate - req->rate);

	/* Check whether any other parent clock can produce a better result */
	for (which = 0; which < parent_count; which++) {
		struct clk_hw *parent = clk_hw_get_parent_by_index(hw, which);
		unsigned long delta;
		unsigned long other_rate;

		BUG_ON(!parent);
		if (parent == current_parent)
			continue;

		/* We don't support CLK_SET_RATE_PARENT */
		parent_rate = clk_hw_get_rate(parent);
		other_rate = kona_peri_clk_round_rate(hw, req->rate,
						      &parent_rate);
		delta = abs(other_rate - req->rate);
		if (delta < best_delta) {
			best_delta = delta;
			best_rate = other_rate;
			req->best_parent_hw = parent;
			req->best_parent_rate = parent_rate;
		}
	}

	req->rate = best_rate;
	return 0;
}

static int kona_peri_clk_set_parent(struct clk_hw *hw, u8 index)
{
	struct kona_clk *bcm_clk = to_kona_clk(hw);
	struct clk_reg_data *data = bcm_clk->u.reg_data;
	struct bcm_clk_sel *sel = &data->sel;
	struct bcm_clk_trig *trig;
	int ret;

	BUG_ON(index >= sel->parent_count);

	/* If there's only one parent we don't require a selector */
	if (!selector_exists(sel))
		return 0;

	/*
	 * The regular trigger is used by default, but if there's a
	 * pre-trigger we want to use that instead.
	 */
	trig = trigger_exists(&data->pre_trig) ? &data->pre_trig
					       : &data->trig;

	ret = selector_write(bcm_clk->ccu, &data->gate, sel, trig, index);
	if (ret == -ENXIO) {
		pr_err("%s: gating failure for %s\n", __func__,
			bcm_clk->init_data.name);
		ret = -EIO;	/* Don't proliferate weird errors */
	} else if (ret == -EIO) {
		pr_err("%s: %strigger failed for %s\n", __func__,
			trig == &data->pre_trig ? "pre-" : "",
			bcm_clk->init_data.name);
	}

	return ret;
}

static u8 kona_peri_clk_get_parent(struct clk_hw *hw)
{
	struct kona_clk *bcm_clk = to_kona_clk(hw);
	struct clk_reg_data *data = bcm_clk->u.reg_data;
	u8 index;

	index = selector_read_index(bcm_clk->ccu, &data->sel);

	/* Not all callers would handle an out-of-range value gracefully */
	return index == BAD_CLK_INDEX ? 0 : index;
}

static int kona_peri_clk_set_rate(struct clk_hw *hw, unsigned long rate,
			unsigned long parent_rate)
{
	struct kona_clk *bcm_clk = to_kona_clk(hw);
	struct clk_reg_data *data = bcm_clk->u.reg_data;
	struct bcm_clk_div *div = &data->div;
	u64 scaled_div = 0;
	int ret;

	if (parent_rate > (unsigned long)LONG_MAX)
		return -EINVAL;

	if (rate == clk_hw_get_rate(hw))
		return 0;

	if (!divider_exists(div))
		return rate == parent_rate ? 0 : -EINVAL;

	/*
	 * A fixed divider can't be changed.  (Nor can a fixed
	 * pre-divider be, but for now we never actually try to
	 * change that.)  Tolerate a request for a no-op change.
	 */
	if (divider_is_fixed(&data->div))
		return rate == parent_rate ? 0 : -EINVAL;

	/*
	 * Get the scaled divisor value needed to achieve a clock
	 * rate as close as possible to what was requested, given
	 * the parent clock rate supplied.
	 */
	(void)round_rate(bcm_clk->ccu, div, &data->pre_div,
				rate ? rate : 1, parent_rate, &scaled_div);

	/*
	 * We aren't updating any pre-divider at this point, so
	 * we'll use the regular trigger.
	 */
	ret = divider_write(bcm_clk->ccu, &data->gate, &data->div,
				&data->trig, scaled_div);
	if (ret == -ENXIO) {
		pr_err("%s: gating failure for %s\n", __func__,
			bcm_clk->init_data.name);
		ret = -EIO;	/* Don't proliferate weird errors */
	} else if (ret == -EIO) {
		pr_err("%s: trigger failed for %s\n", __func__,
			bcm_clk->init_data.name);
	}

	return ret;
}

struct clk_ops kona_peri_clk_ops = {
	.enable = kona_peri_clk_enable,
	.disable = kona_peri_clk_disable,
	.is_enabled = kona_peri_clk_is_enabled,
	.recalc_rate = kona_peri_clk_recalc_rate,
	.determine_rate = kona_peri_clk_determine_rate,
	.set_parent = kona_peri_clk_set_parent,
	.get_parent = kona_peri_clk_get_parent,
	.set_rate = kona_peri_clk_set_rate,
};

/* Put a peripheral clock into its initial state */
static bool __peri_clk_init(struct kona_clk *bcm_clk)
{
	struct ccu_data *ccu = bcm_clk->ccu;
	struct clk_reg_data *peri = bcm_clk->u.reg_data;
	const char *name = bcm_clk->init_data.name;
	struct bcm_clk_trig *trig;

	BUG_ON(bcm_clk->type != bcm_clk_peri);

	if (!policy_init(ccu, &peri->policy)) {
		pr_err("%s: error initializing policy for %s\n",
			__func__, name);
		return false;
	}
	if (!gate_init(ccu, &peri->gate)) {
		pr_err("%s: error initializing gate for %s\n", __func__, name);
		return false;
	}
	if (!hyst_init(ccu, &peri->hyst)) {
		pr_err("%s: error initializing hyst for %s\n", __func__, name);
		return false;
	}
	if (!div_init(ccu, &peri->gate, &peri->div, &peri->trig)) {
		pr_err("%s: error initializing divider for %s\n", __func__,
			name);
		return false;
	}

	/*
	 * For the pre-divider and selector, the pre-trigger is used
	 * if it's present, otherwise we just use the regular trigger.
	 */
	trig = trigger_exists(&peri->pre_trig) ? &peri->pre_trig
					       : &peri->trig;

	if (!div_init(ccu, &peri->gate, &peri->pre_div, trig)) {
		pr_err("%s: error initializing pre-divider for %s\n", __func__,
			name);
		return false;
	}

	if (!sel_init(ccu, &peri->gate, &peri->sel, trig)) {
		pr_err("%s: error initializing selector for %s\n", __func__,
			name);
		return false;
	}

	return true;
}

/*
 * Bus clock operations. These are largely the same as peripheral clocks,
 * so we can re-use the functions.
 */

struct clk_ops kona_bus_clk_ops = {
	.enable = kona_peri_clk_enable,
	.disable = kona_peri_clk_disable,
	.is_enabled = kona_peri_clk_is_enabled,
};

/* Put a bus clock into its initial state */
static bool __bus_clk_init(struct kona_clk *bcm_clk)
{
	struct ccu_data *ccu = bcm_clk->ccu;
	struct clk_reg_data *bus = bcm_clk->u.reg_data;
	const char *name = bcm_clk->init_data.name;

	BUG_ON(bcm_clk->type != bcm_clk_bus);

	if (!gate_init(ccu, &bus->gate)) {
		pr_err("%s: error initializing gate for %s\n", __func__, name);
		return false;
	}
	if (!hyst_init(ccu, &bus->hyst)) {
		pr_err("%s: error initializing hyst for %s\n", __func__, name);
		return false;
	}

	return true;
}

/* PLL clock operations */

/* Reset PLL clock and wait for it to unlock. */
static int pll_reset(struct ccu_data *ccu, struct pll_reg_data *pll)
{
	struct bcm_pll_reset *reset = &pll->reset;
	int ret = 0;
	u32 reg_val;
	u32 i;

	__ccu_write_enable(ccu);

	/* Set post_reset and reset bits at reset offset */
	reg_val = __ccu_read(ccu, reset->offset);
	reg_val |= reset->post_reset_bit | reset->reset_bit;
	__ccu_write(ccu, reset->offset, reg_val);

	/* If the clock is autogated or enabled, wait for unlock */
	reg_val = __ccu_read(ccu, pll->pwrdwn.offset);
	if (pll_is_autogated(pll) || (reg_val & (1 << pll->pwrdwn.pwrdwn_bit)) == 0) {
		i = 0;
		do {
			udelay(1);
			reg_val = __ccu_read(ccu, pll->lock.offset);
			i++;
		} while (!(reg_val & (1 << pll->lock.lock_bit) && i < 1000));

		if (i >= 1000 && !pll_has_delayed_lock(pll))
			ret = -EINVAL;
	}

	__ccu_write_disable(ccu);

	return ret;
}

/* Calculate the rate of the clock based on the provided divider values. */
static unsigned long
compute_pll_rate(struct kona_clk *clk, u32 pdiv, u32 ndiv, u32 nfrac,
		 u32 frac_div, unsigned long xtal_rate)
{
	u64 rate = (u64)xtal_rate * (u64)(ndiv * frac_div + nfrac);
	do_div(rate, pdiv * frac_div);

	return (unsigned long)rate;
}

/*
 * Calculate the divider values required to get the new rate, and rounds
 * down the provided rate to match.
 *
 * Returns the rounded down rate, sets pdiv, ndiv and nfrac to the
 * provided pointers.
 */
static unsigned long
compute_pll_divs(struct kona_clk *clk, unsigned long rate, u32 *pdiv_ptr,
		 u32 *ndiv_ptr, u32 *nfrac_ptr, unsigned long xtal_rate)
{
	struct pll_reg_data *pll = clk->u.pll_reg_data;
	unsigned long calc_rate;
	u32 max_ndiv = 1 << pll->ndiv.width;
	unsigned long tmp1;
	u64 temp_frac;

	u32 frac_div = 1 + (bitfield_mask(pll->nfrac.shift, pll->nfrac.width) \
			    >> pll->nfrac.shift);
	u32 ndiv, nfrac;
	u32 pdiv = 1;

	ndiv = rate / xtal_rate;
	if (ndiv > max_ndiv)
		ndiv = max_ndiv;

	temp_frac = ((u64)rate - (u64)ndiv * xtal_rate) * frac_div;
	do_div(temp_frac, xtal_rate);

	nfrac = (u32)temp_frac;
	nfrac &= (frac_div - 1);

	calc_rate = compute_pll_rate(clk, pdiv, ndiv, nfrac, frac_div,
				     xtal_rate);
	if (calc_rate == rate)
		goto done;

	for (; nfrac < frac_div; nfrac++) {
		calc_rate = compute_pll_rate(clk, pdiv, ndiv, nfrac,
					     frac_div, xtal_rate);

		if (calc_rate > rate) {
			tmp1 = compute_pll_rate(clk, pdiv, ndiv,
					nfrac - 1, frac_div, xtal_rate);
			if (abs(calc_rate - rate) > abs(rate - tmp1))
				nfrac--;
			break;
		};
	}
	calc_rate = compute_pll_rate(clk, pdiv, ndiv, nfrac,
				     frac_div, xtal_rate);
done:
	if (ndiv == max_ndiv)
		ndiv = 0;

	if (ndiv_ptr)
		*ndiv_ptr = ndiv;
	if (nfrac_ptr)
		*nfrac_ptr = nfrac;
	if (pdiv_ptr)
		*pdiv_ptr = pdiv;

	return calc_rate;
}

/* Set the PLL clock offset with values from the desense struct. */
static int desense_set_offset(struct kona_clk *clk, int offset)
{
	struct pll_reg_data *pll = clk->u.pll_reg_data;
	struct bcm_pll_desense *desense = &pll->desense;
	struct ccu_data *ccu = clk->ccu;
	unsigned long curr_rate, new_rate, xtal_rate;
	int offset_rate;
	u32 ndiv_off, nfrac_off;
	u32 ndiv, nfrac;
	u32 pll_offset_val;

	xtal_rate = clk_hw_get_rate(clk_hw_get_parent(&clk->hw));

	if (!desense_flag_enable(desense))
		return 0;

	curr_rate = clk_hw_get_rate(&clk->hw);
	offset_rate = curr_rate + offset;

	new_rate = compute_pll_divs(clk, offset_rate, NULL, &ndiv_off,
				    &nfrac_off, xtal_rate);

	if (abs(new_rate - offset_rate) > 100) {
		pr_err("%s: offset %u not supported for rate %lu",
		       __func__, offset, curr_rate);
		return -EINVAL;
	}

	ndiv = bitfield_extract(__ccu_read(ccu, pll->ndiv.offset),
				pll->ndiv.shift, pll->ndiv.width);
	nfrac = bitfield_extract(__ccu_read(ccu, pll->nfrac.offset),
				pll->nfrac.shift, pll->nfrac.width);

	pll_offset_val = __ccu_read(ccu, desense->offset);

	if (desense_ctrl_ndiv(desense)) {
		pll_offset_val = bitfield_replace(pll_offset_val,
				PLL_OFFSET_NDIV_SHIFT, PLL_OFFSET_NDIV_WIDTH,
				ndiv_off);
	} else if (ndiv != ndiv_off) {
		pr_err("%s: ndiv != ndiv_off, but divider does not handle ndiv",
			__func__);
		return -EINVAL;
	}

	if (desense_ctrl_nfrac(desense)) {
		pll_offset_val = bitfield_replace(pll_offset_val,
				PLL_OFFSET_NFRAC_SHIFT, PLL_OFFSET_NFRAC_WIDTH,
				nfrac_off);
	} else if (nfrac != nfrac_off) {
		pr_err("%s: nfrac != nfrac_off, but divider does not handle nfrac",
			__func__);
		return -EINVAL;
	}

	__ccu_write(ccu, desense->offset, pll_offset_val);

	return 0;
}

/* Initialize the desense offset registers. */
static int desense_init(struct kona_clk *clk)
{
	struct pll_reg_data *pll = clk->u.pll_reg_data;
	struct bcm_pll_desense *desense = &pll->desense;
	struct ccu_data *ccu = clk->ccu;
	u32 reg_val;

	/*
	 * Set PLL desense offset mode; this is either software (1) or
	 * hardware (0).
	 */
	reg_val = __ccu_read(ccu, desense->offset);

	if (desense_flag_enable(desense))
		reg_val |= PLL_OFFSET_MODE_MASK;
	else
		reg_val &= ~PLL_OFFSET_MODE_MASK;

	__ccu_write(ccu, desense->offset, reg_val);

	/* Set PLL offset */
	if (desense_flag_enable(desense)) {
		if (desense_set_offset(clk, desense->delta)) {
			pr_err("%s: failed to set desense offset delta %d\n",
				__func__, desense->delta);
			return -EIO;
		}
	}

	return 0;
}

static int kona_pll_clk_enable(struct clk_hw *hw)
{
	struct kona_clk *bcm_clk = to_kona_clk(hw);
	struct pll_reg_data *pll = bcm_clk->u.pll_reg_data;
	struct bcm_pll_pwrdwn *pwrdwn = &pll->pwrdwn;
	struct ccu_data *ccu = bcm_clk->ccu;
	u32 reg_val;

	/* For autogated clocks, we don't control the enable/disable */
	if (pll_is_autogated(pll))
		return 0;

	__ccu_write_enable(ccu);

	/* For non-autogated clocks, we set the powerdown bit */
	reg_val = __ccu_read(ccu, pwrdwn->offset);
	reg_val &= ~(1 << pwrdwn->pwrdwn_bit);
	__ccu_write(ccu, pwrdwn->offset, reg_val);

	/* Then, we reset the PLL clock and wait for it to unlock */
	pll_reset(ccu, pll);

	__ccu_write_disable(ccu);

	return 0;
}

static void kona_pll_clk_disable(struct clk_hw *hw)
{
	struct kona_clk *bcm_clk = to_kona_clk(hw);
	struct pll_reg_data *pll = bcm_clk->u.pll_reg_data;
	struct bcm_pll_pwrdwn *pwrdwn = &pll->pwrdwn;
	struct ccu_data *ccu = bcm_clk->ccu;
	u32 reg_val;

	/* For autogated clocks, we don't control the enable/disable */
	if (pll_is_autogated(pll))
		return;

	__ccu_write_enable(ccu);

	reg_val = __ccu_read(ccu, pwrdwn->offset);
	reg_val |= (1 << pwrdwn->pwrdwn_bit);
	__ccu_write(ccu, pwrdwn->offset, reg_val);

	__ccu_write_disable(ccu);
}

static int kona_pll_clk_is_enabled(struct clk_hw *hw)
{
	struct kona_clk *bcm_clk = to_kona_clk(hw);
	struct pll_reg_data *pll = bcm_clk->u.pll_reg_data;
	struct bcm_pll_pwrdwn *pwrdwn = &bcm_clk->u.pll_reg_data->pwrdwn;
	struct ccu_data *ccu = bcm_clk->ccu;
	u32 reg_val;

	/* For autogated clocks, we don't control the enable/disable */
	if (pll_is_autogated(pll))
		return true;

	reg_val = __ccu_read(ccu, pwrdwn->offset);
	if (reg_val & (1 << pwrdwn->pwrdwn_bit))
		return false;

	return true;
}

static unsigned long kona_pll_clk_recalc_rate(struct clk_hw *hw,
			unsigned long parent_rate)
{
	struct kona_clk *bcm_clk = to_kona_clk(hw);
	struct pll_reg_data *pll = bcm_clk->u.pll_reg_data;
	struct ccu_data *ccu = bcm_clk->ccu;
	u32 nfrac, pdiv, ndiv, frac_div;
	unsigned long rate;

	pdiv = bitfield_extract(__ccu_read(ccu, pll->pdiv.offset),
				pll->pdiv.shift, pll->pdiv.width);
	ndiv = bitfield_extract(__ccu_read(ccu, pll->ndiv.offset),
				pll->ndiv.shift, pll->ndiv.width);
	nfrac = bitfield_extract(__ccu_read(ccu, pll->nfrac.offset),
				pll->nfrac.shift, pll->nfrac.width);
	frac_div = 1 + bitfield_mask(pll->nfrac.shift, pll->nfrac.width);

	rate = compute_pll_rate(bcm_clk, pdiv, ndiv, nfrac, frac_div,
				parent_rate);

	return rate;
}

static int kona_pll_clk_set_rate(struct clk_hw *hw, unsigned long rate,
			unsigned long parent_rate)
{
	struct kona_clk *bcm_clk = to_kona_clk(hw);
	const char *name = bcm_clk->init_data.name;
	struct pll_reg_data *pll = bcm_clk->u.pll_reg_data;
	struct bcm_pll_cfg *pll_cfg = &pll->cfg;
	struct ccu_data *ccu = bcm_clk->ccu;
	unsigned long xtal_rate;
	u32 pdiv, ndiv, nfrac;
	u32 new_rate;
	u32 reg_val;
	u32 t;

	xtal_rate = clk_hw_get_rate(clk_hw_get_parent(hw));

	/* Round down rate and get pdiv, ndiv, nfrac values */
	new_rate = compute_pll_divs(bcm_clk, (u32)rate, &pdiv, &ndiv, &nfrac,
				    xtal_rate);

	if (abs(new_rate - rate) > 100) {
		pr_err("%s: invalid rate %lu for PLL clock %s\n",
			__func__, rate, name);
		return -EINVAL;
	}

	__ccu_write_enable(ccu);

	/* Set correct PLL config register value for this rate */
	if (pll_cfg_exists(pll_cfg) && pll_cfg->n_tholds) {
		for (t = 0; t < pll_cfg->n_tholds; t++) {
			if (pll_cfg->tholds[t] > new_rate ||
			    pll_cfg->tholds[t] == PLL_CFG_THOLD_MAX) {
				__ccu_write(ccu, pll_cfg->offset,
					pll_cfg->cfg_values[t] << pll_cfg->shift);
				break;
			}
		}
	}

	/* Write nfrac */
	reg_val = __ccu_read(ccu, pll->nfrac.offset);
	reg_val = bitfield_replace(reg_val, pll->nfrac.shift, pll->nfrac.width,
				   nfrac);
	__ccu_write(ccu, pll->nfrac.offset, reg_val);

	/* Write ndiv */
	reg_val = __ccu_read(ccu, pll->ndiv.offset);
	reg_val = bitfield_replace(reg_val, pll->ndiv.shift, pll->ndiv.width,
				   ndiv);
	__ccu_write(ccu, pll->ndiv.offset, reg_val);

	/* Write pdiv */
	reg_val = __ccu_read(ccu, pll->pdiv.offset);
	reg_val = bitfield_replace(reg_val, pll->pdiv.shift, pll->pdiv.width,
				   pdiv);
	__ccu_write(ccu, pll->pdiv.offset, reg_val);

	/* Reset clock and wait for it to unlock */
	pll_reset(ccu, pll);

	__ccu_write_disable(ccu);

	return 0;
}

static long kona_pll_clk_round_rate(struct clk_hw *hw, unsigned long rate,
			unsigned long *parent_rate)
{
	struct kona_clk *bcm_clk = to_kona_clk(hw);
	unsigned long xtal_rate = clk_hw_get_rate(clk_hw_get_parent(hw));
	u32 round_rate;

	round_rate = compute_pll_divs(bcm_clk, (u32)rate, NULL, NULL, NULL,
				      xtal_rate);

	return round_rate;
}

struct clk_ops kona_pll_clk_ops = {
	.enable = kona_pll_clk_enable,
	.disable = kona_pll_clk_disable,
	.is_enabled = kona_pll_clk_is_enabled,
	.set_rate = kona_pll_clk_set_rate,
	.recalc_rate = kona_pll_clk_recalc_rate,
	.round_rate = kona_pll_clk_round_rate,
};

static bool __pll_clk_init(struct kona_clk *bcm_clk)
{
	struct pll_reg_data *pll = bcm_clk->u.pll_reg_data;
	const char *name = bcm_clk->init_data.name;
	struct ccu_data *ccu = bcm_clk->ccu;
	u32 reg_val;

	BUG_ON(bcm_clk->type != bcm_clk_pll);

	/*
	 * If the clock is autogated, we set the idle powerdown override bit,
	 * otherwise we unset it.
	 */
	if (pwrdwn_has_idle_override(&pll->pwrdwn)) {
		reg_val = __ccu_read(ccu, pll->pwrdwn.offset);
		if (pll_is_autogated(pll))
			reg_val |= BIT(pll->pwrdwn.idle_pwrdwn_override_bit);
		else
			reg_val &= ~BIT(pll->pwrdwn.idle_pwrdwn_override_bit);
		__ccu_write(ccu, pll->pwrdwn.offset, reg_val);
	}

	/* If clock has desense, initialize it */
	if (desense_exists(&pll->desense)) {
		if (desense_init(bcm_clk)) {
			pr_err("%s: error initializing desense for %s\n",
				__func__, name);
			return false;
		}
	}

	return true;
}

static int kona_pll_chnl_clk_enable(struct clk_hw *hw)
{
	struct kona_clk *bcm_clk = to_kona_clk(hw);
	struct pll_chnl_reg_data *pll_chnl = bcm_clk->u.pll_chnl_reg_data;
	struct ccu_data *ccu = bcm_clk->ccu;
	u32 reg_val;

	__ccu_write_enable(ccu);

	/* Set enable bit */
	reg_val = __ccu_read(ccu, pll_chnl->enable.offset);
	reg_val |= 1 << (pll_chnl->enable.bit);
	__ccu_write(ccu, pll_chnl->enable.offset, reg_val);

	/* Tell the PLL channel to reload values */
	reg_val = __ccu_read(ccu, pll_chnl->load.offset);
	reg_val |= 1 << (pll_chnl->load.en_bit);
	__ccu_write(ccu, pll_chnl->load.offset, reg_val);

	__ccu_write_disable(ccu);

	return 0;
}

static void kona_pll_chnl_clk_disable(struct clk_hw *hw)
{
	struct kona_clk *bcm_clk = to_kona_clk(hw);
	struct pll_chnl_reg_data *pll_chnl = bcm_clk->u.pll_chnl_reg_data;
	struct ccu_data *ccu = bcm_clk->ccu;
	u32 reg_val;

	__ccu_write_enable(ccu);

	/* Clear enable bit */
	reg_val = __ccu_read(ccu, pll_chnl->enable.offset);
	reg_val &= ~(1 << (pll_chnl->enable.bit));
	__ccu_write(ccu, pll_chnl->enable.offset, reg_val);

	__ccu_write_disable(ccu);
}

static int kona_pll_chnl_clk_is_enabled(struct clk_hw *hw)
{
	struct kona_clk *bcm_clk = to_kona_clk(hw);
	struct pll_chnl_reg_data *pll_chnl = bcm_clk->u.pll_chnl_reg_data;
	struct ccu_data *ccu = bcm_clk->ccu;
	u32 reg_val;

	/*
	 * Get the value of the enable bit; if it's set, the clock is
	 * enabled, otherwise it's disabled.
	 */
	reg_val = __ccu_read(ccu, pll_chnl->enable.offset);
	reg_val &= ~(1 << (pll_chnl->enable.bit));

	return (bool)reg_val;
}

static long kona_pll_chnl_clk_round_rate(struct clk_hw *hw, unsigned long rate,
			     unsigned long *parent_rate)
{
	struct kona_clk *bcm_clk = to_kona_clk(hw);
	struct pll_chnl_reg_data *pll_chnl = bcm_clk->u.pll_chnl_reg_data;
	struct bcm_pll_chnl_mdiv *mdiv_reg = &pll_chnl->mdiv;
	u32 max_mdiv = (1 << mdiv_reg->width) + 1;
	u32 mdiv;

	/* Calculate mdiv value */
	mdiv = (u32)(*parent_rate / rate);
	if (mdiv > max_mdiv)
		mdiv = max_mdiv;

	return *parent_rate / (unsigned long)mdiv;
}

static unsigned long kona_pll_chnl_clk_recalc_rate(struct clk_hw *hw,
			unsigned long parent_rate)
{
	struct kona_clk *bcm_clk = to_kona_clk(hw);
	struct pll_chnl_reg_data *pll_chnl = bcm_clk->u.pll_chnl_reg_data;
	struct bcm_pll_chnl_mdiv *mdiv_reg = &pll_chnl->mdiv;
	struct ccu_data *ccu = bcm_clk->ccu;
	u32 max_mdiv = (1 << mdiv_reg->width) + 1;
	u32 reg_val;
	u32 mdiv;

	/* Get mdiv value */
	reg_val = __ccu_read(ccu, mdiv_reg->offset);
	mdiv = bitfield_extract(reg_val, mdiv_reg->shift, mdiv_reg->width);
	if (mdiv > max_mdiv)
		mdiv = max_mdiv;

	return parent_rate / (unsigned long)mdiv;
}

static int kona_pll_chnl_clk_set_rate(struct clk_hw *hw, unsigned long rate,
			unsigned long parent_rate)
{
	struct kona_clk *bcm_clk = to_kona_clk(hw);
	const char *name = bcm_clk->init_data.name;
	struct pll_chnl_reg_data *pll_chnl = bcm_clk->u.pll_chnl_reg_data;
	struct bcm_pll_chnl_mdiv *mdiv_reg = &pll_chnl->mdiv;
	struct bcm_pll_chnl_load *load = &pll_chnl->load;
	struct ccu_data *ccu = bcm_clk->ccu;
	u32 reg_val;
	u32 mdiv;

	mdiv = (u32)(parent_rate / rate);
	if (abs(rate - parent_rate / mdiv) > 100) {
		pr_err("%s: invalid clock rate %lu for PLL channel clock %s\n",
			__func__, rate, name);
		return -EINVAL;
	}

	__ccu_write_enable(ccu);

	/* Write divider to PLL registers */
	reg_val = __ccu_read(ccu, mdiv_reg->offset);
	reg_val = bitfield_replace(reg_val, mdiv_reg->shift, mdiv_reg->width,
							   mdiv);
	__ccu_write(ccu, mdiv_reg->offset, reg_val);

	/* Tell the PLL channel to load the values */
	reg_val = __ccu_read(ccu, load->offset);
	reg_val |= 1 << load->en_bit;
	__ccu_write(ccu, load->offset, reg_val);

	__ccu_write_disable(ccu);

	return 0;
}

struct clk_ops kona_pll_chnl_clk_ops = {
	.enable = kona_pll_chnl_clk_enable,
	.disable = kona_pll_chnl_clk_disable,
	.is_enabled = kona_pll_chnl_clk_is_enabled,
	.set_rate = kona_pll_chnl_clk_set_rate,
	.recalc_rate = kona_pll_chnl_clk_recalc_rate,
	.round_rate = kona_pll_chnl_clk_round_rate,
};

/*
 * Core clock rates are based on either a static table of rates or the PLL
 * channel rate, depending on the selected frequency ID as written to the
 * CCU frequency policy register.
 *
 * The first 6 policy IDs are used for static rates, and the last 2 are used
 * for the PLL channels. We use the last policy ID here as we derive the
 * frequency from the second PLL channel.
 */

static int kona_core_clk_set_rate(struct clk_hw *hw, unsigned long rate,
			unsigned long parent_rate)
{
	struct kona_clk *bcm_clk = to_kona_clk(hw);
	struct core_reg_data *core = bcm_clk->u.core_reg_data;
	struct bcm_core_policy *policy = &core->policy;
	struct ccu_data *ccu = bcm_clk->ccu;

	struct clk_hw *pll_ch = clk_hw_get_parent(hw);
	struct clk_hw *pll = clk_hw_get_parent(pll_ch);

	/*
	 * Temporarily switch away from using the PLL channel as the clock source
	 * by switching the policy to "economy".
	 */
	kona_ccu_set_freq_policy(ccu, policy->policy, policy->eco_freq_id);

	/* Set PLL frequency to double of the desired rate */
	clk_set_rate(pll->clk, rate * 2);

	/* Set PLL channel frequency to the desired rate */
	clk_set_rate(pll_ch->clk, rate);

	/* Switch policy to "super turbo" to apply the changes */
	kona_ccu_set_freq_policy(ccu, policy->policy, policy->target_freq_id);

	return 0;
}

static unsigned long kona_core_clk_recalc_rate(struct clk_hw *hw,
			unsigned long parent_rate)
{
	/* Same as parent PLL channel */
	return parent_rate;
}

static int kona_core_clk_determine_rate(struct clk_hw *hw,
					struct clk_rate_request *req)
{
	struct clk_hw *current_parent;
	unsigned long parent_rate;

	current_parent = clk_hw_get_parent(hw);
	parent_rate = clk_hw_get_rate(current_parent);

	req->rate = parent_rate;
	return 0;
}

struct clk_ops kona_core_clk_ops = {
	.set_rate = kona_core_clk_set_rate,
	.recalc_rate = kona_core_clk_recalc_rate,
	.determine_rate = kona_core_clk_determine_rate,
};

static bool __core_clk_init(struct kona_clk *bcm_clk)
{
	struct core_reg_data *core = bcm_clk->u.core_reg_data;
	struct bcm_core_policy *policy = &core->policy;
	struct ccu_data *ccu = bcm_clk->ccu;

	kona_ccu_set_freq_policy(ccu, policy->policy, policy->target_freq_id);

	return true;
}

static bool __kona_clk_init(struct kona_clk *bcm_clk)
{
	switch (bcm_clk->type) {
	case bcm_clk_bus:
		return __bus_clk_init(bcm_clk);
	case bcm_clk_core:
		return __core_clk_init(bcm_clk);
	case bcm_clk_peri:
		return __peri_clk_init(bcm_clk);
	case bcm_clk_pll:
		return __pll_clk_init(bcm_clk);
	case bcm_clk_pll_chnl:
		return true; /* No initialization needed */
	default:
		BUG();
	}
	return false;
}

/* Set a CCU and all its clocks into their desired initial state */
bool __init kona_ccu_init(struct ccu_data *ccu)
{
	unsigned long flags;
	unsigned int which;
	struct kona_clk *kona_clks = ccu->kona_clks;
	bool success = true;
	u32 val;

	flags = ccu_lock(ccu);
	__ccu_write_enable(ccu);
	if (ccu_policy_exists(&ccu->policy)) {
		if (!__ccu_policy_engine_stop(ccu))
			pr_err("Could not stop policy engine");
	}

	/* Enable all policies */
	if (ccu_policy_exists(&ccu->policy)) {
		for (which = 0; which < CCU_POLICY_MAX; which++) {
			if (ccu->policy.mask.mask1_offset != 0)
				__ccu_write(ccu, ccu->policy.mask.mask1_offset + (4 * which),
					    CCU_POLICY_ENABLE_ALL);

			if (ccu->policy.mask.mask2_offset != 0)
				__ccu_write(ccu, ccu->policy.mask.mask2_offset + (4 * which),
					    CCU_POLICY_ENABLE_ALL);
		}
	}

	/* Set voltage from voltage tables */
	if (ccu_voltage_exists(&ccu->voltage)) {
		for (which = 0; which < ccu->voltage.voltage_table_len; which++) {
			kona_ccu_set_voltage(ccu, which, ccu->voltage.voltage_table[which]);
		}
	}

	/* Set peripheral voltage from voltage tables */
	if (ccu_peri_volt_exists(&ccu->peri_volt)) {
		for (which = 0; which < ccu->peri_volt.peri_volt_table_len; which++) {
			kona_ccu_set_peri_voltage(ccu, which,
					ccu->peri_volt.peri_volt_table[which]);
		}
	}

	/* Set peripheral voltage from voltage tables */
	if (ccu_freq_policy_exists(&ccu->freq_policy)) {
		for (which = 0; which < ccu->freq_policy.freq_policy_table_len; which++) {
			kona_ccu_set_freq_policy(ccu, which,
					ccu->freq_policy.freq_policy_table[which]);
		}
	}

	if (ccu_policy_exists(&ccu->policy)) {
		__ccu_policy_engine_start(ccu, true);
	}

	/* Disable interrupts by default */
	kona_ccu_interrupt_enable(ccu, CCU_INT_ACT, false);
	kona_ccu_interrupt_enable(ccu, CCU_INT_TGT, false);

	/* Initialize clocks */
	for (which = 0; which < ccu->clk_num; which++) {
		struct kona_clk *bcm_clk = &kona_clks[which];

		if (!bcm_clk->ccu)
			continue;

		success &= __kona_clk_init(bcm_clk);
	}

	/* For BCM21664 ROOT CCU, we need to enable the 8ph pll1 ref clock manually. */
	if (!strcmp(ccu->name, "root_ccu")) {
		pr_info("Need to initialize 8ph pll1");

		val = __ccu_read(ccu, 0x0C3C);
		val |= 0x00800000;
		__ccu_write(ccu, 0x0C3C, val);
	}

	__ccu_write_disable(ccu);
	ccu_unlock(ccu, flags);
	return success;
}
