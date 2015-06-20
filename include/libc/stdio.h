/*
 * Copyright (c) 2014-2015 Łukasz S.
 * Distributed under the terms of GPL-2 License.
 */

#ifndef _LIBC_STDIO_H_
#define _LIBC_STDIO_H_

#include <libc/types.h>

int pf_vsnprintf(void *buf, size_t nbyte, const char *fmt, va_list args);
int pf_snprintf(char *buf, size_t nbyte, const char *fmt, ...);

#define vsnprintf pf_vsnprintf
#define snprintf pf_snprintf

#endif // _LIBC_STDIO_H_
