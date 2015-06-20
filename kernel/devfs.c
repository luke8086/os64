/*
 * Copyright (c) 2014-2015 Łukasz S.
 * Distributed under the terms of GPL-2 License.
 */

/*
 * kernel/devfs.c - basic device filesystem
 */

#include <kernel/kernel.h>

/* devfs node handles */
enum {
    DEVFS_NODE_ROOT     = 0,
    DEVFS_NODE_VT       = 1,
    DEVFS_NODE_KBD      = 2,
    DEVFS_NODE_TIME     = 3,
    DEVFS_NODE_COUNT    = 4,
};

/* private functions */
static int devfs_open(uintptr_t sbh, uintptr_t inh);
static int devfs_close(struct file *file);
static ssize_t devfs_read_kbd(struct file *file, void *buf, size_t nbyte);
static ssize_t devfs_read_time(struct file *file, void *buf, size_t nbyte);
static ssize_t devfs_read_dir(struct file *file, void *buf, size_t nbyte);
static ssize_t devfs_read(struct file *file, void *buf, size_t nbyte);
static ssize_t devfs_write(struct file *file, const void *buf, size_t nbyte);

/* file operations */
static struct file_ops devfs_file_ops = {
    .close_fn = &devfs_close,
    .read_fn = &devfs_read,
    .write_fn = &devfs_write,
};

/* vfs operations */
static struct vfs_ops devfs_ops = {
    .open_fn = &devfs_open,
};

/* initialize a file object for specified superblock and inode */
static int
devfs_open(uintptr_t sbh, uintptr_t inh) 
{
    return file_new(sbh, inh, &devfs_file_ops);
}

/* close a file */
static int
devfs_close(struct file *file)
{
    file_release(file);
    return 0;
}

/* read from a kbd device */
static ssize_t
devfs_read_kbd(struct file *file, void *buf, size_t nbyte)
{
    uint16_t key;

    if (nbyte < sizeof(key)) {
        return 0;
    }

    if (kbd_read(&key)) {
        return 0;
    }

    memcpy(buf, &key, sizeof(key));

    return sizeof(key);
}

/* read from a time device */
static ssize_t
devfs_read_time(struct file *file, void *buf, size_t nbyte)
{
    struct time tm;
    size_t size = 20;
    char tmpbuf[size + 1];

    if (nbyte < size || file->pos > 0) {
        return 0;
    }

    cmos_get_time(&tm);

    (void)snprintf(tmpbuf, sizeof(tmpbuf), "%04d-%02d-%02d %02d:%02d:%02d\n",
                   tm.year, tm.month, tm.day, tm.hour, tm.minute, tm.second);

    memcpy(buf, tmpbuf, size);

    file->pos += size;
    
    return size;
}

/* read directory entry */
static ssize_t
devfs_read_dir(struct file *file, void *buf, size_t nbyte)
{
    if (nbyte < sizeof(struct file_info)) {
        return 0;
    }

    if (file->inh) {
        return 0;
    }

    file->pos++;

    if (file->pos >= DEVFS_NODE_COUNT) {
        return 0;
    }

    struct file_info *info = (struct file_info *)buf;

    info->inh = file->pos;
    info->type = FT_REG;

    switch(file->pos) {
    case DEVFS_NODE_VT: memcpy(info->name, "vt", 3); break;
    case DEVFS_NODE_KBD: memcpy(info->name, "kbd", 4); break;
    case DEVFS_NODE_TIME: memcpy(info->name, "time", 5); break;
    default: break;
    }

    return sizeof(struct file_info);
}

/* read data to a memory buffer */
static ssize_t
devfs_read(struct file *file, void *buf, size_t nbyte)
{
    switch (file->inh) {
    case DEVFS_NODE_ROOT: return devfs_read_dir(file, buf, nbyte);
    case DEVFS_NODE_KBD: return devfs_read_kbd(file, buf, nbyte);
    case DEVFS_NODE_TIME: return devfs_read_time(file, buf, nbyte);
    default: return 0;
    }
}

/* write to a device */
static ssize_t
devfs_write(struct file *file, const void *buf, size_t nbyte)
{
    switch (file->inh) {
    case DEVFS_NODE_VT: return vt_write((char *)buf, nbyte);
    default: return -1;
    }
}

/* mount a dev filesystem */
int
devfs_mount(const char *volume)
{
    return vfs_mount(volume, &devfs_ops, 0);
}
