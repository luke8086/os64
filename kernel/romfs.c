/*
 * Copyright (c) 2014-2015 Łukasz S.
 * Distributed under the terms of GPL-2 License.
 */

/*
 * kernel/romfs.c - basic romfs driver
 */

#include <kernel/kernel.h>

/* inode types */
enum {
    IN_DIR = 0x01,
    IN_REG = 0x02,
};

/* superblock structure */
struct romfs_sb {
    uintptr_t addr;
    uint32_t size;
    uint32_t cksum;
    char *volume;
};

/* inode structure */
struct romfs_inode {
    uintptr_t addr;
    uint8_t flags;
    uint32_t next;
    uint32_t info;
    uint32_t size;
    uint32_t cksum;
    char *name;
    uintptr_t data;
};

/* private functions */
static inline uintptr_t romfs_align_ptr(uintptr_t ptr);
static inline uint32_t romfs_load_be32(uint32_t x);
static void romfs_load_sb(struct romfs_sb *sb, uintptr_t addr);
static void romfs_load_inode(struct romfs_inode *i, uintptr_t inh);
static uintptr_t romfs_first_inode(uintptr_t sbh);
static uintptr_t romfs_first_child_inode(uintptr_t sbh, uintptr_t inh);
static uintptr_t romfs_next_inode(uintptr_t sbh, uintptr_t inh);
static void romfs_load_file_info(struct file_info *info, uintptr_t inh);
static int romfs_open(uintptr_t sbh, uintptr_t inh);
static int romfs_close(struct file *file);
static ssize_t romfs_read_reg(struct file *file, void *buf, size_t nbyte);
static ssize_t romfs_read_dir(struct file *file, void *buf, size_t nbyte);
static ssize_t romfs_read(struct file *file, void *buf, size_t nbyte);
static ssize_t romfs_write(struct file *file, const void *buf, size_t nbyte);

/* file operations */
static struct file_ops romfs_file_ops = {
    .close_fn = &romfs_close,
    .read_fn = &romfs_read,
    .write_fn = &romfs_write,
};

/* vfs operations */
static struct vfs_ops romfs_ops = {
    .open_fn = &romfs_open,
};

/* align pointer to 16-bit boundary */
static inline uintptr_t
romfs_align_ptr(uintptr_t ptr)
{
    return (ptr & 0xF) ? ((ptr + 0x10) & ~0xF) : ptr;
}

/* load a big-endian 32-bit value */
static inline uint32_t
romfs_load_be32(uint32_t x)
{
    return ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >> 8) |
            (((x) & 0x0000ff00) << 8)  | (((x) & 0x000000ff) << 24));
}

/* load a superblock from a specified address */
static void
romfs_load_sb(struct romfs_sb *sb, uintptr_t addr)
{
    uint32_t *size, *cksum;

    sb->addr = addr;

    size = (uint32_t *) (sb->addr + 8);
    cksum = (uint32_t *) (sb->addr + 12);

    sb->size = romfs_load_be32(*size);
    sb->cksum = romfs_load_be32(*cksum);

    sb->volume = (char *) (sb->addr + 16);
}

/* load an inode structure for a specified inode handle */
static void
romfs_load_inode(struct romfs_inode *i, uintptr_t inh)
{
    uint32_t *next, *spec, *size, *cksum;

    next = (uint32_t*)(inh);
    spec = (uint32_t*)(inh + 4);
    size = (uint32_t*)(inh + 8);
    cksum = (uint32_t*)(inh + 12);

    i->addr = inh;
    i->next = romfs_load_be32(*next) & (~0xF);
    i->flags = romfs_load_be32(*next) & 0xF;
    i->info = romfs_load_be32(*spec);
    i->size = romfs_load_be32(*size);
    i->cksum = romfs_load_be32(*cksum);
    i->name = (char*)inh + 16;
    i->data = inh + 16 + strlen(i->name) + 1;
    i->data = romfs_align_ptr(i->data);
}

/* get the first inode handle in a specified superblock */
static uintptr_t
romfs_first_inode(uintptr_t sbh)
{
    struct romfs_sb sb;
    romfs_load_sb(&sb, sbh);

    uintptr_t inh;
    inh = (uintptr_t)sb.volume + strlen(sb.volume) + 1;
    inh = romfs_align_ptr(inh);

    return inh;
}

/* get the first child inode handle in a specified superblock */
static uintptr_t
romfs_first_child_inode(uintptr_t sbh, uintptr_t inh)
{
    struct romfs_inode inode;

    romfs_load_inode(&inode, inh);

    return sbh + inode.info;
}

/* get the next inode handle in a specified superblock */
static uintptr_t
romfs_next_inode(uintptr_t sbh, uintptr_t inh)
{
    struct romfs_inode inode;

    romfs_load_inode(&inode, inh);

    if (!inode.next) {
        return 0;
    }

    return sbh + inode.next;
}

/* load a file info structure for a specified inode */
static void
romfs_load_file_info(struct file_info *info, uintptr_t inh)
{
    struct romfs_inode inode;
    romfs_load_inode(&inode, inh);

    info->inh = inh;

    strncpy(info->name, inode.name, NAME_MAX);
    info->name[NAME_MAX - 1] = '\x00';

    if (inode.flags & IN_DIR) {
        info->type = FT_DIR;
    } else if (inode.flags & IN_REG) {
        info->type = FT_REG;
    } else {
        info->type = FT_UNK;
    }
}

/* initialize a file object for a specified superblock and inode */
static int
romfs_open(uintptr_t sbh, uintptr_t inh) 
{
    struct romfs_sb sb;

    romfs_load_sb(&sb, sbh);

    if (!inh) {
        inh = romfs_first_inode(sbh);
    }

    return file_new(sbh, inh, &romfs_file_ops);
}

/* close a file */
static int
romfs_close(struct file *file)
{
    file_release(file);
    return 0;
}

/* read from a regular file to a buffer */
static ssize_t
romfs_read_reg(struct file *file, void *buf, size_t nbyte)
{
    struct romfs_inode inode;

    romfs_load_inode(&inode, file->inh);

    if (file->pos + nbyte > inode.size) {
        nbyte = inode.size - file->pos;
    }

    memcpy(buf, (void*)(inode.data + file->pos), nbyte);

    file->pos += nbyte;

    return nbyte;
}

/* read a directory entry to a buffer */
static ssize_t
romfs_read_dir(struct file *file, void *buf, size_t nbyte)
{
    uintptr_t inh;
    struct file_info info;

    if (nbyte < sizeof(struct file_info)) {
        return 0;
    }

    if (file->pos) {
        inh = file->pos;
    } else if (file->inh) {
        inh = romfs_first_child_inode(file->sbh, file->inh);
    } else {
        inh = romfs_first_inode(file->sbh);
    }

    inh = romfs_next_inode(file->sbh, inh);

    if (!inh) {
        return 0;
    }

    romfs_load_file_info(&info, inh);
    memcpy(buf, &info, sizeof(info));

    file->pos = inh;

    // skip .. entry
    if (!strcmp(info.name, "..")) {
        return romfs_read_dir(file, buf, nbyte);
    }

    return sizeof(struct file_info);
}

/* read data from a file to a buffer */
static ssize_t
romfs_read(struct file *file, void *buf, size_t nbyte)
{
    struct file_info info;
    romfs_load_file_info(&info, file->inh);

    switch (info.type) {
    case FT_REG: return romfs_read_reg(file, buf, nbyte);
    case FT_DIR: return romfs_read_dir(file, buf, nbyte);
    default: return 0;
    }
}

/* write to a file (not supported, returns error) */
static ssize_t
romfs_write(struct file *file, const void *buf, size_t nbyte)
{
    return -1;
}

/* mount a rom filesystem */
int
romfs_mount(uintptr_t addr, const char *volume)
{
    return vfs_mount(volume, &romfs_ops, addr);
}
