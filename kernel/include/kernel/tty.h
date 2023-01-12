#ifndef _KERNEL_TTY_H
#define _KERNEL_TTY_H

#include <stddef.h>

void term_init();
void term_put(char c);
void term_write(const char *data, size_t size);
void term_writestring(const char *data);

#endif
