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
 * ARM64 RTOS Type Definitions
 */

#ifndef RTOS_TYPES_H
#define RTOS_TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* Basic Types */
typedef uint64_t    reg_t;
typedef uint64_t    addr_t;
typedef uint32_t    irq_t;
typedef uint32_t    tick_t;
typedef int32_t     status_t;

/* Status Codes */
#define STATUS_OK           0
#define STATUS_ERROR        (-1)
#define STATUS_TIMEOUT      (-2)
#define STATUS_BUSY         (-3)
#define STATUS_NO_MEM       (-4)
#define STATUS_INVALID      (-5)
#define STATUS_WOULD_BLOCK  (-6)

/* Task States */
typedef enum {
    TASK_STATE_READY = 0,
    TASK_STATE_RUNNING,
    TASK_STATE_BLOCKED,
    TASK_STATE_SUSPENDED,
    TASK_STATE_TERMINATED
} task_state_t;

/* Task Control Block */
typedef struct tcb {
    /* Context - must be first for assembly access */
    reg_t           sp;                 /* Stack pointer */
    reg_t           regs[31];           /* x0-x30 */
    reg_t           elr;                /* Exception link register */
    reg_t           spsr;               /* Saved program status */

    /* Task Info */
    uint32_t        id;
    const char      *name;
    uint8_t         priority;
    task_state_t    state;

    /* Stack */
    void            *stack_base;
    size_t          stack_size;

    /* Scheduling */
    tick_t          wake_tick;
    tick_t          time_slice;
    tick_t          remaining_slice;

    /* Linked List */
    struct tcb      *next;
    struct tcb      *prev;

    /* Blocking */
    void            *wait_obj;
    status_t        wait_result;
} tcb_t;

/* Spinlock */
typedef struct {
    volatile uint32_t lock;
    uint32_t          irq_flags;
} spinlock_t;

#define SPINLOCK_INIT { 0, 0 }

/* Spinlock Functions (defined in scheduler.c) */
extern void spin_lock(spinlock_t *lock);
extern void spin_unlock(spinlock_t *lock);
extern void spin_lock_irq(spinlock_t *lock);
extern void spin_unlock_irq(spinlock_t *lock);

/* Task Functions (defined in scheduler.c) */
extern void task_yield(void);
extern void task_terminate(void);
extern void task_sleep(tick_t ticks);
extern tcb_t *task_current(void);
extern tick_t get_system_ticks(void);

/* Semaphore */
typedef struct {
    volatile int32_t count;
    spinlock_t       lock;
    tcb_t            *wait_list;
} semaphore_t;

/* Mutex */
typedef struct {
    volatile tcb_t  *owner;
    uint32_t        lock_count;
    uint8_t         orig_priority;
    spinlock_t      lock;
    tcb_t           *wait_list;
} mutex_t;

/* Event Flags */
typedef struct {
    volatile uint32_t flags;
    spinlock_t        lock;
    tcb_t             *wait_list;
} event_t;

/* Message Queue */
typedef struct {
    void            *buffer;
    size_t          msg_size;
    size_t          capacity;
    volatile size_t head;
    volatile size_t tail;
    volatile size_t count;
    spinlock_t      lock;
    tcb_t           *send_wait;
    tcb_t           *recv_wait;
} msgqueue_t;

/* Timer */
typedef void (*timer_callback_t)(void *arg);

typedef struct timer {
    tick_t          expire_tick;
    tick_t          period;
    timer_callback_t callback;
    void            *arg;
    bool            active;
    bool            periodic;
    struct timer    *next;
} timer_t;

/* Memory Pool */
typedef struct {
    void            *base;
    size_t          block_size;
    size_t          block_count;
    volatile void   *free_list;
    spinlock_t      lock;
} mempool_t;

/* IRQ Handler */
typedef void (*irq_handler_t)(uint32_t irq, void *arg);

typedef struct {
    irq_handler_t   handler;
    void            *arg;
} irq_entry_t;

/* List Macros */
#define LIST_HEAD_INIT(name) { NULL }

#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

/* Compiler Hints */
#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)
#define ALIGNED(n)      __attribute__((aligned(n)))
#define PACKED          __attribute__((packed))
#define WEAK            __attribute__((weak))
#define NORETURN        __attribute__((noreturn))
#define SECTION(s)      __attribute__((section(s)))
#define USED            __attribute__((used))

/* Memory Barriers */
#define dmb()           __asm__ volatile("dmb sy" ::: "memory")
#define dsb()           __asm__ volatile("dsb sy" ::: "memory")
#define isb()           __asm__ volatile("isb" ::: "memory")

/* Atomic Operations */
static inline uint32_t atomic_load(volatile uint32_t *addr)
{
    uint32_t val;
    __asm__ volatile("ldar %w0, [%1]" : "=r"(val) : "r"(addr) : "memory");
    return val;
}

static inline void atomic_store(volatile uint32_t *addr, uint32_t val)
{
    __asm__ volatile("stlr %w0, [%1]" : : "r"(val), "r"(addr) : "memory");
}

static inline uint32_t atomic_add(volatile uint32_t *addr, uint32_t val)
{
    uint32_t result, tmp;
    __asm__ volatile(
        "1: ldaxr %w0, [%2]\n"
        "   add %w0, %w0, %w3\n"
        "   stlxr %w1, %w0, [%2]\n"
        "   cbnz %w1, 1b"
        : "=&r"(result), "=&r"(tmp)
        : "r"(addr), "r"(val)
        : "memory"
    );
    return result;
}

static inline uint32_t atomic_sub(volatile uint32_t *addr, uint32_t val)
{
    uint32_t result, tmp;
    __asm__ volatile(
        "1: ldaxr %w0, [%2]\n"
        "   sub %w0, %w0, %w3\n"
        "   stlxr %w1, %w0, [%2]\n"
        "   cbnz %w1, 1b"
        : "=&r"(result), "=&r"(tmp)
        : "r"(addr), "r"(val)
        : "memory"
    );
    return result;
}

static inline bool atomic_cas(volatile uint32_t *addr, uint32_t expected, uint32_t desired)
{
    uint32_t tmp;
    __asm__ volatile(
        "1: ldaxr %w0, [%1]\n"
        "   cmp %w0, %w2\n"
        "   b.ne 2f\n"
        "   stlxr %w0, %w3, [%1]\n"
        "   cbnz %w0, 1b\n"
        "   mov %w0, #1\n"
        "   b 3f\n"
        "2: mov %w0, #0\n"
        "3:"
        : "=&r"(tmp)
        : "r"(addr), "r"(expected), "r"(desired)
        : "memory", "cc"
    );
    return tmp != 0;
}

#endif /* RTOS_TYPES_H */
