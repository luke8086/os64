/*
 * Copyright (c) 2014-2015 Łukasz S.
 * Distributed under the terms of GPL-2 License.
 */

/*
 * kernel/pit.c - basic 8254 PIT driver
 */

#include <kernel/kernel.h>

/* i/o port addresses */
enum {
    PIT_CR0 = 0x40, // counter 0 register
    PIT_CWR = 0x43, // control word register
};

/* tick counter */
volatile static uint64_t pit_nticks;

/* read current value of the millisecond timer */
uint64_t
pit_get_msecs(void)
{
    uint64_t ret;
    uint64_t flags;

    flags = cpu_get_flags();
    cpu_cli();
    ret = pit_nticks * 50;
    cpu_set_flags(flags);

    return ret;
}

/* handle timer interrupt */
static void
pit_intr_handle(uint8_t intno, struct intr_stack *intr_stack, struct regs *regs)
{
    ++pit_nticks;
}

/*
 * initialize counter 0
 */
void
pit_init(void)
{
    uint8_t hz = 20; // 20 hz divisor
    uint32_t div = 1193180 / hz;
    uint8_t div_l = (uint8_t)((div >> 0) & 0xFF);
    uint8_t div_h = (uint8_t)((div >> 8) & 0xFF);

    // counter 0, write both lsb and msb, use mode 3, binary counter
    cpu_outb(PIT_CWR, 0x36);

    // write lsb and msb for counter 0
    cpu_outb(PIT_CR0, div_l);
    cpu_outb(PIT_CR0, div_h);

    pit_nticks = 0;

    intr_set_handler(0x20, pit_intr_handle);
}
