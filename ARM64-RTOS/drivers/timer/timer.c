/*
 * Gracemont Industrial Control Framework - ARM64 RTOS
 * Copyright (C) 2024 Zixiao System <https://github.com/Zixiao-System>
 *
 * This file is part of Gracemont Industrial Control Framework.
 * Repository: https://github.com/Zixiao-System/gracemont
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * ARM64 Generic Timer Driver
 */

#include "timer.h"
#include "rtos_config.h"
#include "rtos.h"

/*
 * ARM Generic Timer Registers (accessed via system registers)
 *
 * CNTFRQ_EL0    - Counter Frequency
 * CNTVCT_EL0    - Virtual Counter Value
 * CNTV_CVAL_EL0 - Virtual Timer Compare Value
 * CNTV_TVAL_EL0 - Virtual Timer Timer Value
 * CNTV_CTL_EL0  - Virtual Timer Control
 */

/*
 * Timer Control Register bits
 */
#define TIMER_CTL_ENABLE    (1 << 0)    /* Timer Enable */
#define TIMER_CTL_IMASK     (1 << 1)    /* Interrupt Mask (1=masked) */
#define TIMER_CTL_ISTATUS   (1 << 2)    /* Interrupt Status */

/*
 * Software Timer Context
 */
typedef struct sw_timer {
    struct sw_timer *next;
    timer_callback_t callback;
    void            *arg;
    tick_t          expire_time;
    tick_t          period;
    bool            periodic;
    bool            active;
} sw_timer_t;

/*
 * Timer Driver State
 */
static struct {
    uint64_t        freq;           /* Counter frequency */
    uint64_t        ticks_per_tick; /* Counter ticks per RTOS tick */
    volatile tick_t system_ticks;   /* RTOS system tick counter */

    /* Software timer list */
    sw_timer_t      *timer_list;
    spinlock_t      lock;

    /* High-resolution timestamps */
    uint64_t        epoch;          /* Epoch offset for absolute time */

    bool            initialized;
} timer_state;

/*
 * Read counter frequency
 */
static inline uint64_t read_cntfrq(void)
{
    uint64_t freq;
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(freq));
    return freq;
}

/*
 * Read virtual counter
 */
static inline uint64_t read_cntvct(void)
{
    uint64_t cnt;
    __asm__ volatile("mrs %0, cntvct_el0" : "=r"(cnt));
    return cnt;
}

/*
 * Read virtual timer control
 */
static inline uint32_t read_cntv_ctl(void)
{
    uint32_t ctl;
    __asm__ volatile("mrs %0, cntv_ctl_el0" : "=r"(ctl));
    return ctl;
}

/*
 * Write virtual timer control
 */
static inline void write_cntv_ctl(uint32_t ctl)
{
    __asm__ volatile("msr cntv_ctl_el0, %0" : : "r"((uint64_t)ctl));
}

/*
 * Read virtual timer compare value
 */
static inline uint64_t read_cntv_cval(void)
{
    uint64_t cval;
    __asm__ volatile("mrs %0, cntv_cval_el0" : "=r"(cval));
    return cval;
}

/*
 * Write virtual timer compare value
 */
static inline void write_cntv_cval(uint64_t cval)
{
    __asm__ volatile("msr cntv_cval_el0, %0" : : "r"(cval));
}

/*
 * Write virtual timer value (countdown)
 */
static inline void write_cntv_tval(uint32_t tval)
{
    __asm__ volatile("msr cntv_tval_el0, %0" : : "r"((uint64_t)tval));
}

/*
 * Process software timers
 */
static void process_sw_timers(void)
{
    tick_t now = timer_state.system_ticks;

    spin_lock(&timer_state.lock);

    sw_timer_t **pp = &timer_state.timer_list;

    while (*pp) {
        sw_timer_t *timer = *pp;

        if (timer->active && timer->expire_time <= now) {
            /* Timer expired */
            timer_callback_t callback = timer->callback;
            void *arg = timer->arg;

            if (timer->periodic && timer->period > 0) {
                /* Reschedule periodic timer */
                timer->expire_time = now + timer->period;
                pp = &timer->next;
            } else {
                /* Remove one-shot timer */
                timer->active = false;
                *pp = timer->next;
            }

            spin_unlock(&timer_state.lock);

            /* Call callback outside lock */
            if (callback) {
                callback(arg);
            }

            spin_lock(&timer_state.lock);
            continue;
        }

        pp = &timer->next;
    }

    spin_unlock(&timer_state.lock);
}

/*
 * System Tick Handler (called from scheduler)
 */
void timer_tick_handler(void)
{
    timer_state.system_ticks++;
    process_sw_timers();
}

/*
 * Timer IRQ Handler
 */
static void timer_irq_handler(uint32_t irq, void *arg)
{
    (void)arg;

    /* Acknowledge by setting next compare value */
    uint64_t cval = read_cntv_cval();
    cval += timer_state.ticks_per_tick;
    write_cntv_cval(cval);

    /* Call scheduler tick */
    extern void scheduler_tick(void);
    scheduler_tick();

    /* Process software timers */
    timer_tick_handler();
}

/*
 * Initialize Timer Driver
 */
status_t timer_driver_init(void)
{
    if (timer_state.initialized) {
        return STATUS_OK;
    }

    /* Get counter frequency */
    timer_state.freq = read_cntfrq();
    if (timer_state.freq == 0) {
        /* QEMU may not set this, use configured value */
        timer_state.freq = CONFIG_CPU_FREQ_HZ;
    }

    /* Calculate ticks per RTOS tick */
    timer_state.ticks_per_tick = timer_state.freq / CONFIG_TICK_RATE_HZ;

    /* Initialize state */
    timer_state.system_ticks = 0;
    timer_state.timer_list = NULL;
    timer_state.lock = (spinlock_t)SPINLOCK_INIT;
    timer_state.epoch = 0;

    /* Disable timer */
    write_cntv_ctl(0);

    /* Set compare value for first tick */
    uint64_t cnt = read_cntvct();
    write_cntv_cval(cnt + timer_state.ticks_per_tick);

    /* Register IRQ handler */
    irq_register(CONFIG_TIMER_IRQ, timer_irq_handler, NULL);
    irq_enable(CONFIG_TIMER_IRQ);

    /* Enable timer */
    write_cntv_ctl(TIMER_CTL_ENABLE);

    timer_state.initialized = true;

    return STATUS_OK;
}

/*
 * Get System Ticks
 */
tick_t get_system_ticks(void)
{
    return timer_state.system_ticks;
}

/*
 * Get High-Resolution Timestamp (nanoseconds)
 */
uint64_t timer_get_ns(void)
{
    uint64_t cnt = read_cntvct();
    /* Convert to nanoseconds: cnt * 1e9 / freq */
    /* Avoid overflow by dividing first if freq > 1e9 */
    if (timer_state.freq > 1000000000ULL) {
        return cnt / (timer_state.freq / 1000000000ULL);
    } else {
        return (cnt * 1000000000ULL) / timer_state.freq;
    }
}

/*
 * Get High-Resolution Timestamp (microseconds)
 */
uint64_t timer_get_us(void)
{
    uint64_t cnt = read_cntvct();
    return (cnt * 1000000ULL) / timer_state.freq;
}

/*
 * Get High-Resolution Timestamp (milliseconds)
 */
uint64_t timer_get_ms(void)
{
    uint64_t cnt = read_cntvct();
    return (cnt * 1000ULL) / timer_state.freq;
}

/*
 * Delay in microseconds (busy wait)
 */
void timer_delay_us(uint32_t us)
{
    uint64_t start = read_cntvct();
    uint64_t target = start + ((uint64_t)us * timer_state.freq) / 1000000ULL;

    while (read_cntvct() < target) {
        /* Busy wait */
        __asm__ volatile("yield");
    }
}

/*
 * Delay in milliseconds (busy wait)
 */
void timer_delay_ms(uint32_t ms)
{
    timer_delay_us(ms * 1000);
}

/*
 * Initialize Software Timer
 */
void timer_init(timer_t *timer, timer_callback_t callback, void *arg)
{
    if (!timer) return;

    sw_timer_t *sw = (sw_timer_t *)timer;
    sw->callback = callback;
    sw->arg = arg;
    sw->expire_time = 0;
    sw->period = 0;
    sw->periodic = false;
    sw->active = false;
    sw->next = NULL;
}

/*
 * Start Software Timer
 */
status_t timer_start(timer_t *timer, tick_t delay, bool periodic)
{
    if (!timer || delay == 0) {
        return STATUS_INVALID;
    }

    sw_timer_t *sw = (sw_timer_t *)timer;

    spin_lock(&timer_state.lock);

    /* Remove from list if already active */
    if (sw->active) {
        sw_timer_t **pp = &timer_state.timer_list;
        while (*pp) {
            if (*pp == sw) {
                *pp = sw->next;
                break;
            }
            pp = &(*pp)->next;
        }
    }

    /* Configure timer */
    sw->expire_time = timer_state.system_ticks + delay;
    sw->period = periodic ? delay : 0;
    sw->periodic = periodic;
    sw->active = true;

    /* Add to list (sorted by expire time) */
    sw_timer_t **pp = &timer_state.timer_list;
    while (*pp && (*pp)->expire_time <= sw->expire_time) {
        pp = &(*pp)->next;
    }
    sw->next = *pp;
    *pp = sw;

    spin_unlock(&timer_state.lock);

    return STATUS_OK;
}

/*
 * Stop Software Timer
 */
void timer_stop(timer_t *timer)
{
    if (!timer) return;

    sw_timer_t *sw = (sw_timer_t *)timer;

    spin_lock(&timer_state.lock);

    if (sw->active) {
        sw->active = false;

        /* Remove from list */
        sw_timer_t **pp = &timer_state.timer_list;
        while (*pp) {
            if (*pp == sw) {
                *pp = sw->next;
                break;
            }
            pp = &(*pp)->next;
        }
    }

    spin_unlock(&timer_state.lock);
}

/*
 * Check if timer is active
 */
bool timer_is_active(timer_t *timer)
{
    if (!timer) return false;
    return ((sw_timer_t *)timer)->active;
}

/*
 * Get remaining time
 */
tick_t timer_remaining(timer_t *timer)
{
    if (!timer) return 0;

    sw_timer_t *sw = (sw_timer_t *)timer;
    if (!sw->active) return 0;

    tick_t now = timer_state.system_ticks;
    if (sw->expire_time <= now) return 0;

    return sw->expire_time - now;
}

/*
 * Get timer frequency
 */
uint64_t timer_get_frequency(void)
{
    return timer_state.freq;
}

/*
 * Calibrate timer (for accurate timing)
 */
void timer_calibrate(void)
{
    /* Read frequency from hardware */
    uint64_t freq = read_cntfrq();
    if (freq > 0 && freq != timer_state.freq) {
        timer_state.freq = freq;
        timer_state.ticks_per_tick = freq / CONFIG_TICK_RATE_HZ;
    }
}
