/*
 * Copyright (c) 2014-2015 Łukasz S.
 * Distributed under the terms of GPL-2 License.
 */

/*
 * kernel/crtc.c - basic 6845 CRTC driver
 */

#include <kernel/kernel.h>

/* i/o port addresses */
enum {
    CRTC_PORT_INDEX = 0x3D4,
    CRTC_PORT_DATA = 0x3D5,
};

/* crtc register indexes */
enum {
    CRTC_REG_CURSOR_H = 0x0E,
    CRTC_REG_CURSOR_L = 0x0F,
};

/* set cursor position */
void
crtc_cursor_set(uint16_t pos)
{
    uint8_t pos_l = pos & 0xff;
    uint8_t pos_h = (pos >> 8) & 0xff;

    cpu_outb(CRTC_PORT_INDEX, CRTC_REG_CURSOR_L);
    cpu_outb(CRTC_PORT_DATA, pos_l);

    cpu_outb(CRTC_PORT_INDEX, CRTC_REG_CURSOR_H);
    cpu_outb(CRTC_PORT_DATA, pos_h);
}

/* get cursor position */
uint16_t
crtc_cursor_get(void)
{
    uint8_t pos_l = 0;
    uint8_t pos_h = 0;
    uint16_t pos = 0;

    cpu_outb(CRTC_PORT_INDEX, CRTC_REG_CURSOR_H);
    pos_h = cpu_inb(CRTC_PORT_DATA);

    cpu_outb(CRTC_PORT_INDEX, CRTC_REG_CURSOR_L);
    pos_l = cpu_inb(CRTC_PORT_DATA);

    pos = (pos_h << 8) | pos_l;

    return pos;
}
