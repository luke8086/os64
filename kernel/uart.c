/*
 * Copyright (c) 2014-2015 Łukasz S.
 * Distributed under the terms of GPL-2 License.
 */

/*
 * kernel/uart.c - UART 8250 driver
 */

#include <kernel/kernel.h>

/* i/o port address and register indexes */
enum {
    UART_COM1   = 0x3F8,    // COM1 base address
    UART_THR    = 0,        // transmitter holding register
    UART_DLL    = 0,        // divisor latch low byte register
    UART_IER    = 1,        // interrupt enable register
    UART_DLH    = 1,        // divisor latch high byte register
    UART_FCR    = 2,        // fifo control register
    UART_LCR    = 3,        // line control register
    UART_MCR    = 4,        // modem control register
    UART_LSR    = 5,        // line status register
};

/* write to the specified register of the specified port */
static inline void
uart_reg_write(uint16_t port, uint8_t reg, uint8_t val)
{
    cpu_outb(port + reg, val);
}

/* read from the specified register of the specified port */
static inline uint8_t
uart_reg_read(uint16_t port, uint8_t reg)
{
    return cpu_inb(port + reg);
}

/* check transmitter holding register empty bit */
static uint8_t
uart_thr_empty(uint16_t port)
{
    uint8_t status = uart_reg_read(port, UART_LSR);
    return status & 0x20;
}

/* write a character to the specified port */
static void
uart_thr_write(uint16_t port, char c)
{
    while (!uart_thr_empty(port)) {};
    uart_reg_write(port, UART_THR, c);
}

/* initialize the specified port */
static void
uart_port_init(uint16_t port)
{
    // disable interrupts
    cpu_outb(port + UART_IER, 0x00);

    // enable DLAB (divisor latch access bit)
    cpu_outb(port + UART_LCR, 0x80);

    // set divisor to 0x0003
    // 115200 BPS / 0x0003 = 38400 BPS
    cpu_outb(port + UART_DLL, 0x03);
    cpu_outb(port + UART_DLH, 0x00);

    // disable DLAB, disable parity bit, set one stop bit, set 8-bit word
    cpu_outb(port + UART_LCR, 0x03);

    // clear and enable FIFO, set 14-byte interrupt trigger level
    cpu_outb(port + UART_FCR, 0xC7);

    // set RTS and DTR
    cpu_outb(port + UART_MCR, 0x03);
}

/* public interface  to write string to the default serial port (COM1) */
void
uart_write(const char *msg, size_t nbytes)
{
    while (nbytes--) {
        char c = *msg++;
        if (c == '\n') {
            uart_thr_write(UART_COM1, '\r');
        }
        uart_thr_write(UART_COM1, c);
    }
}

/* initialize uart driver */
void
uart_init(void)
{
    uart_port_init(UART_COM1);
}
