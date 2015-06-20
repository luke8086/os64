/*
 * Copyright (c) 2014-2015 Łukasz S.
 * Distributed under the terms of GPL-2 License.
 */

/*
 * kernel/printk.c - formatted print for kernel messages
 */

#include <kernel/kernel.h>

/* print a formatted string to uart and vt */
int
vprintk(int level, const char *fmt, va_list args)
{
    int count;
    static char buf[4096];

    switch (level) {
    case KERN_DEBUG: printk(0, "debug: "); break;
    case KERN_WARN: printk(0, "warn: "); break;
    case KERN_ERR: printk(0, "err: "); break;
    case KERN_INFO: break;
    default: break;
    }

    count = vsnprintf(buf, sizeof(buf), fmt, args);

    if (count < 0) {
        return count;
    }

    vt_write(buf, count);
    uart_write(buf, count);

    return count;
}

/* print a formatted string to uart and vt */
int
printk(int level, const char *fmt, ...)
{
    int ret;
    va_list args;

    va_start(args, fmt);
    ret = vprintk(level, fmt, args);
    va_end(args);

    return ret;
}

