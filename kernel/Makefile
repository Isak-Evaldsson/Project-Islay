# Initial flags inherited from top level makefile
CFLAGS:=$(CFLAGS) -ffreestanding -Wall -Wextra
CPPFLAGS:=$(CPPFLAGS) -D__is_kernel -I./include
LDFLAGS:=$(LDFLAGS)
LIBS:=$(LIBS) -nostdlib -lgcc

ARCHDIR := arch/$(ARCH)
DRIVER_DIR := devices
KERNEL_OUT := $(HOST_DIR)/kernel

include $(ARCHDIR)/make.config

KERNEL_SRC += \
	main.c \
	kshell.c \
	ssp.c \
	boot/init.c \
	devices/builtin_bus.c \
	devices/bus.c \
	devices/tty.c \
	devices/device.c \
	devices/display/text_mode_display.c \
	devices/input_manager.c \
	devices/keyboard/keyboard.c \
	devices/keyboard/keymaps.c \
	devices/keyboard/ps2_keyboard.c\
	devices/timer.c \
	fs/file_operations.c \
	fs/file_table.c \
	fs/init.c \
	fs/inode.c \
	fs/mounting.c \
	fs/pathwalker.c \
	fs/pseudo_file.c \
	fs/devfs/devfs.c \
	fs/kinfo/kinfo.c \
	fs/romfs/romfs.c \
	memory/bootmem_allocator.c \
	memory/heap_allocator.c \
	memory/memory_map.c \
	memory/page_frame_allocator.c \
	memory/vmem_manager.c \
	tasks/interrupts.c \
	tasks/scheduler.c \
	tasks/tasks.c \
	tasks/task_queue.c \
	tasks/locking.c \
	tests/fs_tests.c \
	tests/interrupt_tests.c \
	tests/list_tests.c \
	tests/post_boot_tests.c \
	tests/scheduler_tests.c \
	utils/endianness.c \
	utils/initobj.c \
	utils/kpanic.c \
	utils/kprintf.c \
	utils/libc.c \
	utils/list.c \
	utils/log.c \
	utils/sleep.c \
	utils/__fwriter.c \

KERNEL_OBJS += $(addprefix $(KERNEL_OUT)/, $(patsubst %.S,%.o, $(patsubst %.c,%.o, $(KERNEL_SRC))))
KERNEL_ELF  := $(KERNEL_OUT)/kernel.elf
KERNEL_TEXT := $(KERNEL_OUT)/kernel_text_section

UAPI_DIR     := include/uapi
UAPI_HEADERS := $(shell find $(UAPI_DIR) -name '*.h')
UAPI_LIB     := $(subst $(UAPI_DIR), $(TARGET_LIB), $(UAPI_HEADERS))

.PHONY: all clean

all: $(TARGET_BOOT)/kernel.elf $(UAPI_LIB)

$(KERNEL_ELF): $(KERNEL_OBJS) $(ARCHDIR)/linker.ld
	$(TARGET_CC) -T $(ARCHDIR)/linker.ld -o $@ $(CFLAGS) $(LDFLAGS) $(KERNEL_OBJS) $(LIBS)

$(KERNEL_OUT)/%.o: %.c
	mkdir -p $(dir $@)
	$(TARGET_CC) -MD -c $< -o $@ -std=gnu11 $(CFLAGS) $(CPPFLAGS)

$(KERNEL_OUT)/%.o: %.S
	mkdir -p $(dir $@)
	$(TARGET_CC) -MD -c $< -o $@ $(CFLAGS) $(CPPFLAGS)

$(KERNEL_TEXT): $(KERNEL_ELF)
	objdump -S -dj .text  $(KERNEL_ELF) > $(KERNEL_TEXT)
	objdump -S -dj .multiboot.text  $(KERNEL_ELF) > $(KERNEL_OUT)/kernel_multiboot_text_section

$(TARGET_BOOT)/kernel.elf: $(KERNEL_ELF) $(KERNEL_TEXT)
	mkdir -p $(TARGET_BOOT)
	cp $(KERNEL_ELF) $(TARGET_BOOT)

# TODO: Currently, if a uapi file is removed, the rule is re-run, but we don't delete anything
$(UAPI_LIB): $(UAPI_DIR)
	mkdir -p $(TARGET_LIB)
	cp -r $(UAPI_DIR)/. $(TARGET_LIB)/.

clean:
	rm -rf $(KERNEL_OUT)

-include $(KERNEL_OBJS:.o=.d)
