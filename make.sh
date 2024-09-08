#!/bin/bash
# SPDX-License-Identifier: BSD-3-Clause
#
# See README.md and LICENSE.txt for license details.
#
# Copyright (C) 2024 Isak Evaldsson
#

# make.sh: Project Islay OS build script
#
# Resposilble for configuring the build environment as well as calling the 
# makefiles with the subfolders.
set -e

# All subdirs including makefiles
PROJECTS="kernel initrd"

# All subdirs containging system headers
SYSTEM_HEADER_PROJECTS="kernel"

# Options configured by the input parameters
USE_GDB=false
ARCH=i386
COMPILER_PREFIX=i686-elf # Depends on arch

# Display script help text
function help() {
    echo "Usage: $0 command [args]"
    echo "Commands [supported args]:"
    echo "  clean: Calls make clean on all sub-projects, useful when needing to" 
    echo "         do a full re-build."
    echo ""
    echo "  build --arch <arch>: Builds the kernel/OS by calling make build on"
    echo "                       all subprojects."
    echo ""
    echo "  qemu --arch <arch> --gdb: Build and run kernel i qemu."
    echo ""
    echo "Args:"
    echo "  --arch <arch>: Target architecutre, uses $ARCH as default"
    echo "  --gdb|-g:      attch qemu to gdb"
}

# clean(): clean everything to force a full re-build
function clean() {
    echo "Calling make clean on all sub-projects..."
    for PROJECT in $PROJECTS; do
        (cd $PROJECT && DESTDIR="$SYSROOT" $MAKE clean)
    done

    rm -rf sysroot
    rm -rf isodir
    rm -rf islayos.iso
}

function config() {
    echo "Configuring environment for the $ARCH / $COMPILER_PREFIX tool-chain"

    # Export ARCH to called makefiles
    export ARCH=$ARCH

    # TODO: Stack smashing protector complains about the task switch code, so we disable it for know.
    # The long term solution is to make sure that only c code is compiled with stack-smash protection
    export CFLAGS='-Wextra -Wall -Og -g' #-fstack-protector-all'
    export CPPFLAGS=''

    # Define tool-chain
    export AR=${COMPILER_PREFIX}-ar
    export AS=${COMPILER_PREFIX}-as
    export CC=${COMPILER_PREFIX}-gcc
    export MAKE=${MAKE:-make}

    # Do we need to export?, why are them even needed?
    export PREFIX=/usr
    export EXEC_PREFIX=$PREFIX
    export BOOTDIR=/boot
    export LIBDIR=$EXEC_PREFIX/lib
    export INCLUDEDIR=$PREFIX/include


    # Configure the cross-compiler to use the desired system root.
    export SYSROOT="$(pwd)/sysroot"
    export CC="$CC --sysroot=$SYSROOT"

    # Work around that the -elf gcc targets doesn't have a system include directory
    # because it was configured with --without-headers rather than --with-sysroot.
    if echo "$COMPILER_PREFIX" | grep -Eq -- '-elf($|-)'; then
        export CC="$CC -isystem=$INCLUDEDIR"
    fi
}

function build_iso() {
    echo "Building iso..."

    # Install headers into sysroot
    mkdir -p "$SYSROOT"

    for PROJECT in $SYSTEM_HEADER_PROJECTS; do
        (cd $PROJECT && DESTDIR="$SYSROOT" $MAKE install-headers)
    done

    # Build all the subprojects 
    for PROJECT in $PROJECTS; do
        (cd $PROJECT && DESTDIR="$SYSROOT" $MAKE install)
    done

    # Merge them into an ISO
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
}

# Starts the qemu session
function qemu() {
    echo "Lanuching kernel in qemu..."

    local BUILD="kernel/build"
    local gdb_opts=(
        "$BUILD/kernel.elf"
        -ex 'target remote localhost:1234'
        -ex 'layout src'
        -ex 'layout regs'
        -ex 'break *_start'
        -ex 'continue'
    )
    local qemu_opts=(
        -cdrom islayos.iso
        -no-shutdown
        -no-reboot
        -d int
        -D $BUILD/log.txt
        -serial file:$BUILD/serial
    )

    if [ $USE_GDB = true ]; then
        echo "Running with gdb..."
        qemu-system-$ARCH "${qemu_opts[@]}" -S -s &
        gdb "${gdb_opts[@]}"
    else
        qemu-system-$ARCH "${qemu_opts[@]}"
    fi
}

if [ $# -lt 1 ]; then
    help
    exit 1
fi

# Store here since it will be shited out
cmd=$1
shift

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -a|--arch)
            case $2 in
                i386)
                ;&
                x86)
                ;&
                i686)
                ARCH=i386
                COMPILER_PREFIX=i686-elf
                ;;
                *)
                echo "error: unkown architecture '$2'"
                exit 1
                ;;
            esac    
            shift
            shift
            ;;

        -g|--gdb)
            USE_GDB=true
            shift
            ;;

        *)
            echo "error: unkown option '$1'"
            echo ""
            help
            exit 1
    esac
done

# Exectue commads
config # Always needed since some makefiles depends on varibles defined here 
case $cmd in
    clean)
        clean 
        ;;
    build)
        build_iso
        ;;
    qemu)
        build_iso
        qemu
        ;;
    *)
        echo "error: unkown command '$cmd'"
        echo ""
        help
        exit 1
        ;;
esac
