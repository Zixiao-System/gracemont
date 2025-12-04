/*
 * Gracemont X86_64 RTOS - Core Type Definitions
 * Copyright (C) 2024 Zixiao System
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef RTOS_TYPES_H
#define RTOS_TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* ============================================================================
 * Basic Types
 * ============================================================================ */

typedef uint64_t    reg_t;          /* Register size */
typedef uint64_t    addr_t;         /* Address type */
typedef uint32_t    irq_t;          /* IRQ number */
typedef uint64_t    tick_t;         /* System tick count */
typedef int32_t     status_t;       /* Status/error code */

/* ============================================================================
 * Status Codes
 * ============================================================================ */

#define STATUS_OK           0
#define STATUS_ERROR        (-1)
#define STATUS_TIMEOUT      (-2)
#define STATUS_BUSY         (-3)
#define STATUS_NO_MEM       (-4)
#define STATUS_INVALID      (-5)
#define STATUS_NOT_FOUND    (-6)
#define STATUS_WOULD_BLOCK  (-7)

/* ============================================================================
 * Task States
 * ============================================================================ */

typedef enum {
    TASK_STATE_READY = 0,
    TASK_STATE_RUNNING,
    TASK_STATE_BLOCKED,
    TASK_STATE_SUSPENDED,
    TASK_STATE_TERMINATED
} task_state_t;

/* ============================================================================
 * Task Control Block (TCB)
 * ============================================================================ */

typedef struct tcb {
    /* Context - must be at the beginning for assembly access */
    reg_t           sp;             /* Stack pointer [offset 0] */
    reg_t           rbx;            /* Callee-saved registers [offset 8] */
    reg_t           rbp;            /* [offset 16] */
    reg_t           r12;            /* [offset 24] */
    reg_t           r13;            /* [offset 32] */
    reg_t           r14;            /* [offset 40] */
    reg_t           r15;            /* [offset 48] */
    reg_t           rip;            /* Return address [offset 56] */

    /* Task information */
    uint32_t        id;
    const char      *name;
    uint8_t         priority;       /* 0 = highest priority */
    task_state_t    state;

    /* Stack */
    void            *stack_base;
    size_t          stack_size;

    /* Scheduling */
    tick_t          wake_tick;      /* When to wake from sleep */
    tick_t          remaining_slice; /* Remaining time slice */

    /* Linked list pointers */
    struct tcb      *next;
    struct tcb      *prev;

    /* Blocking */
    void            *wait_obj;      /* Object being waited on */
    status_t        wait_result;    /* Result of wait operation */

    /* Statistics */
    uint64_t        total_ticks;    /* Total ticks consumed */
    uint64_t        switches;       /* Context switch count */
} tcb_t;

/* ============================================================================
 * Spinlock (x86_64 implementation)
 * ============================================================================ */

typedef struct {
    volatile uint32_t lock;
    uint64_t          irq_flags;
} spinlock_t;

#define SPINLOCK_INIT { .lock = 0, .irq_flags = 0 }

/* ============================================================================
 * Synchronization Primitives
 * ============================================================================ */

typedef struct {
    volatile int32_t count;
    spinlock_t      lock;
    tcb_t           *wait_list;
} semaphore_t;

typedef struct {
    volatile uint32_t locked;
    tcb_t           *owner;
    spinlock_t      lock;
    tcb_t           *wait_list;
    uint32_t        recursion;
} mutex_t;

typedef struct {
    volatile uint32_t flags;
    spinlock_t      lock;
    tcb_t           *wait_list;
} event_t;

typedef struct {
    void            *buffer;
    size_t          msg_size;
    size_t          max_msgs;
    size_t          head;
    size_t          tail;
    size_t          count;
    spinlock_t      lock;
    tcb_t           *send_wait_list;
    tcb_t           *recv_wait_list;
} msgq_t;

/* ============================================================================
 * Timer
 * ============================================================================ */

typedef void (*timer_callback_t)(void *arg);

typedef struct timer {
    tick_t          expire_tick;
    tick_t          period;
    timer_callback_t callback;
    void            *arg;
    bool            active;
    bool            periodic;
    struct timer    *next;
    struct timer    *prev;
} timer_t;

/* ============================================================================
 * IRQ Handler
 * ============================================================================ */

typedef void (*irq_handler_t)(uint32_t irq, void *arg);

typedef struct {
    irq_handler_t   handler;
    void            *arg;
} irq_entry_t;

/* ============================================================================
 * Memory Pool
 * ============================================================================ */

typedef struct {
    void            *base;
    void            *pool;
    size_t          block_size;
    size_t          block_count;
    void            *free_list;
    spinlock_t      lock;
} mempool_t;

/* ============================================================================
 * x86_64 Memory Barriers
 * ============================================================================ */

#define dmb()           __asm__ volatile("mfence" ::: "memory")
#define dsb()           __asm__ volatile("mfence" ::: "memory")
#define isb()           __asm__ volatile("" ::: "memory")
#define barrier()       __asm__ volatile("" ::: "memory")

#define rmb()           __asm__ volatile("lfence" ::: "memory")
#define wmb()           __asm__ volatile("sfence" ::: "memory")

/* ============================================================================
 * x86_64 Atomic Operations
 * ============================================================================ */

static inline uint32_t atomic_load(volatile uint32_t *addr)
{
    return __atomic_load_n(addr, __ATOMIC_SEQ_CST);
}

static inline void atomic_store(volatile uint32_t *addr, uint32_t val)
{
    __atomic_store_n(addr, val, __ATOMIC_SEQ_CST);
}

static inline uint32_t atomic_add(volatile uint32_t *addr, uint32_t val)
{
    return __atomic_fetch_add(addr, val, __ATOMIC_SEQ_CST);
}

static inline uint32_t atomic_sub(volatile uint32_t *addr, uint32_t val)
{
    return __atomic_fetch_sub(addr, val, __ATOMIC_SEQ_CST);
}

static inline bool atomic_cas(volatile uint32_t *addr, uint32_t expected, uint32_t desired)
{
    return __atomic_compare_exchange_n(addr, &expected, desired, false,
                                       __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

static inline uint32_t atomic_xchg(volatile uint32_t *addr, uint32_t val)
{
    return __atomic_exchange_n(addr, val, __ATOMIC_SEQ_CST);
}

static inline uint64_t atomic_load64(volatile uint64_t *addr)
{
    return __atomic_load_n(addr, __ATOMIC_SEQ_CST);
}

static inline void atomic_store64(volatile uint64_t *addr, uint64_t val)
{
    __atomic_store_n(addr, val, __ATOMIC_SEQ_CST);
}

/* ============================================================================
 * x86_64 Spinlock Operations
 * ============================================================================ */

static inline uint64_t arch_irq_save(void)
{
    uint64_t flags;
    __asm__ volatile("pushfq; popq %0; cli" : "=r"(flags) :: "memory");
    return flags;
}

static inline void arch_irq_restore(uint64_t flags)
{
    __asm__ volatile("pushq %0; popfq" :: "r"(flags) : "memory", "cc");
}

static inline void spin_lock(spinlock_t *lock)
{
    while (__atomic_exchange_n(&lock->lock, 1, __ATOMIC_ACQUIRE)) {
        while (__atomic_load_n(&lock->lock, __ATOMIC_RELAXED)) {
            __asm__ volatile("pause" ::: "memory");
        }
    }
}

static inline void spin_unlock(spinlock_t *lock)
{
    __atomic_store_n(&lock->lock, 0, __ATOMIC_RELEASE);
}

static inline void spin_lock_irq(spinlock_t *lock)
{
    lock->irq_flags = arch_irq_save();
    spin_lock(lock);
}

static inline void spin_unlock_irq(spinlock_t *lock)
{
    spin_unlock(lock);
    arch_irq_restore(lock->irq_flags);
}

/* ============================================================================
 * Utility Macros
 * ============================================================================ */

#define ARRAY_SIZE(a)       (sizeof(a) / sizeof((a)[0]))
#define ALIGN_UP(x, a)      (((x) + (a) - 1) & ~((a) - 1))
#define ALIGN_DOWN(x, a)    ((x) & ~((a) - 1))
#define MIN(a, b)           ((a) < (b) ? (a) : (b))
#define MAX(a, b)           ((a) > (b) ? (a) : (b))

#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

#define UNUSED(x)           ((void)(x))

/* Packed attribute */
#define PACKED              __attribute__((packed))
#define ALIGNED(n)          __attribute__((aligned(n)))
#define NORETURN            __attribute__((noreturn))
#define WEAK                __attribute__((weak))

#endif /* RTOS_TYPES_H */
