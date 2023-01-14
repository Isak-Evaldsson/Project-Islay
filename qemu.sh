#!/bin/sh
set -e
. ./iso.sh

qemu-system-$(./target-triplet-to-arch.sh $HOST) -d int -action reboot=shutdown -action shutdown=pause -cdrom islayos.iso
