/*
 * Copyright (c) 2014-2015 Łukasz S.
 * Distributed under the terms of GPL-2 License.
 */

/*
 * kernel/cmos.c - basic CMOS driver
 */

#include <kernel/kernel.h>

/* i/o port addresses */
enum {
    CMOS_PORT_ADDR = 0x70,
    CMOS_PORT_DATA = 0x71,
};

/* private functions */
static uint16_t cmos_load_bcd(uint16_t bcd);
static uint8_t cmos_get_reg(uint8_t reg);
static uint8_t cmos_check_uip(void);
static void cmos_read_time(struct time *t);
static int cmos_compare_time(struct time *t1, struct time *t2);
static void cmos_sanitize_time(struct time *t);

/* load a binary coded decimal */
static uint16_t
cmos_load_bcd(uint16_t bcd)
{
    uint16_t ret = 0;

    ret += ((bcd >> 8) & 0x0F) * 100;
    ret += ((bcd >> 4) & 0x0F) * 10;
    ret += ((bcd >> 0) & 0x0F);

    return ret;
}

/* read a CMOS register */
static uint8_t
cmos_get_reg(uint8_t reg)
{
    cpu_outb(CMOS_PORT_ADDR, reg);
    return cpu_inb(CMOS_PORT_DATA);
}

/* check the update-in-progress flag */
static uint8_t
cmos_check_uip(void)
{
    return cmos_get_reg(0x0a) & (0x80);
}

/* load raw values from CMOS time registers to a time struct */
static void
cmos_read_time(struct time *t)
{
    while (cmos_check_uip()) { };

    t->second = cmos_get_reg(0x00);
    t->minute = cmos_get_reg(0x02);
    t->hour = cmos_get_reg(0x04);
    t->day = cmos_get_reg(0x07);
    t->month = cmos_get_reg(0x08);
    t->year = cmos_get_reg(0x09);
}

/* sanitize raw values in a time struct */
static void
cmos_sanitize_time(struct time *t)
{
    uint8_t reg_b;
    int is_bcd, is_12h, is_pm;

    reg_b = cmos_get_reg(0x0b);
    is_bcd = !(reg_b & 0x04);
    is_12h = !(reg_b & 0x02);
    is_pm = !!(t->hour & 0x80);

    // clear the PM bit
    t->hour = t->hour & 0x7F;
    
    if (is_bcd) {
        t->second = cmos_load_bcd(t->second);
        t->minute = cmos_load_bcd(t->minute);
        t->hour = cmos_load_bcd(t->hour);
        t->day = cmos_load_bcd(t->day);
        t->month = cmos_load_bcd(t->month);
        t->year = cmos_load_bcd(t->year);
    }

    if (is_12h && is_pm) {
        t->hour = (t->hour + 12) % 24;
    }

    // calculate full year
    t->year += (t->year < 70) ? 2000 : 1900;
}

/* compare two time structs */
static int
cmos_compare_time(struct time *t1, struct time *t2)
{
    uint64_t *i1, *i2;

    i1 = (uint64_t *)t1;
    i2 = (uint64_t *)t2;

    return *i2 - *i1;
}

/* load verified and sanitized CMOS time to a time struct */
void
cmos_get_time(struct time *t)
{
    struct time t1, t2;

    do {
        cmos_read_time(&t1);
        cmos_read_time(&t2);
    } while (cmos_compare_time(&t1, &t2));

    cmos_sanitize_time(&t2);

    memcpy(t, &t2, sizeof(t2));
}
