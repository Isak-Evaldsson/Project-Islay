#!/bin/sh
set -e

GDB=false
BUILD="kernel/build"

QEMU_OPTS=(
    -cdrom islayos.iso
    -no-shutdown
    -no-reboot
    -d int
    -D $BUILD/log.txt
    -serial file:$BUILD/serial
)

if [ $# -ge 1 ]; then
    if [ $1 = "gdb" ]; then
        GDB=true
    else
        echo "unknown option"
        exit 1
    fi
fi

source ./iso.sh

if [ $GDB = true ]; then
    qemu-system-$(./target-triplet-to-arch.sh $HOST) "${QEMU_OPTS[@]}" -S -s &
    gdb "$BUILD/kernel.elf"  -ex 'target remote localhost:1234' -ex 'layout src' -ex 'layout regs' -ex 'break *_start' -ex 'continue'
else
    qemu-system-$(./target-triplet-to-arch.sh $HOST) "${QEMU_OPTS[@]}"
fi
