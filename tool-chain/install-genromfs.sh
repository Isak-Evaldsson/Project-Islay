#!/bin/bash

#
# Script downloading, building and installing genromfs
#

DOWNLOAD_URL="http://downloads.sourceforge.net/project/romfs/genromfs/0.5.2/genromfs-0.5.2.tar.gz"
TMP_DIR="genromfs-tmp"
ZIP="$TMP_DIR/genromfs.tar.gz"

# create temporary working dir
mkdir $TMP_DIR

# download src files
if ! curl -L --output $ZIP $DOWNLOAD_URL; then
    echo "Failed to download genromfs.tar.gz"
    rm -rf $TMP_DIR
    exit 1
fi

# extract
tar -xzf $ZIP -C $TMP_DIR

# build/install
sudo make -C "$TMP_DIR/genromfs-0.5.2" install

# cleanup
rm -rf $TMP_DIR
