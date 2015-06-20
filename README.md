### os/64 ###

A minimal 64-bit operating system for the x86-64 architecture, currently at an early stage of development.

### Build requirements ###

```
python-3
binutils
clang
nasm
genromfs
grub-2
util-linux 2.21+
```

### Testing ###

```
vim config.py    # edit configuration
./make.py        # build
./make.py qemu   # launch in QEMU
./make.py bochs  # launch in Bochs
```

### Screenshots ###

![qemu.png](https://bitbucket.org/qx89l4/os64/raw/master/misc/shot-qemu.png)
![vbox.png](https://bitbucket.org/qx89l4/os64/raw/master/misc/shot-vbox.png)
