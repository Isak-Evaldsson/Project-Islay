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
# Script downloading, building and installing gcc cross compiler
#
GCC_VERSION="14.2.0"
GCC_NAME="gcc-$GCC_VERSION"
DOWNLOAD_URL="https://ftp.gnu.org/gnu/gcc/$GCC_NAME/$GCC_NAME.tar.gz"
SUB_DIR="gcc"
SRC_DIR="$TMP_DIR/$SUB_DIR/$GCC_NAME"
OBJ_DIR="$TMP_DIR/$SUB_DIR/objects"

# Download and extract src code
if [ ! -d $SRC_DIR ]; then
    echo "Downloading sources..."
    download_tar $DOWNLOAD_URL $SUB_DIR
fi

# Create objects dir
mkdir -p $OBJ_DIR
pushd $OBJ_DIR

# Build and install
"$SRC_DIR/configure" --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers --disable-hosted-libstdcxx

make -j $CORE_COUNT all-gcc
make -j $CORE_COUNT all-target-libgcc
make -j $CORE_COUNT all-target-libstdc++-v3
make install-gcc
make install-target-libgcc
make install-target-libstdc++-v3
popd
