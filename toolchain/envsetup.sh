# SPDX-License-Identifier: BSD-3-Clause
#
# See README.md and LICENSE.txt for license details.
#
# Copyright (C) 2025 Isak Evaldsson
#
#!/usr/bin/env bash

#
# Common variables and functions used by multiple scripts
#
function error {
    echo "error: $1"
    exit 1
}

# Make sure the build environments is properly setup by caller
[[ -z "${TARGET}"  ]] && error "Missing TARGET env variable"
[[ -z "${PREFIX}"  ]] && error "Missing PREFIX env variable"
[[ -z "${TMP_DIR}" ]] && error "Missing TOOLCHAIN_TMP env variable"

if [[ "$OSTYPE" == "darwin"* ]]; then
	CORE_COUNT=$(sysctl -n hw.logicalcpu)
else 
	CORE_COUNT=$(nproc)
fi
MAKE="make -j $CORE_COUNT"

mkdir -p $TMP_DIR
mkdir -p $PREFIX

# Downloads the file at $1 and extract it into $TMP_DIR/$2
function download_tar {
    if [ "$#" -lt 2 ]; then
        echo "Missing argument <url> <subdir>"
    fi

    local url=$1
    local subdir="$TMP_DIR/$2"
    if [[ $url == *.gz ]]; then
        local tar_path="$subdir/tmp.tar.gz"
        local comp_arg="--gzip"
    else
        local tar_path="$subdir/tmp.tar.xz"
        local comp_arg="--xz"
    fi
    mkdir -p $subdir

    if ! curl -L --output $tar_path $DOWNLOAD_URL; then
        echo "Failed to download tar at $1"
        exit 1
    fi

    tar -xf $tar_path -C $subdir $comp_arg
}
