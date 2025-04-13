# SPDX-License-Identifier: BSD-3-Clause
#
# See README.md and LICENSE.txt for license details.
#
# Copyright (C) 2025 Isak Evaldsson
#
#!/bin/bash

set -e 

if [ $# -ne 1 ]; then
    echo "$0: missing argument <ARCH>"
    exit 1
fi

export ARCH=$1 # Must be set before calling envsetup
source ./envsetup.sh
echo "Installing toolchain for target $TARGET at $PREFIX"

# Platform specfic dependencies
case $OSTYPE in 
    'darwin'*)
        ./install-grub.sh
        ;;

    'linux-gnu')
        if ! command -v "apt" 2>&1 >/dev/null; then
            echo "Not a debain based distro, you need to handle depencies own your own"
        else
            sudo apt install "qemu-system-$ARCH" build-essential libmpfr-dev libgmp3-dev libmpc-dev xorriso mtools grub-pc-bin -y
        fi
        ;;
esac

# Building and installing tools from src
./install-binutils.sh
./install-gcc.sh
./install-genromfs.sh

echo "Toolchain installation complete!"
