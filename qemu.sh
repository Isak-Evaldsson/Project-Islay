#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
#
# See README.md and LICENSE.txt for license details.
#
# Copyright (C) 2026 Isak Evaldsson
#
set -e

function exit_help {
    echo "$0 <qemu_variant> <boot_dir> <log_dir> <use_gdb>"
    exit 1
}

QEMU_VARIANT=$1
BOOT_DIR=$2
LOG_DIR=$3
USE_GDB="${4:-false}"

[[ -z "${QEMU_VARIANT}"  ]] && exit_help
[[ -z "${BOOT_DIR}"  ]] && exit_help
[[ -z "${LOG_DIR}"  ]] && exit_help

mkdir -p $LOG_DIR

gdb_opts=(
    "$BOOT_DIR/kernel.elf"
    -ex 'target remote localhost:1234'
    -ex 'layout src'
    -ex 'layout regs'
    -ex 'break *_start'
    -ex 'continue'
)

qemu_opts=(
    ${qemu_boot_ops[@]}
    -no-shutdown
    -no-reboot
    -d int
    -D $LOG_DIR/log.txt
    -serial file:$LOG_DIR/serial
    -kernel "$BOOT_DIR/kernel.elf"
    -initrd "$BOOT_DIR/project_islay.initrd"
)

if [ $USE_GDB = true ]; then
    echo "Running with gdb..."
    qemu-system-$QEMU_VARIANT "${qemu_opts[@]}" -S -s &
    gdb "${gdb_opts[@]}"
else
    qemu-system-$QEMU_VARIANT "${qemu_opts[@]}"
fi
