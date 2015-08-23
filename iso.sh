#!/bin/sh
set -e
. ./build.sh

mkdir -p isodir/boot/grub

cp sysroot/boot/plaid.kernel isodir/boot/plaid.kernel
cat > isodir/boot/grub/grub.cfg <<EOF
menuentry "plaid" {
    multiboot /boot/plaid.kernel
}
EOF
grub-mkrescue -o plaid.iso isodir
