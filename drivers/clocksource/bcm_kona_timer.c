// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2012 Broadcom Corporation

#include <linux/init.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/jiffies.h>
#include <linux/clockchips.h>
#include <linux/types.h>
#include <linux/clk.h>

#include <linux/io.h>

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>

#define KONA_GPTIMER_STCS_OFFSET			0x00000000
#define KONA_GPTIMER_STCLO_OFFSET			0x00000004
#define KONA_GPTIMER_STCHI_OFFSET			0x00000008
#define KONA_GPTIMER_STCM0_OFFSET			0x0000000C

#define KONA_GPTIMER_STCS_TIMER_MATCH_SHIFT		0
#define KONA_GPTIMER_STCS_COMPARE_ENABLE_SHIFT		4

/*
 * There are 2 timers for Kona (AON and Peripheral), plus Core for the
 * BCM23550, adding up to a potential total of 3.
 */
#define MAX_NUM_TIMERS			3

/* Each timer has 4 channels, each with its own IRQ. */
#define MAX_NUM_CHANNELS		4

/*
 * Trick for storing channel number and timer number in IRQ request devid:
 * We use the first 2 bits to store channel number (0-3), and the rest to
 * store the timer number (0-2).
 */
#define TO_DEVID(timer, channel)	((channel & 0x00ff) + (timer << 2))
#define DEVID_TO_TIMER(dev_id)		(dev_id >> 2)
#define DEVID_TO_CHANNEL(dev_id)	(dev_id & 0x00ff)

struct kona_bcm_timer_channel {
	unsigned int timer_id; /* Number of parent timer in the timers struct */
	unsigned int id; /* Number of channel, from 0 to 3 */

	bool has_clockevent;
	struct clock_event_device clockevent;
};

struct kona_bcm_timer {
	char *name;

	unsigned int num_channels;
	unsigned int channel_irqs[MAX_NUM_CHANNELS]; /* Map channel number to IRQ */
	unsigned int rate;

	bool has_gptimer; /* Whether this timer is used for the GP timer */
	u32 system_timer_channel; /* The channel used for the GP timer */

	struct kona_bcm_timer_channel channels[MAX_NUM_CHANNELS];

	void __iomem *base;
};

static struct kona_bcm_timer *timers[MAX_NUM_TIMERS];
static int num_timers = 0; /* Count of currently initialized timers */
static int system_timer_num = -1; /* Index of timer used as GP timer */

static inline struct kona_bcm_timer *
channel_to_timer(struct kona_bcm_timer_channel *channel)
{
	return timers[channel->timer_id];
}

static inline struct kona_bcm_timer_channel *
clockevent_to_channel(struct clock_event_device *evt)
{
	return container_of(evt, struct kona_bcm_timer_channel, clockevent);
}

/*
 * We use the peripheral timers for system tick, the cpu global timer for
 * profile tick
 */
static void kona_timer_disable_and_clear(void __iomem *base, int channel_num)
{
	uint32_t reg;

	/*
	 * clear and disable interrupts
	 * We are using compare/match register 0 for our system interrupts
	 */
	reg = readl(base + KONA_GPTIMER_STCS_OFFSET);

	/* Clear compare (0) interrupt */
	reg |= 1 << (channel_num + KONA_GPTIMER_STCS_TIMER_MATCH_SHIFT);
	/* disable compare */
	reg &= ~(1 << (channel_num + KONA_GPTIMER_STCS_COMPARE_ENABLE_SHIFT));

	writel(reg, base + KONA_GPTIMER_STCS_OFFSET);

}

static int
kona_timer_get_counter(void __iomem *timer_base, uint32_t *msw, uint32_t *lsw)
{
	int loop_limit = 3;

	/*
	 * Read 64-bit free running counter
	 * 1. Read hi-word
	 * 2. Read low-word
	 * 3. Read hi-word again
	 * 4.1
	 *      if new hi-word is not equal to previously read hi-word, then
	 *      start from #1
	 * 4.2
	 *      if new hi-word is equal to previously read hi-word then stop.
	 */

	do {
		*msw = readl(timer_base + KONA_GPTIMER_STCHI_OFFSET);
		*lsw = readl(timer_base + KONA_GPTIMER_STCLO_OFFSET);
		if (*msw == readl(timer_base + KONA_GPTIMER_STCHI_OFFSET))
			break;
	} while (--loop_limit);
	if (!loop_limit) {
		pr_err("kona-timer: getting counter failed, timer will "
		       "be impacted\n");
		return -ETIMEDOUT;
	}

	return 0;
}

static int kona_timer_set_next_event(unsigned long clc,
				  struct clock_event_device *evt)
{
	/*
	 * timer (0) is disabled by the timer interrupt already
	 * so, here we reload the next event value and re-enable
	 * the timer.
	 *
	 * This way, we are potentially losing the time between
	 * timer-interrupt->set_next_event. CPU local timers, when
	 * they come in should get rid of skew.
	 */

	struct kona_bcm_timer_channel *channel = clockevent_to_channel(evt);
	struct kona_bcm_timer *timer = channel_to_timer(channel);
	uint32_t lsw, msw;
	uint32_t reg;
	int ret;

	ret = kona_timer_get_counter(timer->base, &msw, &lsw);
	if (ret)
		return ret;

	/* Load the "next" event tick value */
	writel(lsw + clc, timer->base + KONA_GPTIMER_STCM0_OFFSET);

	/* Enable compare */
	reg = readl(timer->base + KONA_GPTIMER_STCS_OFFSET);
	reg |= (1 << (channel->id + KONA_GPTIMER_STCS_COMPARE_ENABLE_SHIFT));
	writel(reg, timer->base + KONA_GPTIMER_STCS_OFFSET);

	return 0;
}

static int kona_timer_shutdown(struct clock_event_device *evt)
{
	struct kona_bcm_timer_channel *channel = clockevent_to_channel(evt);
	struct kona_bcm_timer *timer = channel_to_timer(channel);
	kona_timer_disable_and_clear(timer->base, channel->id);
	return 0;
}

static void __init
kona_timer_clockevents_init(struct kona_bcm_timer *timer,
				struct kona_bcm_timer_channel *channel)
{
	channel->clockevent.name = "system timer";
	channel->clockevent.features = CLOCK_EVT_FEAT_ONESHOT;
	channel->clockevent.set_next_event = kona_timer_set_next_event;
	channel->clockevent.set_state_shutdown = kona_timer_shutdown;
	channel->clockevent.tick_resume = kona_timer_shutdown;
	channel->clockevent.cpumask = cpumask_of(0);

	channel->has_clockevent = true;

	clockevents_config_and_register(&channel->clockevent,
		timer->rate, 6, 0xffffffff);
}

static irqreturn_t kona_timer_interrupt(int irq, void *dev_id)
{
	struct kona_bcm_timer *timer = timers[DEVID_TO_TIMER((int)dev_id)];
	struct kona_bcm_timer_channel *channel = \
		&timer->channels[DEVID_TO_CHANNEL((int)dev_id)];

	kona_timer_disable_and_clear(timer->base, channel->id);
	if (channel->has_clockevent)
		channel->clockevent.event_handler(&channel->clockevent);
	return IRQ_HANDLED;
}

static int __init kona_timer_init(struct device_node *node)
{
	struct kona_bcm_timer *timer;
	struct clk *external_clk;
	u32 system_timer_channel;
	u32 freq;
	unsigned int i;

	if ((num_timers + 1) >= MAX_NUM_TIMERS) {
		pr_err("kona-timer: exceeded maximum number of timers (%d)\n",
			MAX_NUM_TIMERS);
		return -EINVAL;
	}

	timer = kzalloc(struct_size(timer, channels, MAX_NUM_CHANNELS),
			GFP_KERNEL);
	if (!timer)
		return -ENOMEM;

	external_clk = of_clk_get_by_name(node, NULL);

	if (!IS_ERR(external_clk)) {
		timer->rate = clk_get_rate(external_clk);
		clk_prepare_enable(external_clk);
	} else if (!of_property_read_u32(node, "clock-frequency", &freq)) {
		timer->rate = freq;
	} else {
		pr_err("kona-timer: unable to determine clock-frequency\n");
		goto err_free_timer;
	}

	/*
	 * Setup IRQs for all channels. Each channel has 1 IRQ, so we can
	 * guess the defined channel count just by looking at how many IRQs
	 * are defined.
	 */
	timer->num_channels = of_irq_count(node);
	BUG_ON(timer->num_channels > MAX_NUM_CHANNELS);

	pr_info("kona-timer: timer %d, %d channels", num_timers, timer->num_channels);

	for (i = 0; i < timer->num_channels; i++) {
		timer->channel_irqs[i] = irq_of_parse_and_map(node, i);
		if (request_irq(timer->channel_irqs[i], kona_timer_interrupt,
				IRQF_TIMER, "Kona Timer Tick", (void *)TO_DEVID(num_timers, i)))
			pr_err("kona-timer: request_irq() failed\n");

		/* Set up channel struct */
		timer->channels[i].id = i;
		timer->channels[i].timer_id = num_timers;
		timer->channels[i].has_clockevent = false;
	}

	/* Setup IO address */
	timer->base = of_iomap(node, 0);

	/*
	 * Add the timer pointer to the list of timers. We need to do this here
	 * as this pointer needs to be available for the system timer
	 * initialization functions.
	 */
	timers[num_timers] = timer;

	/*
	 * Get information about the channel used for the system timer. We do
	 * this by checking for the "brcm,kona-system-timer-channel" property.
	 */
	if (!of_property_read_u32(node, "brcm,kona-system-timer-channel",
				&system_timer_channel)) {
		pr_info("kona-timer: found system timer at timer %d channel %d",
				num_timers, system_timer_channel);
		if (system_timer_num > -1) {
			pr_warn("kona-timer: system timer has already been "
			        "initialized for timer ID %d, ignoring",
				system_timer_num);
		} else {
			timer->has_gptimer = true;
			timer->system_timer_channel = system_timer_channel;
		}
	} else {
		timer->has_gptimer = false;
		timer->system_timer_channel = 0;
	}

	/* Initialize the timer */
	for (i = 0; i < timer->num_channels; i++) {
		kona_timer_disable_and_clear(timer->base, i);
		kona_timer_clockevents_init(timer,
					&timer->channels[i]);
		kona_timer_set_next_event((timer->rate / HZ),
					&timer->channels[i].clockevent);
	};

	num_timers++;

	return 0;
err_free_timer:
	kfree(timer);
	return -EINVAL;
}

TIMER_OF_DECLARE(brcm_kona, "brcm,kona-timer", kona_timer_init);
/*
 * bcm,kona-timer is deprecated by brcm,kona-timer
 * being kept here for driver compatibility
 */
TIMER_OF_DECLARE(bcm_kona, "bcm,kona-timer", kona_timer_init);
