current_dir=$(shell pwd)

export CP?=cp
export MAKE?=make
export HOST?=i686-elf
export HOSTARCH:=$(shell $(current_dir)/target-triplet-to-arch.sh $(HOST))

export AR:=$(HOST)-ar
export AS:=$(HOST)-as
export CC:=$(HOST)-gcc

export PREFIX?=/usr
export EXEC_PREFIX?=$(PREFIX)
export BOOTDIR?=/boot
export LIBDIR?=$(EXECDIR)/lib
export INCLUDEDIR?=$(PREFIX)/include

export SYSROOT?=$(current_dir)/sysroot

export CFLAGS?=-O2 -g --sysroot=$(SYSROOT) -isystem=$(INCLUDEDIR)
export CPPFLAGS?=

export DESTDIR?=$(current_dir)/sysroot

.PHONY: all install-headers clean libc kernel

all: setup install-headers libc kernel iso

setup:
	mkdir -p $(SYSROOT)

clean:
	$(MAKE) -C libc clean
	$(MAKE) -C kernel clean
	rm -rf sysroot
	rm -rf isodir
	rm -f plaid.iso

install-headers: setup
	$(MAKE) -C libc install-headers
	$(MAKE) -C kernel install-headers

libc: setup
	$(MAKE) -C libc install

kernel: libc
	$(MAKE) -C kernel install

iso: kernel
	mkdir -p isodir/boot/grub
	cp sysroot/boot/plaid.kernel isodir/boot/plaid.kernel
	cp default.grub.cfg isodir/boot/grub/grub.cfg
	grub-mkrescue -o plaid.iso isodir

qemu: iso
	qemu-system-$(HOSTARCH) -cdrom plaid.iso $(QEMU_ARGS)
