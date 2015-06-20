/*
 * Copyright (c) 2014-2015 Łukasz S.
 * Distributed under the terms of GPL-2 License.
 */

/*
 * libc/string.c - string handling routines
 */

#include <libc/types.h>
#include <libc/string.h>

/*
 * copy memory area
 */
void *
memcpy(void *dest, const void *src, size_t n)
{
    register uint64_t *srcq;
    register uint64_t *destq;
    uint8_t *srcb;
    uint8_t *destb;

    srcq = (uint64_t *)src;
    destq = (uint64_t *)dest;
    for (; n > sizeof(*srcq); n -= sizeof(*srcq)) {
        *(destq++) = *(srcq++);
    }

    srcb = (uint8_t *)srcq;
    destb = (uint8_t *)destq;
    while (n--) {
        *(destb++) = *(srcb++);
    }

    return dest;
}

/*
 * fill memory with a constant byte
 */
void *
memset(void *dest, uint8_t c, size_t n)
{
    uint8_t *my_dest = (uint8_t *)dest;

    while (n--) {
        *(my_dest++) = c;
    }

    return dest;
}

/*
 * compare two strings
 */
int16_t
strcmp(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2)) {
        ++s1;
        ++s2;
    }
    return (*s1 - *s2);
}

/*
 * calculate the length of a string
 */
size_t
strlen(const char *s1)
{
    size_t ret = 0;

    while (*s1++) {
        ++ret;
    }

    return ret;
}

/*
 * copy a string
 */
char *
strncpy(char *dest, const char *src, size_t n)
{
    size_t i;

    for (i = 0; i < n && src[i] != '\0'; ++i) {
        dest[i] = src[i];
    }

    while (i < n) {
        dest[i++] = '\0';
    }

    return dest;
}

/*
 * locate a character in a string
 */
char *
strchr(const char *str, int c)
{
    char *s = (char *)str;

    while (*s) {
        if (*s == c) {
            return s;
        }
        ++s;
    }

    if (!c) {
        return s;
    }

    return NULL;
}
