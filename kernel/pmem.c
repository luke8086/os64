/*
 * Copyright (c) 2014-2015 Łukasz S.
 * Distributed under the terms of GPL-2 License.
 */

/*
 * kernel/pmem.c - physical memory manager
 */

#include <kernel/kernel.h>

/* frame status */
enum {
    PMEM_FRAME_RESV  = 0,
    PMEM_FRAME_AVAIL = 1,
};

/* left / right alignment macros */
#define ALIGN_L(x, s) ((x % s == 0) ? x : (x)     - (x % s))
#define ALIGN_R(x, s) ((x % s == 0) ? x : (x + s) - (x % s))

/* memory map */
static uint8_t pmem_frames[MEM_FRAME_COUNT];

/* saved total amount of available memory */
static size_t pmem_total_p;

/* private methods */
static void pmem_set_avail(uintptr_t start, uintptr_t end);
static void pmem_set_resv(uintptr_t start, uintptr_t end);
static void pmem_init_kern(void);
static void pmem_init_bios(void);

/* mark frames contained in a specified memory region as available */
static void
pmem_set_avail(uintptr_t start, uintptr_t end)
{
    start = ALIGN_R(start, MEM_PAGE_SIZE);
    end = ALIGN_L(end, MEM_PAGE_SIZE);

    while (start < end) {
        pmem_frames[start / MEM_PAGE_SIZE] = PMEM_FRAME_AVAIL;
        start += MEM_PAGE_SIZE;
    }
}

/* mark frames containing a specified memory region as reserved */
static void
pmem_set_resv(uintptr_t start, uintptr_t end)
{
    start = ALIGN_L(start, MEM_PAGE_SIZE);
    end = ALIGN_R(end, MEM_PAGE_SIZE);

    while (start < end) {
        pmem_frames[start / MEM_PAGE_SIZE] = PMEM_FRAME_RESV;
        start += MEM_PAGE_SIZE;
    }
}

/* dump available memory regions */
void
pmem_dump_avail(void)
{
    intptr_t start = -1;
    intptr_t end = -1;

    printk(KERN_INFO, "usable memory map:\n");

    for (ssize_t i = 0; i < MEM_FRAME_COUNT; ++i) {
        if (pmem_frames[i] == PMEM_FRAME_AVAIL) {
            if (start < 0) {
                start = i;
            }
            end = i;
        } else {
            if (start >= 0) {
                printk(KERN_INFO, "  AVL     %016x - %016x\n",
                       start * MEM_PAGE_SIZE, end * MEM_PAGE_SIZE);
            }
            start = -1;
            end = -1;
        }
    }

    if (start >= 0) {
        printk(KERN_INFO, "  AVL     %016x - %016x\n",
               start * MEM_PAGE_SIZE, end * MEM_PAGE_SIZE);
    }
}

/* dump kernel memory */
void
pmem_dump_kern(void)
{
    // extern void *kernel_start;
    // extern void *kernel_end;
    extern void *kernel_text_start;
    extern void *kernel_text_end;
    extern void *kernel_data_start;
    extern void *kernel_data_end;
    extern void *kernel_bss_start;
    extern void *kernel_bss_end;

    // printk(KERN_INFO, "kernel:      %016x - %016x\n",
    //        &kernel_start, &kernel_end);
    printk(KERN_INFO, "kernel memory map:\n");
    printk(KERN_INFO, "  TEXT    %016x - %016x\n",
           &kernel_text_start, &kernel_text_end);
    printk(KERN_INFO, "  DATA    %016x - %016x\n",
           &kernel_data_start, &kernel_data_end);
    printk(KERN_INFO, "  BSS     %016x - %016x\n",
           &kernel_bss_start, &kernel_bss_end);
}

/* alloc a single frame. return physical address or 0 on error */
uintptr_t
pmem_alloc(void)
{
    for (size_t i = 0; i < MEM_FRAME_COUNT; ++i) {
        if (pmem_frames[i] == PMEM_FRAME_AVAIL) {
            pmem_frames[i] = PMEM_FRAME_RESV;
            return i * MEM_PAGE_SIZE;
        }
    }

    return 0;
}

/* release a single frame */
void
pmem_free(uintptr_t paddr)
{
    kassert((paddr % MEM_PAGE_SIZE == 0), "paddr must be page-aligned");
    pmem_frames[paddr / MEM_PAGE_SIZE] = PMEM_FRAME_AVAIL;
}

/* setup available memory regions basing on bios memory map */
static void
pmem_init_bios(void)
{
    size_t mmap_entry_count = mboot_mmap_entry_count();
    uintptr_t addr;
    size_t len;
    int avail;

    pmem_total_p = 0;
    for (size_t i = 0; i < mmap_entry_count; ++i) {
        mboot_mmap_entry_read(i, &addr, &len, &avail);
        if (avail) {
            pmem_set_avail(addr, addr + len - 1);
            pmem_total_p += len;
        }
    }
}

/* mark kernel memory as reserved */
static void
pmem_init_kern(void)
{
    // FIXME: figure out a better way to check where the mboot data ends
    extern void *kernel_end;
    uintptr_t mods_end;
    uintptr_t end;

    mods_end = mboot_mods_end();
    end = (uintptr_t)&kernel_end;
    end = (mods_end > end) ? mods_end : end;

    pmem_set_resv(0, end);
}

/* return total amount of memory */
size_t
pmem_total(void)
{
    return pmem_total_p;
}

/* initialize physical memory map */
void
pmem_init(void)
{
    memset(pmem_frames, PMEM_FRAME_RESV, sizeof(pmem_frames));
    pmem_init_bios();
    pmem_init_kern();
}
