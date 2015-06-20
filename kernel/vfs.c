/*
 * Copyright (c) 2014-2015 Łukasz S.
 * Distributed under the terms of GPL-2 License.
 */

/*
 * kernel/vfs.c - virtual filesystem switch
 */

#include <kernel/kernel.h>

/* mount point info */
struct vfs_mountpoint {
    uint8_t active;
    char volume[PATH_MAX];
    uintptr_t sbh;
    struct vfs_ops *ops;
};

/* private functions */
static ssize_t vfs_path_len(const char *path);
static const char *vfs_path_next(const char *path);
static int vfs_path_first(char buf[NAME_MAX], const char *path);
static int vfs_find_name(struct vfs_mountpoint *mp, struct file_info *parent,
                         const char *name, struct file_info *dest);
static int vfs_find_path(struct vfs_mountpoint *mp, const char *path,
                         struct file_info *dest);
static struct vfs_mountpoint *vfs_find_mountpoint(const char *path);

/* array of the mountpoints */
static struct vfs_mountpoint vfs_mountpoints[8];

/* get the length of the first component of a path. return -1 on invalid path */
static ssize_t
vfs_path_len(const char *path)
{
    if (path[0] != '/') {
        return -1;
    }

    ++path;

    for (ssize_t i = 0; i < NAME_MAX; ++i) {
        if (!path[i] || path[i] == '/') {
            return i;
        }
    }

    return -1;
}

/* get address of the next component of a path. return NULL on invalid path */
static const char *
vfs_path_next(const char *path)
{
    ssize_t len = vfs_path_len(path);

    if (len < 0) {
        return NULL;
    }

    return path + 1 + len;
}

/* get the first component of a path. return 0 on success */
static int
vfs_path_first(char buf[NAME_MAX], const char *path)
{
    ssize_t len = vfs_path_len(path);

    if (len < 0) {
        return -1;
    }

    strncpy(buf, path + 1, len);
    buf[len] = 0;

    return 0;
}

/*  mount a filesystem on a given volume */
int
vfs_mount(const char *volume, struct vfs_ops *ops, uintptr_t sbh)
{
    struct vfs_mountpoint *mp = ARRAY_TAKE(vfs_mountpoints);

    if (!mp) {
        printk(KERN_WARN, "cannot mount %s, too many filesystems\n", volume);
        return -1;
    }

    vfs_path_first(mp->volume, volume);
    mp->ops = ops;
    mp->sbh = sbh;

    return 0;
}

/* find a mounpoint for a given path */
static struct vfs_mountpoint *
vfs_find_mountpoint(const char *path)
{
    char buf[NAME_MAX];
    array_index_t i;

    if (vfs_path_first(buf, path) < 0) {
        return NULL;
    }

    return ARRAY_FIND_BY(vfs_mountpoints, volume, buf, &i);
}

/* find a file with a given name in a given parent directory  */
static int
vfs_find_name(struct vfs_mountpoint *mp, struct file_info *parent,
              const char *name, struct file_info *dest)
{
    int found = 0;
    struct file_info info;

    int fd = mp->ops->open_fn(mp->sbh, parent->inh);

    if (fd < 0) {
        return -1;
    }

    while(file_read(fd, &info, sizeof(info))) {
        if (!strcmp(info.name, name)) {
            found = 1;
            break;
        }
    }

    file_close(fd);

    if (!found) {
        return -1;
    }

    memcpy(dest, &info, sizeof(info));

    return 0;
}

/*  find a file with a given path on a given mountpoint  */
static int
vfs_find_path(struct vfs_mountpoint *mp, const char *path, struct file_info *dest)
{
    char buf[NAME_MAX];
    struct file_info info;
    struct file_info parent;

    // set parent to the root directory
    parent.type = FT_DIR;
    parent.inh = 0;

    // for empty path return a root directory
    if (!strlen(path) || !strcmp(path, "/")) {
        dest->type = parent.type;
        dest->inh = parent.inh;
        return 0;
    }

    while (1) {
        // load the first path component
        if (vfs_path_first(buf, path) < 0) {
            return -1;
        }

        // find file in the parent directory
        if (vfs_find_name(mp, &parent, buf, &info)) {
            return -1;
        }

        // skip to the next component
        path = vfs_path_next(path);

        // end of path
        if (!path || !path[0]) {
            break;
        }

        // return error if the current entry is not a directory
        if (info.type != FT_DIR) {
            return -1;
        }

        // ignore trailing slash for directories
        if (!strcmp(path, "/")) {
            break;
        }

        // set current inode as the parent
        memcpy(&parent, &info, sizeof(parent));
    }

    memcpy(dest, &info, sizeof(info));

    return 0;
}

/* open a file with a given path */
int
vfs_open(const char *path)
{
    struct file_info info;
    struct vfs_mountpoint *mp;

    if (!(mp = vfs_find_mountpoint(path))) {
        return -1;
    }

    path = vfs_path_next(path);

    if (vfs_find_path(mp, path, &info)) {
        return -1;
    }

    return mp->ops->open_fn(mp->sbh, info.inh);
}

/* initialize mountpoints */
void
vfs_init(void)
{
    ARRAY_INIT(vfs_mountpoints);
}
