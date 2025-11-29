/*
 * Gracemont Industrial Control Framework
 * ARM64 RTOS Kernel - Scheduler
 */

#include "rtos_types.h"
#include "rtos_config.h"

/* Scheduler State */
static tcb_t *current_task = NULL;
static tcb_t *ready_list[CONFIG_MAX_PRIORITY];
static tcb_t *task_table[CONFIG_MAX_TASKS];
static uint32_t task_count = 0;
static volatile tick_t system_ticks = 0;
static volatile bool scheduler_running = false;
static spinlock_t scheduler_lock = SPINLOCK_INIT;

/* Idle Task */
static tcb_t idle_tcb;
static uint8_t idle_stack[CONFIG_IDLE_STACK_SIZE] ALIGNED(16);

/* External Functions */
extern void arch_context_switch(tcb_t *prev, tcb_t *next);
extern void arch_first_switch(tcb_t *task);
extern uint64_t arch_irq_save(void);
extern void arch_irq_restore(uint64_t flags);

/* Forward Declarations */
static void idle_task(void *arg);
static void schedule_locked(void);

/*
 * Spinlock Operations
 */
void spin_lock(spinlock_t *lock)
{
    uint32_t tmp;
    __asm__ volatile(
        "   sevl\n"
        "1: wfe\n"
        "   ldaxr %w0, [%1]\n"
        "   cbnz %w0, 1b\n"
        "   stxr %w0, %w2, [%1]\n"
        "   cbnz %w0, 1b"
        : "=&r"(tmp)
        : "r"(&lock->lock), "r"(1)
        : "memory"
    );
}

void spin_unlock(spinlock_t *lock)
{
    __asm__ volatile(
        "stlr wzr, [%0]"
        : : "r"(&lock->lock) : "memory"
    );
}

void spin_lock_irq(spinlock_t *lock)
{
    lock->irq_flags = arch_irq_save();
    spin_lock(lock);
}

void spin_unlock_irq(spinlock_t *lock)
{
    spin_unlock(lock);
    arch_irq_restore(lock->irq_flags);
}

/*
 * Ready List Management
 */
static void ready_list_add(tcb_t *task)
{
    uint8_t prio = task->priority;
    task->state = TASK_STATE_READY;

    if (ready_list[prio] == NULL) {
        ready_list[prio] = task;
        task->next = task;
        task->prev = task;
    } else {
        tcb_t *head = ready_list[prio];
        task->next = head;
        task->prev = head->prev;
        head->prev->next = task;
        head->prev = task;
    }
}

static void ready_list_remove(tcb_t *task)
{
    uint8_t prio = task->priority;

    if (task->next == task) {
        ready_list[prio] = NULL;
    } else {
        task->prev->next = task->next;
        task->next->prev = task->prev;
        if (ready_list[prio] == task) {
            ready_list[prio] = task->next;
        }
    }
    task->next = NULL;
    task->prev = NULL;
}

static tcb_t *get_highest_ready(void)
{
    for (int i = CONFIG_MAX_PRIORITY - 1; i >= 0; i--) {
        if (ready_list[i] != NULL) {
            return ready_list[i];
        }
    }
    return &idle_tcb;
}

/*
 * Task Creation
 */
static void task_entry_wrapper(void (*entry)(void *), void *arg)
{
    entry(arg);
    task_terminate();
}

status_t task_create(tcb_t *tcb, const char *name, void (*entry)(void *),
                     void *arg, uint8_t priority, void *stack, size_t stack_size)
{
    if (tcb == NULL || entry == NULL || stack == NULL) {
        return STATUS_INVALID;
    }
    if (priority >= CONFIG_MAX_PRIORITY) {
        return STATUS_INVALID;
    }
    if (task_count >= CONFIG_MAX_TASKS) {
        return STATUS_NO_MEM;
    }

    /* Initialize TCB */
    tcb->name = name;
    tcb->priority = priority;
    tcb->state = TASK_STATE_SUSPENDED;
    tcb->stack_base = stack;
    tcb->stack_size = stack_size;
    tcb->time_slice = 10;
    tcb->remaining_slice = tcb->time_slice;
    tcb->wait_obj = NULL;
    tcb->next = NULL;
    tcb->prev = NULL;

    /* Initialize Stack Frame */
    uint64_t *sp = (uint64_t *)((uintptr_t)stack + stack_size);
    sp = (uint64_t *)((uintptr_t)sp & ~0xF);  /* 16-byte align */

    /* Setup initial context for task_entry_wrapper(entry, arg) */
    sp -= 34;  /* x0-x30, elr, spsr, padding */

    tcb->regs[0] = (reg_t)entry;      /* x0 = entry */
    tcb->regs[1] = (reg_t)arg;        /* x1 = arg */
    tcb->regs[29] = 0;                /* x29 (fp) = 0 */
    tcb->regs[30] = 0;                /* x30 (lr) = 0 */
    tcb->elr = (reg_t)task_entry_wrapper;
    tcb->spsr = 0x305;                /* EL1h, IRQ/FIQ enabled */
    tcb->sp = (reg_t)sp;

    /* Add to task table */
    spin_lock_irq(&scheduler_lock);
    tcb->id = task_count;
    task_table[task_count++] = tcb;
    spin_unlock_irq(&scheduler_lock);

    return STATUS_OK;
}

status_t task_start(tcb_t *tcb)
{
    if (tcb == NULL) {
        return STATUS_INVALID;
    }

    spin_lock_irq(&scheduler_lock);
    if (tcb->state == TASK_STATE_SUSPENDED) {
        ready_list_add(tcb);
        if (scheduler_running && current_task != NULL) {
            if (tcb->priority > current_task->priority) {
                spin_unlock_irq(&scheduler_lock);
                task_yield();
                return STATUS_OK;
            }
        }
    }
    spin_unlock_irq(&scheduler_lock);
    return STATUS_OK;
}

void task_terminate(void)
{
    spin_lock_irq(&scheduler_lock);
    current_task->state = TASK_STATE_TERMINATED;
    schedule_locked();
    /* Never returns */
}

void task_yield(void)
{
    spin_lock_irq(&scheduler_lock);
    if (current_task->state == TASK_STATE_RUNNING) {
        ready_list_add(current_task);
        /* Rotate to next task at same priority */
        ready_list[current_task->priority] = current_task->next;
    }
    schedule_locked();
    spin_unlock_irq(&scheduler_lock);
}

void task_sleep(tick_t ticks)
{
    spin_lock_irq(&scheduler_lock);
    current_task->state = TASK_STATE_BLOCKED;
    current_task->wake_tick = system_ticks + ticks;
    schedule_locked();
    spin_unlock_irq(&scheduler_lock);
}

tcb_t *task_current(void)
{
    return current_task;
}

/*
 * Scheduler Core
 */
static void schedule_locked(void)
{
    tcb_t *prev = current_task;
    tcb_t *next = get_highest_ready();

    if (next != prev) {
        if (next != &idle_tcb) {
            ready_list_remove(next);
        }
        next->state = TASK_STATE_RUNNING;
        current_task = next;
        arch_context_switch(prev, next);
    }
}

void scheduler_tick(void)
{
    system_ticks++;

    /* Check sleeping tasks */
    for (uint32_t i = 0; i < task_count; i++) {
        tcb_t *task = task_table[i];
        if (task->state == TASK_STATE_BLOCKED && task->wait_obj == NULL) {
            if (system_ticks >= task->wake_tick) {
                spin_lock(&scheduler_lock);
                if (task->state == TASK_STATE_BLOCKED) {
                    ready_list_add(task);
                }
                spin_unlock(&scheduler_lock);
            }
        }
    }

    /* Time slice management */
    if (current_task != &idle_tcb) {
        current_task->remaining_slice--;
        if (current_task->remaining_slice == 0) {
            current_task->remaining_slice = current_task->time_slice;
            task_yield();
        }
    }
}

tick_t get_system_ticks(void)
{
    return system_ticks;
}

/*
 * Idle Task
 */
static void idle_task(void *arg)
{
    (void)arg;
    while (1) {
        __asm__ volatile("wfi");
    }
}

/*
 * Scheduler Start
 */
void scheduler_start(void)
{
    /* Initialize idle task */
    task_create(&idle_tcb, "idle", idle_task, NULL, 0, idle_stack, sizeof(idle_stack));
    idle_tcb.state = TASK_STATE_READY;

    /* Find highest priority ready task */
    current_task = get_highest_ready();
    if (current_task != &idle_tcb) {
        ready_list_remove(current_task);
    }
    current_task->state = TASK_STATE_RUNNING;

    scheduler_running = true;
    arch_first_switch(current_task);

    /* Never returns */
}
