#!/bin/sh
set -e
. ./build.sh

mkdir -p isodir
mkdir -p isodir/boot
mkdir -p isodir/boot/grub

cp sysroot/boot/islayos.kernel isodir/boot/islayos.kernel
cat > isodir/boot/grub/grub.cfg << EOF
menuentry "Project Islay" {
	multiboot /boot/islayos.kernel
}
EOF
grub-mkrescue -o islayos.iso isodir
