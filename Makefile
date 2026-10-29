# SPDX-License-Identifier: BSD-3-Clause
#
# See README.md and LICENSE.txt for license details.
#
# Copyright (C) 2026 Isak Evaldsson
#
include build/macros.mk

# Load target specific configuration before initializing the build system
ifndef TARGET
$(error No TARGET variable provided to makefile)
endif

TARGET_CONFIG := targets/$(TARGET)
$(call check_file, $(TARGET_CONFIG))
include $(TARGET_CONFIG)

include build/build.mk

.PHONY: clean kernel initrd toolchain qemu

$(TARGET_COMPILER):
	$(error No '$(TOOLCHAIN_TARGET)' toolchain installed. Run 'make TARGET=<target> toolchain' to build it)

kernel: $(TARGET_COMPILER)
	cd kernel && $(MAKE) all

initrd: kernel
	cd initrd && $(MAKE) all

clean:
	cd kernel && $(MAKE) clean
	cd initrd && $(MAKE) clean

toolchain:
	cd toolchain && $(MAKE) install

qemu: initrd
	./qemu.sh $(QEMU_VARIANT) $(TARGET_BOOT) $(OUT_DIR)/qemu $(GDB)
