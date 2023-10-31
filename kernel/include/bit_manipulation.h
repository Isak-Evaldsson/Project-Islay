#ifndef KLIB_BIT_MANIPULATION_H
#define KLIB_BIT_MANIPULATION_H

#define SET_BIT(b, n)  ((b) |= (1 << (n)))
#define CLR_BIT(b, n)  ((b) &= ~((1) << (n)))
#define MASK_BIT(b, n) ((b) & (1 << n))

#endif /* KLIB_BIT_MANIPULATION_H */
