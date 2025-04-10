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
BINUTILS_VERSION="2.44"
BINUTILS_NAME="binutils-$BINUTILS_VERSION"
DOWNLOAD_URL="https://ftp.gnu.org/gnu/binutils/binutils-2.44.tar.gz"
SUB_DIR="binutils"
SRC_DIR="$TMP_DIR/$SUB_DIR/$BINUTILS_NAME"
OBJ_DIR="$TMP_DIR/$SUB_DIR/objects"

# Download and extract src code
if [ ! -d $SRC_DIR ]; then
    echo "Downloading sources..."
    download_tar $DOWNLOAD_URL $SUB_DIR
fi

# Create objects dir
mkdir -p $OBJ_DIR
pushd $OBJ_DIR

"$SRC_DIR/configure" --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
make
make PREFIX=$PREFIX install
popd
