# SPDX-License-Identifier: BSD-3-Clause
#
# See README.md and LICENSE.txt for license details.
#
# Copyright (C) 2025 Isak Evaldsson
#
#!/bin/bash

set -e 
source ./envsetup.sh

#
# Script downloading, building and installing binutils for cross compilation
#
BINUTILS_VERSION="2.45"
BINUTILS_NAME="binutils-$BINUTILS_VERSION"
DOWNLOAD_URL="https://ftp.gnu.org/gnu/binutils/binutils-$BINUTILS_VERSION.tar.gz"
SUB_DIR="binutils"
SRC_DIR="$TMP_DIR/$SUB_DIR/$BINUTILS_NAME"
OBJ_DIR="$TMP_DIR/$SUB_DIR/objects"

# Download and extract src code
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


"$SRC_DIR/configure" --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
echo $(MAKE) $(ARCH)
make -j $CORE_COUNT
make PREFIX=$PREFIX install
popd
