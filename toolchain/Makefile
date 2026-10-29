# SPDX-License-Identifier: BSD-3-Clause
#
# See README.md and LICENSE.txt for license details.
#
# Copyright (C) 2026 Isak Evaldsson
#
DEPENDENCIES := \
	install \
	objconv \
	automake \
	pkg-config \
	xorriso \
	qemu-system-i386 \
	gdb \

$(foreach package, $(DEPENDENCIES), \
	$(if $(shell which $(package)),, $(error '$(package)' not available. Please install it with your OS package distribution)))

INSTALL_ENV_VARIABLES := \
	TARGET=$(TOOLCHAIN_TARGET) \
	TMP_DIR=$(TOOLCHAIN_DIR)/tmp \
	PREFIX=$(TOOLCHAIN_PREFIX)

.PHONY: install

$(GENROMFS):
	$(INSTALL_ENV_VARIABLES) ./install-genromfs.sh

$(TARGET_COMPILER):
	$(INSTALL_ENV_VARIABLES) ./install-binutils.sh
	$(INSTALL_ENV_VARIABLES) ./install-gcc.sh

install: $(TARGET_COMPILER) $(GENROMFS)
