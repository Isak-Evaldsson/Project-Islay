/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#ifndef KLIB_RING_BUFFER_H
#define KLIB_RING_BUFFER_H
/*
    Macros to define/use a fixed size statically allocated ring buffer.
    Designed to be used for smaller queues with stack allocated elements.
*/
#include <stddef.h>

/*
    Macros defining the ring buffer struct
*/
#define ring_buff_struct(TYPE, SIZE) ring_buff_type(, TYPE, SIZE)

#define ring_buff_type(NAME, TYPE, SIZE) \
    struct NAME {                        \
        size_t size;                     \
        size_t read_idx;                 \
        size_t write_idx;                \
        size_t cap;                      \
        TYPE   array[SIZE];              \
    }

/*
    Initialises the ring buffer
*/
#define ring_buff_init(buff)                                               \
    do {                                                                   \
        (buff).size      = 0;                                              \
        (buff).read_idx  = 0;                                              \
        (buff).write_idx = 0;                                              \
        (buff).cap       = sizeof((buff).array) / sizeof((buff).array[0]); \
    } while (0)

#define ring_buff_init_with_cap(buff, capacity) \
    do {                                        \
        (buff).size      = 0;                   \
        (buff).read_idx  = 0;                   \
        (buff).write_idx = 0;                   \
        (buff).cap       = capacity;            \
    } while (0)

/*
    Marcos allowing us to check the state of the buffer
*/
#define ring_buff_size(buff)  (buff).size
#define ring_buff_empty(buff) ((buff).size == 0)
#define ring_buff_full(buff)  ((buff).size == (buff).cap)
#define ring_buff_first(buff) ((buff).array[(buff.read_idx)])

/*
    Pushes element elm to the buffer.
    NOTE: The macro does not check if buffer is full before pushing.
*/
#define ring_buffer_push(buff, elem)                                          \
    do {                                                                      \
        (buff).array[(buff).write_idx] = elem;                                \
        (buff).write_idx               = ((buff).write_idx + 1) % (buff).cap; \
        (buff).size++;                                                        \
    } while (0)

/*
    Pops element from buffer and stores it in variable var.
    NOTE: The macro does not check if buffer is empty before pop
*/
#define ring_buffer_pop(buff, var)                            \
    do {                                                      \
        (var)           = ring_buff_first(buff);              \
        (buff).read_idx = ((buff).read_idx + 1) % (buff).cap; \
        (buff).size--;                                        \
    } while (0)

#define ring_buffer_pop(buff)                                               \
    ({                                                                      \
        typeof((buff).array[0]) _item = ring_buff_first((buff));            \
        (buff).read_idx               = ((buff).read_idx + 1) % (buff).cap; \
        (buff).size--;                                                      \
        _item;                                                              \
    })

#define ring_buffer_rest(buff) \
    do {                       \
        (buff).read_idx  = 0;  \
        (buff).write_idx = 0;  \
    } while (0)

#endif /* KLIB_RING_BUFFER_H */
