DEFAULT_HOST:=$(shell ../default-host.sh)
HOST?=$(DEFAULT_HOST)
HOSTARCH:=$(shell ../target-triplet-to-arch.sh $(HOST))

CFLAGS?= -g
CPPFLAGS?=
LDFLAGS?=
LIBS?=

DESTDIR?=
PREFIX?=/usr/local
EXEC_PREFIX?=$(PREFIX)
BOOTDIR?=$(EXEC_PREFIX)/boot
INCLUDEDIR?=$(PREFIX)/include

CFLAGS:=$(CFLAGS) -ffreestanding -Wall -Wextra
CPPFLAGS:=$(CPPFLAGS) -D__is_kernel -I.
LDFLAGS:=$(LDFLAGS)
LIBS:=$(LIBS) -nostdlib -lgcc

ARCHDIR=arch/$(HOSTARCH)

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
klib/heap_allocator.o \
klib/kpanic.o \
klib/kprintf.o \
klib/kreadline.o \
klib/libc.o \
klib/log.o \
klib/sleep.o \
klib/__fwriter.o \
devices/input_manager.o \
devices/timer.o \
devices/ps2_keyboard.o \
memory/page_frame_manager.o \
memory/vmem_manager.o \
tasks/scheduler.o \
tasks/task_queue.o \
tasks/locking.o \

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

install: install-headers install-kernel

install-headers:
	mkdir -p $(DESTDIR)$(INCLUDEDIR)
	cp -rp include/. $(DESTDIR)$(INCLUDEDIR)/.

install-kernel: $(KERNEL_ELF)
	mkdir -p $(DESTDIR)$(BOOTDIR)
	cp $(KERNEL_ELF) $(DESTDIR)$(BOOTDIR)

qemu: $(KERNEL_ELF) kernel_text_section
	qemu-system-$(HOSTARCH) $(QEMU_ARGS)

gdb: $(KERNEL_ELF) kernel_text_section 
	qemu-system-$(HOSTARCH) $(QEMU_ARGS)  -S -s &
	gdb $(KERNEL_ELF) \
        -ex 'target remote localhost:1234' \
        -ex 'layout src' \
        -ex 'layout regs' \
        -ex 'break *_start' \
        -ex 'continue'

cloc:
	cloc --exclude-dir=$(BUILD) --exclude-ext=d,md, .

-include $(OBJS:.o=.d)
