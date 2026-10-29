# SPDX-License-Identifier: BSD-3-Clause
#
# See README.md and LICENSE.txt for license details.
#
# Copyright (C) 2026 Isak Evaldsson
#
include build/macros.mk

ifeq (0,${MAKELEVEL})

# Ensure a proper target configuration to be loaded
$(call check_defined, TARGET ARCH)

# Top level output directories for generated files
export TOP_DIR	:=	$(CURDIR)
export OUT_DIR	:=	$(TOP_DIR)/out
export HOST_DIR	:=	$(OUT_DIR)/host

# Target file system directories
export TARGET_DIR	:=	$(OUT_DIR)/target
export TARGET_BOOT	:=	$(TARGET_DIR)/boot
export TARGET_LIB	:=	$(TARGET_DIR)/usr/lib

# Toolchain/cross-compiler setup
export TOOLCHAIN_DIR	:=	$(OUT_DIR)/toolchain
export TOOLCHAIN_PREFIX :=	$(TOOLCHAIN_DIR)/bin/cross
export TOOLCHAIN_TARGET :=	$(ARCH)-elf
export TARGET_COMPILER	:= $(TOOLCHAIN_PREFIX)/bin/$(TOOLCHAIN_TARGET)-gcc 
export TARGET_CC := \
	$(TARGET_COMPILER) \
	--sysroot=$(TOOLCHAIN_PREFIX)/lib/gcc/$(TOOLCHAIN_TARGET)/15.2.0 \
	-isystem =/plugin/include \

export GENROMFS := $(TOOLCHAIN_PREFIX)/usr/bin/genromfs
else
$(error Should only be included by root makefile)
endif