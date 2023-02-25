#!/bin/sh
set -e
. ./iso.sh

qemu-system-$(./target-triplet-to-arch.sh $HOST) -action reboot=shutdown -d int -action shutdown=pause -cdrom islayos.iso
