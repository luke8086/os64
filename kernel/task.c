/*
 * Copyright (c) 2014-2015 Łukasz S.
 * Distributed under the terms of GPL-2 License.
 */

/* kernel/task.c - multi-tasking routines */

#include <kernel/kernel.h>

enum {
    TASK_COUNT      = 8,            // max number of tasks
    TASK_STACK_SIZE = 32768,        // size of task stack
    TASK_RFLAGS     = (0x01 << 9),  // initial RFLAGS (interrupts enabled)
};

/* structure representing the entire state of a task */
struct task {
    uint8_t active;

    task_pid_t pid;
    task_pid_t waits_for;

    uint8_t exit_req;
    uint8_t exit_code;
    uint64_t sleep_until;

    uint64_t rip;
    uint64_t rflags;
    uint64_t rsp;
    struct regs regs;
};

/* private methods */
static void task_save(struct task *task, struct intr_stack *intr_stack, struct regs *regs);
static void task_restore(struct task *task, struct intr_stack *intr_stack, struct regs *regs);
static void task_do_exit(struct task *task);
static struct task *task_next(struct task *task);
static void task_intr_handle(uint8_t intno, struct intr_stack *intr_stack, struct regs *regs);

/* static data */
static uint64_t task_next_pid = 0;
static struct task *task_current;
static struct task tasks[TASK_COUNT];
typedef uint8_t stack_t[TASK_STACK_SIZE];
static stack_t stacks[TASK_COUNT];

/* save task state from the interrupt stack */
static void
task_save(struct task *task, struct intr_stack *intr_stack, struct regs *regs)
{
    memcpy(&task->regs, regs, sizeof(struct regs));

    task->rip    = intr_stack->rip;
    task->rflags = intr_stack->rflags;
    task->rsp    = intr_stack->rsp;
}

/* restore task state to the interrupt stack */
static void
task_restore(struct task *task, struct intr_stack *intr_stack, struct regs *regs)
{
    memcpy(regs, &task->regs, sizeof(struct regs));

    intr_stack->rip     = task->rip;
    intr_stack->rflags  = task->rflags;
    intr_stack->rsp     = task->rsp;
}

/* terminate task and unlock tasks waiting for it */
static void
task_do_exit(struct task *task)
{
    ARRAY_FOREACH(tasks, i) {
        if (tasks[i].waits_for == task->pid) {
            tasks[i].waits_for = -1;
            tasks[i].regs.rax = task->exit_code;
        }
    }
    ARRAY_RELEASE(task);
}

/* return task which should be activated after the given one */
static struct task *
task_next(struct task *task)
{
    uint64_t now;
    int idx;

    now = pit_get_msecs();
    idx = task - tasks;

    while (1) {
        idx = (idx + 1) % ARRAY_LENGTH(tasks);
        task = &tasks[idx];

        if (!task->active)
            continue;

        if (task->waits_for != -1)
            continue;

        if (task->sleep_until > now)
            continue;

        return task;
    }

    // NOTREACHED
    return 0;
}

/* handle task switch interrupt */
static void
task_intr_handle(uint8_t intno, struct intr_stack *intr_stack, struct regs *regs)
{
    if (task_current->exit_req) {
        task_do_exit(task_current);
    } else {
        task_save(task_current, intr_stack, regs);
    }

    task_current = task_next(task_current);
    task_restore(task_current, intr_stack, regs);
}

/* spawn a new task with given entry point and args. return its pid or -1 */
task_pid_t
task_spawn(uintptr_t entry, int argc, char **argv)
{
    struct task *task;
    int idx;

    task = ARRAY_TAKE(tasks);
    if (!task) {
        return -1;
    }

    // fail on pid overflow
    if (task_next_pid <= 0) {
        ARRAY_RELEASE(task);
        return -1;
    }

    idx = task - tasks;

    task->pid = task_next_pid++;
    task->exit_req = 0;
    task->waits_for = -1;
    task->sleep_until = 0;
    task->rflags = TASK_RFLAGS;
    task->rip = (uint64_t)entry;
    task->rsp = (uint64_t)&stacks[idx+1];
    task->regs.rdi = (uint64_t)argc;
    task->regs.rsi = (uint64_t)argv;

    return task->pid;
}

/* spawn a new task with given name and args. return its pid or -1 */
task_pid_t
task_spawn_name(char *name, int argc, char **argv)
{
    extern void nf_main(int argc, char **argv);

    uintptr_t entry;

    if (!strcmp(name, "nf"))
        entry = (uintptr_t)nf_main;
    else
        return -1;

    return task_spawn(entry, argc, argv);
}

/* switch to the next task */
int
task_switch(void)
{
    return cpu_task_switch();
}

/* delay current task until specified task terminates and return its exit code */
int
task_waitpid(task_pid_t pid)
{
    int code;

    if (task_current->pid == pid) {
        return -1;
    }

    ARRAY_FOREACH(tasks, i) {
        if (tasks[i].active && tasks[i].pid == pid) {
            task_current->waits_for = pid;
            break;
        }
    }

    code = task_switch();

    return code;
}

/* delay current task for a given amount of milliseconds */
void
task_sleep(uint64_t msecs)
{
    uint64_t time;

    time = pit_get_msecs() + msecs;
    task_current->sleep_until = time;

    (void)task_switch();
}

/* terminate current task with the given status code */
void
task_exit(uint8_t code)
{
    task_current->exit_req = 1;
    task_current->exit_code = code;

    (void)task_switch();
}

/* return amount of currently running tasks */
int
task_count(void)
{
    int count = 0;

    ARRAY_FOREACH(tasks, i) {
        count += !!tasks[i].active;
    }

    return count;
}

/* initialize task structures and interrupt handler */
void
tasks_init(void)
{
    ARRAY_INIT(tasks);

    // first task is the kernel itself
    tasks[0].active = 1;
    tasks[0].exit_req = 0;
    tasks[0].waits_for = -1;
    tasks[0].sleep_until = 0;
    tasks[0].pid = task_next_pid++;
    tasks[0].rflags = TASK_RFLAGS;
    task_current = &tasks[0];

    // enable interrupt handler for switching tasks
    intr_set_handler(0x31, task_intr_handle);
}
