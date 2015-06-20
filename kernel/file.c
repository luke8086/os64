/*
 * Copyright (c) 2014-2015 Łukasz S.
 * Distributed under the terms of GPL-2 License.
 */

/*
 * kernel/file.c - file routines
 */

#include <kernel/kernel.h>

/* fixed array of file objects */
static struct file files[32];

/* 
 * initialize a new file object with given superblock handle,
 * inode handle and file ops. return file descriptor.
 */
int
file_new(uintptr_t sbh, uintptr_t inh, struct file_ops *ops)
{
    struct file *file = ARRAY_TAKE(files);

    if (!file) {
        printk(KERN_WARN, "too many files open\n");
        return -1;
    }

    file->inh = inh;
    file->ops = ops;
    file->sbh = sbh;
    file->pos = 0;

    return (file - files);
}

/* release a file object */
void
file_release(struct file *file)
{
    ARRAY_RELEASE(file);
}

/* close a file */
int
file_close(int fd)
{
    struct file *file;
    int ret;

    file = &files[fd];
    ret = file->ops->close_fn(file);

    return ret;
}

/* read data to a memory buffer */
ssize_t
file_read(int fd, void *buf, size_t nbyte)
{
    struct file *file;
    ssize_t ret;

    file = &files[fd];
    ret = file->ops->read_fn(file, buf, nbyte);
    
    return ret;
}

/* write data from a memory buffer */
ssize_t
file_write(int fd, void *buf, size_t nbyte)
{
    struct file *file;
    ssize_t ret;

    file = &files[fd];
    ret = file->ops->write_fn(file, buf, nbyte);
    
    return ret;
}

/* initialize the array of files */
void
files_init(void)
{
    ARRAY_INIT(files);
}
