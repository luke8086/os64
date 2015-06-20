/*
 * Copyright (c) 2014-2015 Łukasz S.
 * Distributed under the terms of GPL-2 License.
 */

#ifndef _KERNEL_KERNEL_H_
#define _KERNEL_KERNEL_H_

#include <libc/types.h>
#include <libc/string.h>
#include <libc/stdio.h>
#include <libc/array.h>

/*
 * shared constants
 */

/* system version */
#define SYSTEM_NAME     "os/64"
#define SYSTEM_VERSION  "0.0.1"

/* supported page size and memory amount */
enum {
    MEM_PAGE_SIZE       = 0x200000,
    MEM_FRAME_COUNT     = 0x200,
};

/* virtual base addresses */
enum {
    KHEAP_BASE_ADDR     = 0x4000000,  // 64MB
    VID_PAGE_ADDR       = 0x3e800000, // 1000MB
};

/* virtual terminal parameters */
enum {
    VT_COLS     = 80,
    VT_ROWS     = 25,
};

/* misc gui parameters */
enum {
    GUI_WIDTH           = 800,
    GUI_HEIGHT          = 600,
};

/* max lengths of vfs path and path component */
enum {
    PATH_MAX    = 128,
    NAME_MAX    = 32,
};

/* printk levels */
enum {
    KERN_DEBUG  = 1,
    KERN_INFO   = 2,
    KERN_WARN   = 3,
    KERN_ERR    = 4,
};

/* supported interrupt count */
enum {
    INTR_COUNT  = 0x40,
};

/* file types */
enum {
    FT_NONE     = 0,
    FT_REG      = 1,
    FT_DIR      = 2,
    FT_CHR      = 3,
    FT_BLK      = 4,
    FT_UNK      = 5
};

/*
 * misc helper macros
 */

/* trigger breakpoint in bochs */
#define BOCHS_MAGIC_BREAK                               \
     __asm__("xchgw %bx, %bx")

/* display an error and stop the kernel */
#define kpanic(msg)                                     \
    {                                                   \
        cpu_cli();                                      \
        printk(KERN_ERR, msg);                          \
        while(1);                                       \
    }                                                   \

/* panic unless given condition is true */
#define kassert(cond, msg)                              \
    if (!(cond)) {                                      \
        kpanic("assertion failed: " msg);               \
    }                                                   \

/*
 * shared data types
 */

/* storage for cpu registers */
struct regs {
    uint64_t rax, rbx, rcx, rdx;
    uint64_t rdi, rsi, rbp;
    uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
};

/* layout of the stack after interrupt */
struct intr_stack {
    uint64_t rip;
    uint64_t res;
    uint64_t rflags;
    uint64_t rsp;
};

/* vfs operations */
struct vfs_ops {
    int (*open_fn)(uintptr_t sbh, uintptr_t inh);
};

/* file operations */
struct file;
struct file_ops {
    int (*close_fn)(struct file *file);
    ssize_t (*read_fn)(struct file *file, void *buf, size_t nbyte);
    ssize_t (*write_fn)(struct file *file, const void *buf, size_t nbyte);
};

/* file object  */
struct file {
    uint8_t active;
    uintptr_t sbh;
    uintptr_t inh;
    struct file_ops *ops;
    size_t pos;
};

/* file info object */
struct file_info {
    uintptr_t inh;
    int type;
    char name[NAME_MAX];
};

/* time object */
struct time {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint16_t year;
};

/*
 * shared functions
 */


/* kernel/cmos.c */
void cmos_get_time(struct time *t);

/* kernel/cpu.c */
uint8_t cpu_inb(uint16_t port);
void cpu_outb(uint16_t port, uint8_t val);
void cpu_invlpg(uint64_t vaddr);
void cpu_cli(void);
void cpu_sti(void);
uint64_t cpu_task_switch(void);
uint64_t cpu_get_flags(void);
void cpu_set_flags(uint64_t flags);

/* kernel/crtc.c */
void crtc_cursor_set(uint16_t pos);
uint16_t crtc_cursor_get(void);

/* kernel/devfs.c */
int devfs_mount(const char *path);

/* kernel/file.c */
int file_new(uintptr_t sbh, uintptr_t inh, struct file_ops *ops);
void file_release(struct file *file);
int file_close(int fd);
ssize_t file_read(int fd, void *buf, size_t nbyte);
ssize_t file_write(int fd, void *buf, size_t nbyte);
void files_init(void);

/* kernel/intr.c */
typedef void (*intr_handler_fn)(uint8_t intno, struct intr_stack *intr_stack,
                                struct regs *regs);
void intr_init(void);
void intr_set_handler(uint8_t intno, intr_handler_fn intr_handler);
void intr_handle(uint8_t intno, struct intr_stack *intr_stack, struct regs *regs);

/* kernel/kbd.c */
void kbd_init(void);
int kbd_read(uint16_t *key);

/* kernel/kheap.c */
size_t kheap_used(void);
void *kheap_alloc(size_t size);
void kheap_free(void *ptr);
void kheap_init(void);

/* kernel/mboot.c */
void mboot_init(uintptr_t paddr);
void mboot_dump(void);
uintptr_t mboot_mod(uint8_t n);
uintptr_t mboot_mods_end(void);
uintptr_t mboot_vbe_mode_info_ptr(void);
size_t mboot_mmap_entry_count(void);
void mboot_mmap_entry_read(size_t n, uintptr_t *addr, size_t *len, int *avail);

/* kernel/pit.c */
void pit_init(void);
uint64_t pit_get_msecs(void);

/* kernel/pmem.c */
void pmem_init(void);
uintptr_t pmem_alloc(void);
void pmem_free(uintptr_t paddr);
void pmem_dump_avail(void);
void pmem_dump_kern(void);
size_t pmem_total(void);

/* kernel/printk.c */
int printk(int level, const char *fmt, ...);
int vprintk(int level, const char *fmt, va_list args);

/* kernel/ptt.c */
void ptt_init(void);
void ptt_map(uintptr_t vaddr, uintptr_t paddr, uint8_t user, uint8_t present);
void ptt_unmap(uintptr_t vaddr);

/* kernel/romfs.c */
int romfs_mount(uintptr_t addr, const char *path);

/* kernel/task.c */
void tasks_init(void);
task_pid_t task_spawn(uintptr_t entry, int argc, char **argv);
task_pid_t task_spawn_name(char *name, int argc, char **argv);
int task_waitpid(task_pid_t pid);
int task_switch(void);
void task_sleep(uint64_t msecs);
void task_exit(uint8_t code);
int task_count(void);

/* kernel/uart.c */
void uart_init(void);
void uart_write(const char *msg, size_t nbytes);

/* kernel/vbe.c */
int vbe_gfx_mode(void);
uint8_t *vbe_gfx_addr(void);
uint8_t vbe_bpp(void);
void vbe_init(void);

/* kernel/vfs.c */
void vfs_init(void);
int vfs_mount(const char *path, struct vfs_ops *ops, uintptr_t sbh);
int vfs_open(const char *path);

/* kernel/vt.c */
typedef void (*vt_flush_cb)(uint16_t *buf, int cols, int rows);
size_t vt_write(const char *buf, size_t n);
void vt_set_flush_cb(vt_flush_cb cb);
void vt_init(void);

#endif // _KERNEL_KERNEL_H_
