#!/usr/bin/env python3
#
# Copyright (c) 2015 Łukasz S.
# Distributed under the terms of GPL-2 License.
#

"""
os64 task automation system
"""

import argparse
import copy
import glob
import inspect
import os
import sys
import time

from config import config

# helpers

def expand(s, **kw):
    """Expand string with values from config and kwargs"""

    args = copy.copy(config)
    args.update(kw)
    return s.format(**args)

def exists(path):
    """Checks if the given path exists"""

    path = expand(path)
    return os.path.exists(path)

def makedir(path):
    """Make sure the given directory exists"""

    path = expand(path)
    if not exists(path):
        run('mkdir -p "%s"' % path)

def mtime(path):
    """Return modification time of given file"""

    path = expand(path)
    return os.stat(path).st_mtime

def files(pattern):
    """List all the files matching the given pattern"""

    pattern = expand(pattern)
    return glob.glob(pattern)

def newer(obj, srcs):
    """Check if the object file exists and is newer than all the source files"""

    # return if obj doesn't exist
    obj = expand(obj)
    if not exists(obj):
        return False

    # compare mtimes of obj and srcs
    src_mtime = max(mtime(expand(x)) for x in srcs)
    obj_mtime = mtime(obj)
    return obj_mtime > src_mtime

def run(cmd, ignore_error = False):
    """Print and execute command. Fail on error, unless ignore_error is set"""

    # print command
    cmd = expand(cmd)
    print(cmd)

    # execute and break on error
    ret = os.system(cmd)
    if ret != 0 and not ignore_error:
        sys.exit('command failed')

# task handlers

def task_clean():
    """Clean build directory"""

    for subdir in ('kernel', 'gui', 'libc', 'loader', 'nf'):
        run('rm -rf "{BUILD_DIR}"/%s' % subdir)

def task_kernel_objs():
    """Build kernel objects"""

    # find relevant inclue files
    incl = files("{BASE_DIR}/include/*/*.h") + files("{BASE_DIR}/nf/include/*.h")

    for subdir in ('kernel', 'gui', 'libc', 'loader'):
        makedir("{BUILD_DIR}/%s" % subdir)

        # compile c sources
        for src in files("{BASE_DIR}/%s/*.c" % subdir):
            obj = src.replace(config['BASE_DIR'], config['BUILD_DIR'], 1) + '.o'

            # skip if the object file is newer than sources
            if newer(obj, incl + [src]):
                continue

            # build object file
            run('{CC} {CFLAGS} -c "%s" -o "%s"' % (src, obj))

        # compile assembly sources
        for src in files("{BASE_DIR}/%s/*.s" % subdir):
            obj = src.replace(config['BASE_DIR'], config['BUILD_DIR'], 1) + '.o'

            # skip if the object file is newer than sources
            if newer(obj, incl + [src]):
                continue

            # build object file
            run('{NASM} -f elf64 "%s" -o "%s"' % (src, obj))

def task_nf_objs():
    """Build nf objects"""

    makedir("{BUILD_DIR}/nf")

    # find all relevant files
    incl = files("{BASE_DIR}/include/*/*.h") + files("{BASE_DIR}/nf/src/*.h")

    for src in files("{BASE_DIR}/nf/src/*.c") + files("{BASE_DIR}/nf/src/os64/*.c"):

        base = os.path.basename(src)
        obj = expand("{BUILD_DIR}/nf/%s.o" % base)

        # skip if the object file is newer than sources
        if newer(obj, incl + [src]):
            continue

        # build object file
        run('{CC} {CFLAGS} -DNF_PLAT_OS64 -DNF_PRINTF -c "%s" -o "%s"' % (src, obj))

def task_kernel():
    """Build kernel image"""

    # dependencies
    task_kernel_objs()
    task_nf_objs()

    # find all relevant object files
    objs = ' '.join(
        files("{BUILD_DIR}/loader/*.o")
        + files("{BUILD_DIR}/gui/*.o")
        + files("{BUILD_DIR}/libc/*.o")
        + files("{BUILD_DIR}/kernel/*.o")
        + files("{BUILD_DIR}/nf/*.o")
    )

    # link the kernel image
    run("{LD} {LDFLAGS} %s -o {BUILD_DIR}/kernel.elf" % objs)
    
def task_fs_images():
    """Build apps/data filesystem images"""

    makedir("{BUILD_DIR}")
    run("genromfs -d {BASE_DIR}/apps -f {BUILD_DIR}/apps.img")
    run("genromfs -d {BASE_DIR}/data -f {BUILD_DIR}/data.img")

def task_disk_image():
    """Build disk image"""

    if exists("{DISK_IMAGE}"):
        return

    # cleanup
    makedir("{MOUNT_DIR}")
    run('sudo umount "{MOUNT_DIR}" &>/dev/null', ignore_error=True)
    run('sudo losetup -d "{LOOP_DEVICE}" &>/dev/null', ignore_error=True)

    # create blank disk image and partition
    makedir("{BUILD_DIR}")
    run('dd if=/dev/zero of="{DISK_IMAGE}" bs=1M count=32')
    run('chmod 666 {DISK_IMAGE}')
    run('fdisk "{DISK_IMAGE}" << EOF\no\nn\np\n1\n2048\n\na\nw\nEOF\n')
    run('sudo losetup "{LOOP_DEVICE}" -P "{DISK_IMAGE}"')

    # create and mount ext2 filesystem
    run('sudo mkfs.ext2 "{LOOP_DEVICE}p1"')
    run('sudo fsck -fy "{LOOP_DEVICE}p1"')
    run('sudo mount -o sync "{LOOP_DEVICE}p1" "{MOUNT_DIR}"')
    time.sleep(0.5)

    # instal grub
    run('sudo grub-install --no-floppy --modules="part_msdos multiboot ext2" '\
        '--boot-directory="{MOUNT_DIR}/boot/" "{LOOP_DEVICE}"')

    # unmount and cleaunp
    run('sudo umount {MOUNT_DIR}')
    run('sudo losetup -d {LOOP_DEVICE}')


def task_install():
    """Install kernel, apps/data filesystems and GRUB config on the disk image"""

    # dependencies
    task_kernel()
    task_fs_images()
    task_disk_image()

    # mount filesystem on the disk image
    makedir("{MOUNT_DIR}")
    run('sudo umount "{MOUNT_DIR}" &>/dev/null', ignore_error=True)
    run('sudo mount -o loop,offset="{DISK_FS_OFFSET}" "{DISK_IMAGE}" "{MOUNT_DIR}"')
    time.sleep(0.5)

    # copy grub config, kernel and romfs images
    makedir("{MOUNT_DIR}/boot/grub")
    run('sudo cp "{BASE_DIR}/misc/grub.cfg" "{MOUNT_DIR}/boot/grub"')
    run('sudo cp "{BUILD_DIR}/kernel.elf" "{MOUNT_DIR}"')
    run('sudo cp "{BUILD_DIR}/apps.img" "{MOUNT_DIR}"')
    run('sudo cp "{BUILD_DIR}/data.img" "{MOUNT_DIR}"')

    # unmount filesystem
    run('sudo umount "{MOUNT_DIR}"')

def task_qemu():
    """Launch in QEMU"""

    # dependencies
    task_install()

    # launch qemu
    run("{QEMU} -hda {DISK_IMAGE} -m 64")

def task_bochs():
    """Launch in Bochs"""

    # dependencies
    task_install()

    # update paths in bochsrc
    bochsrc = open(expand("{BASE_DIR}/misc/bochsrc")).read()
    bochsrc = bochsrc.replace("DISK_IMAGE", config['DISK_IMAGE'])
    bochsrc = bochsrc.replace("SERIAL_OUT", config['SERIAL_OUT'])
    open(expand("{BUILD_DIR}/bochsrc"), 'w').write(bochsrc)

    # launch bochs
    run("bochs -f {BUILD_DIR}/bochsrc -rc {BASE_DIR}/misc/bochscmd")

# entry point

def main():
    """Execute application"""

    desc = "os64 task automation system"
    epilog = "available tasks:\n"

    # find task handlers
    handlers = {}
    for name in sorted(globals()):
        if not name.startswith('task_'):
            continue
        fn = globals()[name]
        name = name.replace('task_', '').replace('_', '-')
        handlers[name] = fn
        epilog += "  %-12s - %s\n" % (name, inspect.getdoc(fn))

    # parse command line arguments
    parser = argparse.ArgumentParser(description=desc, epilog=epilog,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('tasks', nargs=argparse.REMAINDER, help='tasks to execute')
    args = parser.parse_args()
    tasks = args.tasks if args.tasks else ['install']

    # execute tasks
    for task in tasks:
        if not task in handlers:
            sys.exit('Unknown task: %s' % task)
        handlers[task]()

if __name__ == '__main__':
    main()
