# SPDX-License-Identifier: BSD-3-Clause
#
# See README.md and LICENSE.txt for license details.
#
# Copyright (C) 2024 Isak Evaldsson
#
#!/bin/sh
set -e
. ./build.sh

mkdir -p isodir
mkdir -p isodir/boot
mkdir -p isodir/boot/grub

cp -r sysroot/boot/. isodir/boot/.
cat > isodir/boot/grub/grub.cfg << EOF
menuentry "Project Islay" {
	multiboot /boot/kernel.elf
	module /boot/project_islay.initrd
}
EOF
grub-mkrescue -o islayos.iso isodir
