# SPDX-License-Identifier: BSD-3-Clause
#
# See README.md and LICENSE.txt for license details.
#
# Copyright (C) 2024 Isak Evaldsson
#
#!/usr/bin/env bash

set -e 
source ./envsetup.sh

#
# Script downloading, building and installing grup on platsforms that not
# come with grub pre-installed, such as macos 
#

GRUB_VERSION="2.14"
GRUB_NAME="grub-$GRUB_VERSION"
DOWNLOAD_URL="https://ftp.gnu.org/gnu/grub/$GRUB_NAME.tar.gz"
SUB_DIR="grub"
SRC_DIR="$TMP_DIR/$SUB_DIR/$GRUB_NAME"
OBJ_DIR="$TMP_DIR/$SUB_DIR/objects"

# Download grub src code
if [ ! -d $SRC_DIR ]; then
    echo "Downloading sources..."
    download_tar $DOWNLOAD_URL $SUB_DIR
fi

# Create objects dir
if [ -d $OBJ_DIR ]; then
    rm -rf $OBJ_DIR # Delete dir to avoid object missmatch
fi
mkdir -p $OBJ_DIR
pushd $OBJ_DIR

# grub needs gnulib and objconv, which is not preinstalled on macos
if [[ $OSTYPE == 'darwin'* ]]; then
  brew install objconv
  brew install automake
  brew install pkg-config
  brew install xorriso
fi

"$SRC_DIR/bootstrap"
"$SRC_DIR/autogen.sh"
exit 1

# Build grub in seperate folder
cd ..
mkdir build
cd build
../grub/configure --disable-werror TARGET_CC=i686-elf-grub TARGET_OBJCOPY=i686-elf-objcopy \
TARGET_STRIP=i686-elf-strip TARGET_NM=i686-elf-nm TARGET_RANLIB=i686-elf-ranlib --target=i686-elf
make

# Install grub
make install

# Remove temporary folder
rm -rf grub-tmp
