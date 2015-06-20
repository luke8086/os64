/*
 * Copyright (c) 2014-2015 Łukasz S.
 * Distributed under the terms of GPL-2 License.
 */

/*
 * kernel/intr.c - interrupt handling
 */

#include <kernel/kernel.h>

/* array of interrupt handling routines */
static intr_handler_fn intr_handlers[INTR_COUNT];

/* assign an interrupt handling routine */
void
intr_set_handler(uint8_t intno, intr_handler_fn intr_handler)
{
    intr_handlers[intno] = intr_handler;
}

/* handle specified interrupt */
void
intr_handle(uint8_t intno, struct intr_stack *intr_stack, struct regs *regs)
{
    intr_handler_fn fn = intr_handlers[intno];

    if (!fn) {
        return;
    }

    fn(intno, intr_stack, regs);
}

/* initialize array of interrupt handling routines and enable interrupts */
void
intr_init(void)
{
    memset(intr_handlers, 0, sizeof(intr_handlers));
    cpu_sti();
}
