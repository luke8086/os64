/*
 * Copyright (c) 2014-2015 Łukasz S.
 * Distributed under the terms of GPL-2 License.
 */

/*
 * kernel/kmain.c - kernel c entry points
 */

#include <kernel/kernel.h>
#include <gui/gui.h>

/* entry point for the low-level interrupt service routines */
void
kmain_intr(uint8_t intno, struct intr_stack *intr_stack, struct regs *regs)
{
    intr_handle(intno, intr_stack, regs);
}

/* entry point for the low-level kernel loader */
void
kmain(uintptr_t mboot_paddr)
{
    // initialize video terminal and uart early for debugging
    vt_init();
    uart_init();

    printk(KERN_INFO, "booting %s...\n", SYSTEM_NAME);

    // initialize and dump multiboot structures
    mboot_init(mboot_paddr);
    mboot_dump();

    // initialize physical memory manager and dump memory map
    pmem_init();
    pmem_dump_kern();
    pmem_dump_avail();

    // initialize page allocator (initial page tables are already configured by the loader)
    ptt_init();

    // initialize kernel heap
    kheap_init();

    // initialize video mode
    vbe_init();

    // initialize interrupt handlers
    intr_init();

    // initialize timer, multi-tasking and system calls
    pit_init();
    tasks_init();

    // initialize keyboard driver
    kbd_init();

    // initialize virtual filesystem switch and mount basic filesystems
    files_init();
    vfs_init();
    (void)devfs_mount("/dev");
    (void)romfs_mount((uintptr_t)mboot_mod(0), "/data");
    (void)romfs_mount((uintptr_t)mboot_mod(1), "/apps");

    // initialize GUI features
    if (vbe_gfx_mode()) {
        win_init();
        font_init();
        gui_init();
        fbcon_init();
        bar_init();
    }

    // execute nf interpreter
    task_spawn_name("nf", 0, 0);

    // terminate curren task
    task_exit(0);
}
