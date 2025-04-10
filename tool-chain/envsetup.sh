# SPDX-License-Identifier: BSD-3-Clause
#
# See README.md and LICENSE.txt for license details.
#
# Copyright (C) 2025 Isak Evaldsson
#
#!/bin/bash

#
# Common variables and functions used by multiple scripts
#

# General configuration
ROOT=$(pwd)
TMP_DIR="$ROOT/tmp"
BIN_DIR="$ROOT/bin"
PREFIX="$BIN_DIR/cross"
ARCH="i686" # What architecture are we building for
TARGET="$ARCH-elf" # What architecture are we building for
PATH="$PREFIX/bin:$PATH" # Ensure that the tools in prefix is used during buildsteps
CORE_COUNT=$(nproc)

mkdir -p $BIN_DIR
mkdir -p $TMP_DIR
mkdir -p $PREFIX

# Downloads the file at $1 and extract it into $TMP_DIR/$2
function download_tar {
    if [ "$#" -lt 2 ]; then
        echo "Missing argument <url> <subdir>"
    fi

    local url=$1
    local subdir="$TMP_DIR/$2"
    local tar_path="$subdir/tmp.tar.gz"
    mkdir -p $subdir

    if ! curl -L --output $tar_path $DOWNLOAD_URL; then
        echo "Failed to download tar at $1"
        exit 1
    fi

    tar -xzf $tar_path -C $subdir
}
