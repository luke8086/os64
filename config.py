import os
import sys

config = {}

config['CC'] = 'clang'
config['LD'] = 'ld'
config['NASM'] = 'nasm'
config['QEMU'] = 'qemu-system-x86_64'

config['BASE_DIR'] = os.path.dirname(sys.argv[0])
config['BUILD_DIR'] = os.path.join(config['BASE_DIR'], 'build')
config['MOUNT_DIR'] = os.path.join(config['BASE_DIR'], 'tmp', 'mnt')

config['DISK_IMAGE'] = os.path.join(config['BUILD_DIR'], 'disk.img')
config['DISK_FS_OFFSET'] = 1048576
config['LOOP_DEVICE'] = '/dev/loop4'
config['SERIAL_OUT'] = os.path.join(config['BASE_DIR'], 'tmp', 'serial.out')

config['CFLAGS'] = (
    "-std=c11 -march=x86-64 -mcmodel=large -mno-red-zone -mno-mmx -mno-sse " \
    "-mno-sse2 -ffreestanding -Wall -Wextra -pedantic -Wno-unused-parameter " \
    "-I{BASE_DIR}/include -I{BASE_DIR}/nf/include -O3"
).format(BASE_DIR = config['BASE_DIR'])

config['LDFLAGS'] = (
    "-m elf_x86_64 -nostdlib -nodefaultlibs " \
    "-T{BASE_DIR}/misc/kernel.ld"
).format(BASE_DIR = config['BASE_DIR'])
