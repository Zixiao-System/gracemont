/*
 * Gracemont Industrial Control Framework - X86_64 RTOS
 * Copyright (C) 2024 Zixiao System <https://github.com/Zixiao-System>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * X86_64 RTOS Kernel - Synchronization Primitives
 */

#include "rtos_types.h"
#include "autoconf.h"

/* External scheduler functions */
extern tcb_t *task_current(void);
extern void task_yield(void);
extern void task_ready_add(tcb_t *task);

/*
 * Wait List Management
 */
static void wait_list_add(tcb_t **list, tcb_t *task)
{
    task->next = *list;
    task->prev = NULL;
    if (*list != NULL) {
        (*list)->prev = task;
    }
    *list = task;
}

static tcb_t *wait_list_remove_first(tcb_t **list)
{
    tcb_t *task = *list;
    if (task != NULL) {
        *list = task->next;
        if (*list != NULL) {
            (*list)->prev = NULL;
        }
        task->next = NULL;
    }
    return task;
}

/*
 * Semaphore Operations
 */
void sem_init(semaphore_t *sem, int32_t initial)
{
    sem->count = initial;
    sem->lock = (spinlock_t)SPINLOCK_INIT;
    sem->wait_list = NULL;
}

status_t sem_wait(semaphore_t *sem)
{
    spin_lock_irq(&sem->lock);

    while (sem->count <= 0) {
        tcb_t *task = task_current();
        task->state = TASK_STATE_BLOCKED;
        task->wait_obj = sem;
        wait_list_add(&sem->wait_list, task);
        spin_unlock_irq(&sem->lock);

        task_yield();

        spin_lock_irq(&sem->lock);
        if (task->wait_result != STATUS_OK) {
            spin_unlock_irq(&sem->lock);
            return task->wait_result;
        }
    }

    sem->count--;
    spin_unlock_irq(&sem->lock);
    return STATUS_OK;
}

status_t sem_trywait(semaphore_t *sem)
{
    spin_lock_irq(&sem->lock);

    if (sem->count > 0) {
        sem->count--;
        spin_unlock_irq(&sem->lock);
        return STATUS_OK;
    }

    spin_unlock_irq(&sem->lock);
    return STATUS_WOULD_BLOCK;
}

void sem_post(semaphore_t *sem)
{
    spin_lock_irq(&sem->lock);

    sem->count++;

    tcb_t *task = wait_list_remove_first(&sem->wait_list);
    if (task != NULL) {
        task->wait_obj = NULL;
        task->wait_result = STATUS_OK;
        task->state = TASK_STATE_READY;
        task_ready_add(task);
    }

    spin_unlock_irq(&sem->lock);
}

/*
 * Mutex Operations
 */
void mutex_init(mutex_t *mutex)
{
    mutex->locked = 0;
    mutex->owner = NULL;
    mutex->lock = (spinlock_t)SPINLOCK_INIT;
    mutex->wait_list = NULL;
    mutex->recursion = 0;
}

status_t mutex_lock(mutex_t *mutex)
{
    tcb_t *task = task_current();

    spin_lock_irq(&mutex->lock);

    /* Recursive lock */
    if (mutex->owner == task) {
        mutex->recursion++;
        spin_unlock_irq(&mutex->lock);
        return STATUS_OK;
    }

    while (mutex->locked) {
        task->state = TASK_STATE_BLOCKED;
        task->wait_obj = mutex;
        wait_list_add(&mutex->wait_list, task);
        spin_unlock_irq(&mutex->lock);

        task_yield();

        spin_lock_irq(&mutex->lock);
        if (task->wait_result != STATUS_OK) {
            spin_unlock_irq(&mutex->lock);
            return task->wait_result;
        }
    }

    mutex->locked = 1;
    mutex->owner = task;
    mutex->recursion = 1;

    spin_unlock_irq(&mutex->lock);
    return STATUS_OK;
}

status_t mutex_trylock(mutex_t *mutex)
{
    tcb_t *task = task_current();

    spin_lock_irq(&mutex->lock);

    if (!mutex->locked) {
        mutex->locked = 1;
        mutex->owner = task;
        mutex->recursion = 1;
        spin_unlock_irq(&mutex->lock);
        return STATUS_OK;
    }

    if (mutex->owner == task) {
        mutex->recursion++;
        spin_unlock_irq(&mutex->lock);
        return STATUS_OK;
    }

    spin_unlock_irq(&mutex->lock);
    return STATUS_WOULD_BLOCK;
}

void mutex_unlock(mutex_t *mutex)
{
    tcb_t *task = task_current();

    spin_lock_irq(&mutex->lock);

    if (mutex->owner != task) {
        spin_unlock_irq(&mutex->lock);
        return;
    }

    if (--mutex->recursion > 0) {
        spin_unlock_irq(&mutex->lock);
        return;
    }

    mutex->locked = 0;
    mutex->owner = NULL;

    /* Wake highest priority waiter */
    tcb_t *waiter = wait_list_remove_first(&mutex->wait_list);
    if (waiter != NULL) {
        waiter->wait_obj = NULL;
        waiter->wait_result = STATUS_OK;
        waiter->state = TASK_STATE_READY;
        task_ready_add(waiter);
    }

    spin_unlock_irq(&mutex->lock);
}

/*
 * Event Flags Operations
 */
void event_init(event_t *event)
{
    event->flags = 0;
    event->lock = (spinlock_t)SPINLOCK_INIT;
    event->wait_list = NULL;
}

status_t event_wait(event_t *event, uint32_t mask, uint32_t *flags_out, bool wait_all, bool clear)
{
    spin_lock_irq(&event->lock);

    while (1) {
        uint32_t match = event->flags & mask;

        if ((wait_all && match == mask) || (!wait_all && match != 0)) {
            if (flags_out != NULL) {
                *flags_out = event->flags;
            }
            if (clear) {
                event->flags &= ~mask;
            }
            spin_unlock_irq(&event->lock);
            return STATUS_OK;
        }

        tcb_t *task = task_current();
        task->state = TASK_STATE_BLOCKED;
        task->wait_obj = event;
        wait_list_add(&event->wait_list, task);
        spin_unlock_irq(&event->lock);

        task_yield();

        spin_lock_irq(&event->lock);
    }
}

void event_set(event_t *event, uint32_t mask)
{
    spin_lock_irq(&event->lock);

    event->flags |= mask;

    /* Wake all waiters - they'll check conditions */
    tcb_t *waiter;
    while ((waiter = wait_list_remove_first(&event->wait_list)) != NULL) {
        waiter->wait_obj = NULL;
        waiter->wait_result = STATUS_OK;
        waiter->state = TASK_STATE_READY;
        task_ready_add(waiter);
    }

    spin_unlock_irq(&event->lock);
}

void event_clear(event_t *event, uint32_t mask)
{
    spin_lock_irq(&event->lock);
    event->flags &= ~mask;
    spin_unlock_irq(&event->lock);
}
