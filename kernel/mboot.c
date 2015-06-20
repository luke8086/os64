/*
 * Copyright (c) 2014-2015 Łukasz S.
 * Distributed under the terms of GPL-2 License.
 */

/*
 * kernel/mboot.c - multiboot structures and routines
 */

#include <kernel/kernel.h>

/* multiboot flags */
enum {
    MBOOT_INFO_CMDLINE      = 0x00000004,   // command line present
    MBOOT_INFO_MODS         = 0x00000008,   // modules present
    MBOOT_INFO_ELF_SHDR     = 0X00000020,   // elf section hdr table present
    MBOOT_INFO_MMAP         = 0x00000040,   // memory map present
    MBOOT_INFO_LOADER_NAME  = 0x00000200,   // boot loader name present
    MBOOT_INFO_VIDEO        = 0x00000800,   // video info present
    MBOOT_MEM_AVAIL         = 0x00000001,   // memory available
};

/* memory map entry */
struct mboot_mmap_entry {
    uint32_t size;
    uint64_t addr;
    uint64_t len;
    uint32_t type;
} __attribute__((packed));

/* module entry */
struct mboot_mod {
    uint32_t start;
    uint32_t end;
    uint32_t cmdline;
    uint32_t reserved;
} __attribute__ ((packed));

/* multiboot info structure */
struct mboot_info {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;

    uint32_t elf_num;
    uint32_t elf_size;
    uint32_t elf_addr;
    uint32_t elf_shndx;

    uint32_t mmap_length;
    uint32_t mmap_addr;

    uint32_t drives_length;
    uint32_t drives_addr;

    uint32_t config_table;
    uint32_t boot_loader_name;
    uint32_t apm_table;

    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;
} __attribute__ ((packed));

/* pointer to the mboot_info struct of the bootloader */
static struct mboot_info *mboot_info;

/* private functions */
static void mboot_dump_mods(uint32_t count, uint32_t paddr) __attribute__((unused));
static void mboot_dump_mmap(uint64_t len, uint64_t paddr) __attribute__((unused));

/* dump module entries */
static void
mboot_dump_mods(uint32_t count, uint32_t paddr)
{
    struct mboot_mod *start, *m;
    size_t i;

    start = (struct mboot_mod*)(uint64_t)paddr;

    printk(KERN_INFO, "modules:\n");

    for (i = 0; i < count; ++i) {
        m = start + i;
        printk(KERN_INFO, "  %08x - %08x %s\n", m->start, m->end,
               (char*)(uint64_t)m->cmdline);
    }
}

/* dump memory map entries */
static void
mboot_dump_mmap(uint64_t len, uint64_t paddr)
{
    struct mboot_mmap_entry *mmap_start, *mmap_end, *e;
    const char *type;

    mmap_start  = (struct mboot_mmap_entry*)paddr;
    mmap_end = (struct mboot_mmap_entry*)(paddr + len);

    printk(KERN_INFO, "bios memory map:\n");

    for (e = mmap_start; e < mmap_end; ++e) {
        type = e->type == MBOOT_MEM_AVAIL ? "AVL" : "RES";
        printk(KERN_INFO, "  %s     %016x - %016x\n",
               type, e->addr, e->addr + e->len);
    }
}

/* return amount of the memory map entries */
size_t
mboot_mmap_entry_count(void)
{
    return mboot_info->mmap_length / sizeof(struct mboot_mmap_entry);
}

/* fetch n-th memory map entry */
void
mboot_mmap_entry_read(size_t n, uintptr_t *addr, size_t *len, int *avail)
{
    struct mboot_mmap_entry *entry;

    entry = (struct mboot_mmap_entry*)(uint64_t)(mboot_info->mmap_addr) + n;
    *addr = entry->addr;
    *len = entry->len;
    *avail = (entry->type == MBOOT_MEM_AVAIL);
}

/* dump information provided by multiboot bootloader */
void
mboot_dump(void)
{
    struct mboot_info *m = mboot_info;

    if (m->flags & MBOOT_INFO_LOADER_NAME) {
        printk(KERN_INFO, "bootloader: %s\n", (char*)(uint64_t)m->boot_loader_name);
    }

    if (m->flags & MBOOT_INFO_CMDLINE) {
        //printk(KERN_INFO, "cmdline: %s\n", (char*)(uint64_t)m->cmdline);
    }

    if (m->flags & MBOOT_INFO_MODS) {
        //mboot_dump_mods(m->mods_count, m->mods_addr);
    }

    if (m->flags & MBOOT_INFO_MMAP) {
        mboot_dump_mmap(m->mmap_length, m->mmap_addr);
    }
}

/* get memory address of the nth module */
uintptr_t
mboot_mod(uint8_t n)
{
    struct mboot_mod *mod = (struct mboot_mod*)(uintptr_t)mboot_info->mods_addr;
    return mod[n].start;
}

/* get final memory address of multiboot modules */
uintptr_t
mboot_mods_end(void)
{
    uint32_t count = mboot_info->mods_count;

    if (count == 0) {
        return 0;
    }

    struct mboot_mod *mods = (struct mboot_mod*)(uint64_t)mboot_info->mods_addr;
    struct mboot_mod *last = mods + count - 1;

    return (uintptr_t)(last->start + last->end);
}

/* get physical memory address of the vbe mode info */
uintptr_t
mboot_vbe_mode_info_ptr(void)
{
    int has_info = mboot_info->flags & MBOOT_INFO_VIDEO;
    uintptr_t ptr = has_info ? mboot_info->vbe_mode_info : 0;
    return ptr;
}

/* initialize multiboot info structure */
void
mboot_init(uintptr_t paddr)
{
    mboot_info = (struct mboot_info*)paddr;
}
