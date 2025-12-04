/*
 * Gracemont Industrial Control Framework - X86_64 RTOS
 * Copyright (C) 2024 Zixiao System <https://github.com/Zixiao-System>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * X86_64 RTOS Kernel - Scheduler
 */

#include "rtos_types.h"

/* Configuration defaults (will come from autoconf.h) */
#ifndef CONFIG_MAX_PRIORITY
#define CONFIG_MAX_PRIORITY 16
#endif
#ifndef CONFIG_MAX_TASKS
#define CONFIG_MAX_TASKS 32
#endif
#ifndef CONFIG_IDLE_STACK_SIZE
#define CONFIG_IDLE_STACK_SIZE 4096
#endif

/* Scheduler State */
static tcb_t *current_task = NULL;
static tcb_t *ready_list[CONFIG_MAX_PRIORITY];
static tcb_t *task_table[CONFIG_MAX_TASKS];
static uint32_t task_count = 0;
static volatile bool scheduler_running = false;
static spinlock_t scheduler_lock = SPINLOCK_INIT;

/* Idle Task */
static tcb_t idle_tcb;
static uint8_t idle_stack[CONFIG_IDLE_STACK_SIZE] ALIGNED(16);

/* External Functions (implemented in context.asm) */
extern void arch_context_switch(tcb_t *prev, tcb_t *next);
extern void arch_first_switch(tcb_t *task);
extern uint64_t arch_irq_save(void);
extern void arch_irq_restore(uint64_t flags);
extern void task_entry_trampoline(void);

/* Forward Declarations */
static void idle_task(void *arg);
static void schedule_locked(void);
void task_terminate(void);
void task_yield(void);

/* Public function to get current task */
tcb_t *task_current(void)
{
    return current_task;
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

/* Public function to add task to ready list */
void task_ready_add(tcb_t *task)
{
    spin_lock_irq(&scheduler_lock);
    ready_list_add(task);
    spin_unlock_irq(&scheduler_lock);
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
 * Task Exit Handler
 */
static void task_exit_handler(void)
{
    task_terminate();
}

/*
 * Task Creation
 */
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
    tcb->remaining_slice = 10;
    tcb->wait_obj = NULL;
    tcb->next = NULL;
    tcb->prev = NULL;
    tcb->total_ticks = 0;
    tcb->switches = 0;

    /* Initialize Stack Frame for x86_64 */
    uint64_t *sp = (uint64_t *)((uintptr_t)stack + stack_size);
    sp = (uint64_t *)((uintptr_t)sp & ~0xFUL);  /* 16-byte align */

    /* Push initial values for task_entry_trampoline:
     * [rsp]     = task function pointer
     * [rsp+8]   = task argument
     * [rsp+16]  = task_exit_handler
     */
    sp -= 3;
    sp[0] = (uint64_t)entry;
    sp[1] = (uint64_t)arg;
    sp[2] = (uint64_t)task_exit_handler;

    /* Set up TCB context for first switch */
    tcb->sp = (reg_t)sp;
    tcb->rbx = 0;
    tcb->rbp = 0;
    tcb->r12 = 0;
    tcb->r13 = 0;
    tcb->r14 = 0;
    tcb->r15 = 0;
    tcb->rip = (reg_t)task_entry_trampoline;

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
    while (1) {
        __asm__ volatile("hlt");
    }
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
    extern volatile tick_t system_ticks;

    spin_lock_irq(&scheduler_lock);
    current_task->state = TASK_STATE_BLOCKED;
    current_task->wake_tick = system_ticks + ticks;
    schedule_locked();
    spin_unlock_irq(&scheduler_lock);
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
        next->switches++;
        current_task = next;
        arch_context_switch(prev, next);
    }
}

void scheduler_tick(void)
{
    extern volatile tick_t system_ticks;

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
    if (current_task != NULL && current_task != &idle_tcb) {
        current_task->total_ticks++;
        current_task->remaining_slice--;
        if (current_task->remaining_slice == 0) {
            current_task->remaining_slice = 10;  /* Reset time slice */
            task_yield();
        }
    }
}

/*
 * Idle Task
 */
static void idle_task(void *arg)
{
    (void)arg;
    while (1) {
        __asm__ volatile("hlt");  /* x86_64 halt instruction */
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
    while (1) {
        __asm__ volatile("hlt");
    }
}
