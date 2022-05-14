.PHONY: compile link

compile:
	i686-elf-gcc -std=gnu11 -ffreestanding -g -c start.s -o start.o
	i686-elf-gcc -std=gnu11 -ffreestanding -g -c kernel.c -o kernel.o

link: compile
	i686-elf-gcc -ffreestanding -nostdlib -g -T linker.ld start.o kernel.o -o mykernel.elf -lgcc

run: link
	qemu-system-i386 -kernel mykernel.elf	