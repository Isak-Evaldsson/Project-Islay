# SPDX-License-Identifier: BSD-3-Clause
#
# See README.md and LICENSE.txt for license details.
#
# Copyright (C) 2024 Isak Evaldsson
#
#!/bin/bash

set -e 
source ./envsetup.sh

#
# Script downloading, building and installing genromfs
#
GENROMFS_VERSION="0.5.2"
GENROMFS_NAME="genromfs-$GENROMFS_VERSION"
DOWNLOAD_URL="http://downloads.sourceforge.net/project/romfs/genromfs/$GENROMFS_VERSION/$GENROMFS_NAME.tar.gz"
SUB_DIR="genromfs"
SRC_DIR="$TMP_DIR/$SUB_DIR/$GENROMFS_NAME"

# Download and extract src code
if [ ! -d $SRC_DIR ]; then
    echo "Downloading sources..."
    download_tar $DOWNLOAD_URL $SUB_DIR
fi

# build/install
pushd $SRC_DIR
make -j $CORE_COUNT PREFIX=$PREFIX install
popd
