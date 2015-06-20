/*
 * Copyright (c) 2014-2015 Łukasz S.
 * Distributed under the terms of GPL-2 License.
 */

/*
 * kernel/ptt.c - page table manager
 */

#include <kernel/kernel.h>

/* amount of entries in a single page table */
enum {
    PTT_ENTRY_COUNT = 0x200,
};

/* page table entry */
union ptt_entry {
    struct {
        uint8_t  present     :  1; // present
        uint8_t  rw          :  1; // read/write
        uint8_t  user        :  1; // user/supervisor
        uint8_t  pwt         :  1; // page-level writethrough
        uint8_t  pcd         :  1; // page-level cache disable
        uint8_t  accessed    :  1; // accessed
        uint8_t  dirty       :  1; // dirty
        uint8_t  page_size   :  1; // page size
        uint8_t  global      :  1; // global page
        uint32_t _avl0       :  3; // available to software
        uint64_t addr        : 40; // pdpt / pd / page base address
        uint64_t _avl1       : 11; // available to software
        uint8_t  nx          :  1; // no-execute
    } __attribute__((packed));

    uint64_t raw;
};

/* top-level page table (page map level 4) */
static union ptt_entry *ptt_pml4;

/* private methods */
static void ptt_dump(union ptt_entry entry) __attribute__((unused));

/* dump a specified page table entry */
static void
ptt_dump(union ptt_entry entry)
{
    uint64_t flags = entry.raw & 0xFFF;
    uint64_t addr = entry.raw & ~0xFFF;
    printk(KERN_INFO, "ptt: addr:%016x addr:%016x flags:%016x\n", addr, entry.addr << 12, flags);
}

/* map a virtual address to a physical address with given flags */
void
ptt_map(uint64_t vaddr, uint64_t paddr, uint8_t user, uint8_t present)
{
    kassert(paddr % MEM_PAGE_SIZE == 0, "paddr must be page-aligned");

    uint16_t pml4_offs = (vaddr & 0xFF8000000000) >> 39;
    uint16_t pdpt_offs = (vaddr & 0x007FC0000000) >> 30;
    uint16_t pd_offs =   (vaddr & 0x00003FE00000) >> 21;
    uint16_t page_offs = (vaddr & 0x0000001FFFFF) >> 00;

    kassert(!pml4_offs, "vaddr must be below 1GB");
    kassert(!pdpt_offs, "vaddr must be below 1GB");
    kassert(!page_offs, "vaddr must be page-aligned");

    union ptt_entry *ptt_pdpt = (union ptt_entry*)(ptt_pml4[pml4_offs].addr << 12);
    union ptt_entry *ptt_pd =   (union ptt_entry*)(ptt_pdpt[pdpt_offs].addr << 12);
    union ptt_entry *ptt_pde =  (union ptt_entry*)(&ptt_pd[pd_offs]);

    ptt_pde->present = present;
    ptt_pde->rw = 1;
    ptt_pde->user = user;
    ptt_pde->page_size = 1;
    ptt_pde->addr = (paddr >> 12);
    ptt_pde->nx = 0;

    cpu_invlpg(vaddr);
}

/* unmap a virtual address */
void
ptt_unmap(uint64_t vaddr)
{
    ptt_map(vaddr, 0, 0, 0);
}

/* initialize the page table manager */
void
ptt_init(void)
{
    extern union ptt_entry loader_pml4[PTT_ENTRY_COUNT];
    ptt_pml4 = loader_pml4;
}
