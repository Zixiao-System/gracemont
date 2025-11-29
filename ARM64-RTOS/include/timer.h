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
 * Timer Driver Header
 */

#ifndef TIMER_H
#define TIMER_H

#include "rtos_types.h"

/*
 * Timer Callback
 */
typedef void (*timer_callback_t)(void *arg);

/*
 * Driver Initialization
 */
status_t timer_driver_init(void);

/*
 * System Tick Functions
 */
tick_t get_system_ticks(void);
void timer_tick_handler(void);

/*
 * High-Resolution Timestamps
 */
uint64_t timer_get_ns(void);
uint64_t timer_get_us(void);
uint64_t timer_get_ms(void);

/*
 * Busy-Wait Delays
 */
void timer_delay_us(uint32_t us);
void timer_delay_ms(uint32_t ms);

/*
 * Software Timer API
 */
void timer_init(timer_t *timer, timer_callback_t callback, void *arg);
status_t timer_start(timer_t *timer, tick_t delay, bool periodic);
void timer_stop(timer_t *timer);
bool timer_is_active(timer_t *timer);
tick_t timer_remaining(timer_t *timer);

/*
 * Timer Information
 */
uint64_t timer_get_frequency(void);
void timer_calibrate(void);

/*
 * Time Conversion Macros
 */
#define TIMER_MS_TO_TICKS(ms)   ((tick_t)(ms) * CONFIG_TICK_RATE_HZ / 1000)
#define TIMER_TICKS_TO_MS(t)    ((uint32_t)(t) * 1000 / CONFIG_TICK_RATE_HZ)
#define TIMER_US_TO_TICKS(us)   ((tick_t)(us) * CONFIG_TICK_RATE_HZ / 1000000)
#define TIMER_TICKS_TO_US(t)    ((uint64_t)(t) * 1000000 / CONFIG_TICK_RATE_HZ)

#endif /* TIMER_H */
