/*
 * Copyright (c) 2014-2015 Łukasz S.
 * Distributed under the terms of GPL-2 License.
 */

#ifndef _LIBC_ARRAY_H_
#define _LIBC_ARRAY_H_

#include <libc/types.h>

#define ARRAY_FOREACH(a, i) \
    for (size_t i = 0, __len = ARRAY_LENGTH(a); i < __len; ++i)

#define ARRAY_INDEX_NONE ((array_index_t)-1)

#define ARRAY_LENGTH(a) (sizeof(a) / sizeof(a[0]))

#define ARRAY_CHECK_INDEX(a, i)     \
    (i >= 0 && (size_t)i < ARRAY_LENGTH(a) && a[i].active)

#define ARRAY_INIT(a)                               \
    ARRAY_FOREACH(a, i) { a[i].active = 0; }        \

#define ARRAY_FIND_INACTIVE(a)                      \
    (__extension__({                                \
        array_index_t __ret = ARRAY_INDEX_NONE;     \
        size_t __len = ARRAY_LENGTH(a);             \
        for (size_t __i = 0; __i < __len; ++__i) {  \
            if (!a[__i].active) {                   \
                __ret = __i;                        \
                break;                              \
            }                                       \
        }                                           \
        __ret;                                      \
    }))

#define ARRAY_TAKE(a)                               \
    (__extension__({                                \
        array_index_t i = ARRAY_FIND_INACTIVE(a);   \
        if (i != ARRAY_INDEX_NONE) {                \
            a[i].active = 1;                        \
        }                                           \
        i == ARRAY_INDEX_NONE ? NULL : &a[i];       \
    }))                                             \

#define ARRAY_RELEASE(x)                            \
    (__extension__({                                \
        x->active = 0;                              \
    }))                                             \

#define ARRAY_FIND(a, i)                            \
    (__extension__({                                \
        i == ARRAY_INDEX_NONE ? NULL : &a[i];       \
    }))                                             \

#define ARRAY_FIND_BY(a, f, v, i)                           \
    (__extension__({                                        \
        *i = ARRAY_INDEX_NONE;                              \
        __typeof__(a[0]) *__ret = NULL;                     \
        ARRAY_FOREACH(a, __i) {                             \
            if (a[__i].active && !strcmp(a[__i].f, v)) {    \
                __ret = &a[__i];                            \
                *i = __i;                                   \
                break;                                      \
            }                                               \
        };                                                  \
        __ret;                                              \
    }))

#endif // _LIBC_ARRAY_H_
