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

export PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig"

# But first we need to build three gun libs which gcc depeends on
# LibGMP
GMP_VERSION="6.3.0"
GMP_NAME="gmp-$GMP_VERSION"
DOWNLOAD_URL="https://ftp.gnu.org/gnu/gmp/$GMP_NAME.tar.gz"
SUB_DIR="gmp"
SRC_DIR="$TMP_DIR/$SUB_DIR/$GMP_NAME"
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

$SRC_DIR/configure --prefix="$PREFIX" --with-pic
$MAKE
$MAKE install

# LibMPRF
MPFR_VERSION="4.2.2"
MPFR_NAME="mpfr-$MPFR_VERSION"
DOWNLOAD_URL="https://ftp.gnu.org/gnu/mpfr/$MPFR_NAME.tar.gz"
SUB_DIR="mpfr"
SRC_DIR="$TMP_DIR/$SUB_DIR/$MPFR_NAME"
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

$SRC_DIR/configure CFLAGS="$(pkg-config --cflags-only-I gmp)" LDFLAGS="$(pkg-config --libs-only-L gmp)" --prefix="$PREFIX" --with-pic

$MAKE
$MAKE install

# LibMPC
MPC_VERSION="1.4.1"
MPC_NAME="mpc-$MPC_VERSION"
DOWNLOAD_URL="https://ftp.gnu.org/gnu/mpc/$MPC_NAME.tar.xz"
SUB_DIR="mpc"
SRC_DIR="$TMP_DIR/$SUB_DIR/$MPC_NAME"
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

$SRC_DIR/configure CFLAGS="$(pkg-config --cflags-only-I gmp mpfr)" LDFLAGS="$(pkg-config --libs-only-L gmp mpfr)" --prefix=$PREFIX --with-pic 

$MAKE
$MAKE install

# Time to build the actual compiler...
GCC_VERSION="15.2.0"
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
if [ -d $OBJ_DIR ]; then
    rm -rf $OBJ_DIR # Delete dir to avoid object missmatch
fi
mkdir -p $OBJ_DIR
pushd $OBJ_DIR

# Build and install
"$SRC_DIR/configure" --with-system-zlib --with-gmp=$PREFIX --with-mpfr=$PREFIX --with-mpc=$PREFIX --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers --disable-hosted-libstdcxx

$MAKE all-gcc
$MAKE all-target-libgcc
$MAKE all-target-libstdc++-v3
$MAKE install-gcc
$MAKE install-target-libgcc
$MAKE install-target-libstdc++-v3
popd
