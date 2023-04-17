#!/bin/bash
#
# Script downloading, building and installing grup on platsforms that not
# come with grub pre-installed, such as macos 
#

# Download grub src code
mkdir grub-tmp
cd grub-tmp
git clone git://git.savannah.gnu.org/grub.git
cd grub

# grub needs gnulib and objconv, which is not preinstalled on macos
if [[ $OSTYPE == 'darwin'* ]]; then
  brew install objconv
  brew install automake
  brew install pkg-config
  brew install xorriso
  ./bootstrap
fi
./autogen.sh

# Build grub in seperate folder
cd ..
mkdir build
cd build
../grub/configure --disable-werror TARGET_CC=i686-elf-gcc TARGET_OBJCOPY=i686-elf-objcopy \
TARGET_STRIP=i686-elf-strip TARGET_NM=i686-elf-nm TARGET_RANLIB=i686-elf-ranlib --target=i686-elf
make

# Install grub
make install

# Remove temporary folder
rm -rf grub-tmp
