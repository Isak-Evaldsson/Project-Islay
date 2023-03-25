#ifndef ARCH_TTY_H
#define ARCH_TTY_H

#include <stddef.h>

extern const int TERM_WIDTH;
extern const int TERM_HEIGHT;

void term_init();
void term_put(char c);
void term_write(const char *data, size_t size);
void term_writestring(const char *data);
void term_clear();

#endif /* ARCH_TTY_H */
