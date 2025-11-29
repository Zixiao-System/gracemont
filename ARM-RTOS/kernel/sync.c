/*
 * Gracemont Industrial Control Framework
 * ARM64 RTOS Kernel - Synchronization Primitives
 */

#include "rtos_types.h"
#include "rtos_config.h"

extern tcb_t *task_current(void);
extern void task_yield(void);

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

static void wait_list_remove(tcb_t **list, tcb_t *task)
{
    if (task->prev != NULL) {
        task->prev->next = task->next;
    } else {
        *list = task->next;
    }
    if (task->next != NULL) {
        task->next->prev = task->prev;
    }
    task->next = NULL;
    task->prev = NULL;
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
        /* Will be added to ready list by scheduler */
    }

    spin_unlock_irq(&sem->lock);
}

/*
 * Mutex Operations
 */
void mutex_init(mutex_t *mutex)
{
    mutex->owner = NULL;
    mutex->lock_count = 0;
    mutex->orig_priority = 0;
    mutex->lock = (spinlock_t)SPINLOCK_INIT;
    mutex->wait_list = NULL;
}

status_t mutex_lock(mutex_t *mutex)
{
    tcb_t *task = task_current();

    spin_lock_irq(&mutex->lock);

    /* Recursive lock */
    if (mutex->owner == task) {
        mutex->lock_count++;
        spin_unlock_irq(&mutex->lock);
        return STATUS_OK;
    }

    while (mutex->owner != NULL) {
        /* Priority inheritance */
        tcb_t *owner = (tcb_t *)mutex->owner;
        if (task->priority > owner->priority) {
            owner->priority = task->priority;
        }

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

    mutex->owner = task;
    mutex->lock_count = 1;
    mutex->orig_priority = task->priority;

    spin_unlock_irq(&mutex->lock);
    return STATUS_OK;
}

status_t mutex_trylock(mutex_t *mutex)
{
    tcb_t *task = task_current();

    spin_lock_irq(&mutex->lock);

    if (mutex->owner == NULL) {
        mutex->owner = task;
        mutex->lock_count = 1;
        mutex->orig_priority = task->priority;
        spin_unlock_irq(&mutex->lock);
        return STATUS_OK;
    }

    if (mutex->owner == task) {
        mutex->lock_count++;
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

    if (--mutex->lock_count > 0) {
        spin_unlock_irq(&mutex->lock);
        return;
    }

    /* Restore priority */
    task->priority = mutex->orig_priority;
    mutex->owner = NULL;

    /* Wake highest priority waiter */
    tcb_t *waiter = wait_list_remove_first(&mutex->wait_list);
    if (waiter != NULL) {
        waiter->wait_obj = NULL;
        waiter->wait_result = STATUS_OK;
        waiter->state = TASK_STATE_READY;
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
    }

    spin_unlock_irq(&event->lock);
}

void event_clear(event_t *event, uint32_t mask)
{
    spin_lock_irq(&event->lock);
    event->flags &= ~mask;
    spin_unlock_irq(&event->lock);
}

/*
 * Message Queue Operations
 */
status_t msgq_init(msgqueue_t *mq, void *buffer, size_t msg_size, size_t capacity)
{
    if (mq == NULL || buffer == NULL || msg_size == 0 || capacity == 0) {
        return STATUS_INVALID;
    }

    mq->buffer = buffer;
    mq->msg_size = msg_size;
    mq->capacity = capacity;
    mq->head = 0;
    mq->tail = 0;
    mq->count = 0;
    mq->lock = (spinlock_t)SPINLOCK_INIT;
    mq->send_wait = NULL;
    mq->recv_wait = NULL;

    return STATUS_OK;
}

status_t msgq_send(msgqueue_t *mq, const void *msg, tick_t timeout)
{
    spin_lock_irq(&mq->lock);

    while (mq->count >= mq->capacity) {
        if (timeout == 0) {
            spin_unlock_irq(&mq->lock);
            return STATUS_WOULD_BLOCK;
        }

        tcb_t *task = task_current();
        task->state = TASK_STATE_BLOCKED;
        task->wait_obj = mq;
        wait_list_add(&mq->send_wait, task);
        spin_unlock_irq(&mq->lock);

        task_yield();

        spin_lock_irq(&mq->lock);
    }

    /* Copy message to buffer */
    uint8_t *dst = (uint8_t *)mq->buffer + (mq->tail * mq->msg_size);
    const uint8_t *src = (const uint8_t *)msg;
    for (size_t i = 0; i < mq->msg_size; i++) {
        dst[i] = src[i];
    }

    mq->tail = (mq->tail + 1) % mq->capacity;
    mq->count++;

    /* Wake receiver */
    tcb_t *waiter = wait_list_remove_first(&mq->recv_wait);
    if (waiter != NULL) {
        waiter->wait_obj = NULL;
        waiter->wait_result = STATUS_OK;
        waiter->state = TASK_STATE_READY;
    }

    spin_unlock_irq(&mq->lock);
    return STATUS_OK;
}

status_t msgq_recv(msgqueue_t *mq, void *msg, tick_t timeout)
{
    spin_lock_irq(&mq->lock);

    while (mq->count == 0) {
        if (timeout == 0) {
            spin_unlock_irq(&mq->lock);
            return STATUS_WOULD_BLOCK;
        }

        tcb_t *task = task_current();
        task->state = TASK_STATE_BLOCKED;
        task->wait_obj = mq;
        wait_list_add(&mq->recv_wait, task);
        spin_unlock_irq(&mq->lock);

        task_yield();

        spin_lock_irq(&mq->lock);
    }

    /* Copy message from buffer */
    uint8_t *dst = (uint8_t *)msg;
    const uint8_t *src = (const uint8_t *)mq->buffer + (mq->head * mq->msg_size);
    for (size_t i = 0; i < mq->msg_size; i++) {
        dst[i] = src[i];
    }

    mq->head = (mq->head + 1) % mq->capacity;
    mq->count--;

    /* Wake sender */
    tcb_t *waiter = wait_list_remove_first(&mq->send_wait);
    if (waiter != NULL) {
        waiter->wait_obj = NULL;
        waiter->wait_result = STATUS_OK;
        waiter->state = TASK_STATE_READY;
    }

    spin_unlock_irq(&mq->lock);
    return STATUS_OK;
}
