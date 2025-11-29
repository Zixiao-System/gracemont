/*
 * Gracemont Industrial Control Framework
 * ARM64 RTOS - Master Header
 */

#ifndef RTOS_H
#define RTOS_H

#include "rtos_config.h"
#include "rtos_types.h"

/*
 * Kernel API
 */

/* Task Management */
extern status_t task_create(tcb_t *tcb, const char *name, void (*entry)(void *),
                            void *arg, uint8_t priority, void *stack, size_t stack_size);
extern status_t task_start(tcb_t *tcb);
extern void task_terminate(void);
extern void task_yield(void);
extern void task_sleep(tick_t ticks);
extern tcb_t *task_current(void);

/* Scheduler */
extern void scheduler_start(void);
extern void scheduler_tick(void);
extern tick_t get_system_ticks(void);

/* Spinlock */
extern void spin_lock(spinlock_t *lock);
extern void spin_unlock(spinlock_t *lock);
extern void spin_lock_irq(spinlock_t *lock);
extern void spin_unlock_irq(spinlock_t *lock);

/* Semaphore */
extern void sem_init(semaphore_t *sem, int32_t initial);
extern status_t sem_wait(semaphore_t *sem);
extern status_t sem_trywait(semaphore_t *sem);
extern void sem_post(semaphore_t *sem);

/* Mutex */
extern void mutex_init(mutex_t *mutex);
extern status_t mutex_lock(mutex_t *mutex);
extern status_t mutex_trylock(mutex_t *mutex);
extern void mutex_unlock(mutex_t *mutex);

/* Event Flags */
extern void event_init(event_t *event);
extern status_t event_wait(event_t *event, uint32_t mask, uint32_t *flags_out,
                           bool wait_all, bool clear);
extern void event_set(event_t *event, uint32_t mask);
extern void event_clear(event_t *event, uint32_t mask);

/* Message Queue */
extern status_t msgq_init(msgqueue_t *mq, void *buffer, size_t msg_size, size_t capacity);
extern status_t msgq_send(msgqueue_t *mq, const void *msg, tick_t timeout);
extern status_t msgq_recv(msgqueue_t *mq, void *msg, tick_t timeout);

/* Timer */
extern void timer_init(timer_t *timer, timer_callback_t callback, void *arg);
extern status_t timer_start(timer_t *timer, tick_t delay, bool periodic);
extern void timer_stop(timer_t *timer);

/* Memory */
extern void heap_init(void);
extern void *heap_alloc(size_t size);
extern void heap_free(void *ptr);
extern void *heap_alloc_aligned(size_t size, size_t alignment);
extern void heap_free_aligned(void *ptr);
extern status_t mempool_init(mempool_t *pool, void *base, size_t block_size, size_t block_count);
extern void *mempool_alloc(mempool_t *pool);
extern void mempool_free(mempool_t *pool, void *block);
extern void dma_pool_init(void);
extern void *dma_alloc(size_t size);
extern void dma_free(void *ptr, size_t size);

/* IRQ */
extern uint64_t arch_irq_save(void);
extern void arch_irq_restore(uint64_t flags);
extern void arch_irq_enable(void);
extern void arch_irq_disable(void);
extern bool arch_irq_enabled(void);
extern void gic_init(void);
extern void irq_enable(uint32_t irq);
extern void irq_disable(uint32_t irq);
extern void irq_set_priority(uint32_t irq, uint8_t priority);
extern status_t irq_register(uint32_t irq, irq_handler_t handler, void *arg);
extern status_t irq_unregister(uint32_t irq);
extern bool in_irq_context(void);

/* Cache */
extern void dcache_invalidate(void *addr, size_t size);
extern void dcache_clean(void *addr, size_t size);
extern void dcache_clean_invalidate(void *addr, size_t size);

/*
 * Critical Section Macros
 */
#define CRITICAL_ENTER()    uint64_t _irq_flags = arch_irq_save()
#define CRITICAL_EXIT()     arch_irq_restore(_irq_flags)

/*
 * Utility Macros
 */
#define MS_TO_TICKS(ms)     ((ms) * CONFIG_TICK_RATE_HZ / 1000)
#define TICKS_TO_MS(t)      ((t) * 1000 / CONFIG_TICK_RATE_HZ)

#define ARRAY_SIZE(arr)     (sizeof(arr) / sizeof((arr)[0]))
#define MIN(a, b)           (((a) < (b)) ? (a) : (b))
#define MAX(a, b)           (((a) > (b)) ? (a) : (b))
#define CLAMP(x, lo, hi)    MIN(MAX(x, lo), hi)

#define BIT(n)              (1UL << (n))
#define BITS(h, l)          ((BIT((h) - (l) + 1) - 1) << (l))

#endif /* RTOS_H */
