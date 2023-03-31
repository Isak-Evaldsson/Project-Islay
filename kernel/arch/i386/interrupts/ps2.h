#ifndef ARCH_i386_PS2_H
#define ARCH_i386_PS2_H

#define PS2_KEYBOARD_INTERRUPT 33
#define PS2_KEYBOARD_IRQ_NUM   1

/*
    Initialises the PS/2 Controller
*/
void ps2_init();

/*
    Reads the scancode from PS/2 device,
    assumes the data to be available
*/
unsigned char ps2_read_scancode();

#endif /* ARCH_i386_PS2_H */
