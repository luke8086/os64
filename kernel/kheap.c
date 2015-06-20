/*
 * Copyright (c) 2014-2015 Łukasz S.
 * Distributed under the terms of GPL-2 License.
 */

/*
 * kernel/kheap.c - heap allocator
 */

#include <kernel/kernel.h>

/* pointer to the end of the heap */
static uintptr_t kheap_ptr;

/* amount of allocated frames */
static size_t kheap_frames;

/* private methods */
static inline size_t kheap_avail(void);
static int kheap_expand(void);

/* return amount of allocated memory in bytes */
size_t
kheap_used(void)
{
    return kheap_ptr - KHEAP_BASE_ADDR;
}

/* return amount of memory available without allocating new frames */
static inline size_t
kheap_avail(void)
{
    return kheap_frames * MEM_PAGE_SIZE - kheap_used();
}

/* expand heap by one frame */
static int
kheap_expand(void)
{
    uintptr_t paddr, vaddr;

    // allocate physical frame
    paddr = pmem_alloc();
    if (!paddr)
        return -1;

    // map physical frame to the next virtual address
    vaddr = KHEAP_BASE_ADDR + kheap_frames * MEM_PAGE_SIZE;
    ptt_map(vaddr, paddr, 1, 1);
    ++kheap_frames;

    return 0;
}

/*
 * allocate a memory chunk with the given size
 * return pointer on success or 0 on failure
 */
void *
kheap_alloc(size_t size)
{
    void *ret;

    // try to expand heap to the required size
    while (kheap_avail() < size) {
        if (kheap_expand())
            return 0;
    }

    ret = (void*)kheap_ptr;
    kheap_ptr += size;

    return ret;
}

/* free the memory chunk pointed by ptr (currently does nothing) */
void
kheap_free(void *ptr)
{
}

/* initialize the kernel heap */
void
kheap_init(void)
{
    kheap_ptr = KHEAP_BASE_ADDR;
    kheap_frames = 0;
}
