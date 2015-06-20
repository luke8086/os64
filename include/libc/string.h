/*
 * Copyright (c) 2014-2015 Łukasz S.
 * Distributed under the terms of GPL-2 License.
 */

#ifndef _LIBC_STRING_H_
#define _LIBC_STRING_H_

#include <libc/types.h>

void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *dest, uint8_t c, size_t n);
int16_t strcmp(const char *s1, const char *s2);
size_t strlen(const char *s);
char *strncpy(char *dest, const char *src, size_t n);
char *strchr(const char *str, int c);

#endif // _LIBC_STRING_H_
