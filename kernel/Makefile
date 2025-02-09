# Initial flags defined in make.sh
CFLAGS:=$(CFLAGS) -ffreestanding -Wall -Wextra
CPPFLAGS:=$(CPPFLAGS) -D__is_kernel -I./include
LDFLAGS:=$(LDFLAGS)
LIBS:=$(LIBS) -nostdlib -lgcc

# Inherited from make.sh
ARCHDIR=arch/$(ARCH)
DRIVER_DIR=devices

include $(ARCHDIR)/make.config

CFLAGS:=$(CFLAGS) $(KERNEL_ARCH_CFLAGS)
CPPFLAGS:=$(CPPFLAGS) $(KERNEL_ARCH_CPPFLAGS)
LDFLAGS:=$(LDFLAGS) $(KERNEL_ARCH_LDFLAGS)
LIBS:=$(LIBS) $(KERNEL_ARCH_LIBS)

KERNEL_OBJS=\
$(KERNEL_ARCH_OBJS) \
main.o \
kshell.o \
ssp.o \
boot/init.o \
devices/tty.o \
devices/device.o \
devices/drivers.o \
devices/display/text_mode_display.o \
devices/input_manager.o \
devices/keyboard/keyboard.o\
devices/keyboard/ps2_keyboard.o\
devices/timer.o \
fs/file_operations.o \
fs/file_table.o \
fs/init.o \
fs/inode.o \
fs/mounting.o \
fs/pathwalker.o \
fs/pseudo_file.o \
fs/devfs/devfs.o \
fs/kinfo/kinfo.o \
fs/romfs/romfs.o\
memory/heap_allocator.o \
memory/page_frame_allocator.o \
memory/vmem_manager.o \
tasks/interrupts.o \
tasks/scheduler.o \
tasks/task_queue.o \
tasks/locking.o \
tests/fs_tests.o \
tests/interrupt_tests.o \
tests/post_boot_tests.o \
tests/scheduler_tests.o \
utils/endianness.o \
utils/kpanic.o \
utils/kprintf.o \
utils/libc.o \
utils/list.o \
utils/log.o \
utils/sleep.o \
utils/__fwriter.o \

OBJS=\
$(ARCHDIR)/crti.o \
$(ARCHDIR)/crtbegin.o \
$(KERNEL_OBJS) \
$(ARCHDIR)/crtend.o \
$(ARCHDIR)/crtn.o \

LINK_LIST=\
$(LDFLAGS) \
$(ARCHDIR)/crti.o \
$(ARCHDIR)/crtbegin.o \
$(KERNEL_OBJS) \
$(LIBS) \
$(ARCHDIR)/crtend.o \
$(ARCHDIR)/crtn.o \

BUILD=build
KERNEL_ELF=$(BUILD)/kernel.elf

QEMU_ARGS=-kernel $(KERNEL_ELF) -no-shutdown -no-reboot -d int -D $(BUILD)/log.txt -serial file:$(BUILD)/serial

.PHONY: all clean install install-headers install-kernel qemu gdb cloc
.SUFFIXES: .o .c .S

all: $(KERNEL_ELF)

$(KERNEL_ELF): $(BUILD) $(OBJS) $(ARCHDIR)/linker.ld
	$(CC) -T $(ARCHDIR)/linker.ld -o $@ $(CFLAGS) $(LINK_LIST)
	grub-file --is-x86-multiboot $(KERNEL_ELF)

$(ARCHDIR)/crtbegin.o $(ARCHDIR)/crtend.o:
	OBJ=`$(CC) $(CFLAGS) $(LDFLAGS) -print-file-name=$(@F)` && cp "$$OBJ" $@

.c.o:
	$(CC) -MD -c $< -o $@ -std=gnu11 $(CFLAGS) $(CPPFLAGS)

.S.o:
	$(CC) -MD -c $< -o $@ $(CFLAGS) $(CPPFLAGS)

$(BUILD):
	mkdir $(BUILD)

kernel_text_section: $(KERNEL_ELF)
	objdump -S -dj .text  $(KERNEL_ELF) > $(BUILD)/kernel_text_section  

clean:
	rm -rf $(BUILD)
	rm -f $(OBJS) *.o */*.o */*/*.o
	rm -f $(OBJS:.o=.d) *.d */*.d */*/*.d 

# Note, the install targets depends on DESTDIR, INCLUDEDIR and BOOTDIR which are
# inherited from make.sh
install: install-headers install-kernel

# Do only install the userspaces headers from uapi
install-headers:
	mkdir -p $(DESTDIR)$(INCLUDEDIR)
	cp -rp include/uapi/. $(DESTDIR)$(INCLUDEDIR)/.

install-kernel: $(KERNEL_ELF) kernel_text_section 
	mkdir -p $(DESTDIR)$(BOOTDIR)
	cp $(KERNEL_ELF) $(DESTDIR)$(BOOTDIR)

cloc:
	cloc --exclude-dir=$(BUILD) --exclude-ext=d,md, .

-include $(OBJS:.o=.d)
