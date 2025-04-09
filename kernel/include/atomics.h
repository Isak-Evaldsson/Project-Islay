/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#ifndef ATOMICS_H
#define ATOMICS_H
#include <stdint.h>

/*
    Atomic types and operation

    Since this kernel priorities correctness over performance, all atomic operations implements the
    sequential consistency memory order (i.e. inserts an implicit  memory barrier that prevents
    re-ordering of both read and writes).
*/

/*
    The preferred atomic types, using the platform specific integer/pointer size
*/
typedef struct {
    int __atomic;
} atomic_int_t;

typedef struct {
    unsigned int __atomic;
} atomic_uint_t;

typedef struct {
    uintptr_t __atomic;
} atomic_ptr_t;

/*
    The 64-bit atomics should be avoided since they may be quite slow on 32-bit platforms. On x86
    for example, RMW operations becomes a cmpxchg-loop.
*/
typedef struct {
    int64_t __atomic;
} atomic_int64_t;

typedef struct {
    uint64_t __atomic;
} atomic_uint64_t;

#define ATOMIC_INIT() {.__atomic = 0}

/* Atomically loads value */
#define atomic_load(atomic_ptr) __atomic_load_n(&(atomic_ptr)->__atomic, __ATOMIC_SEQ_CST)

/* Atomically stores value */
#define atomic_store(atomic_ptr, val)                                       \
    ({                                                                      \
        typeof((*atomic_ptr).__atomic) __val = (val); /* type check val */  \
        __atomic_store_n(&(atomic_ptr)->__atomic, __val, __ATOMIC_SEQ_CST); \
    })

/* Atomically adds val from current value and returns the result */
#define atomic_add_fetch(atomic_ptr, val)                                     \
    ({                                                                        \
        typeof((*atomic_ptr).__atomic) __val = (val); /* type check val */    \
        __atomic_add_fetch(&(atomic_ptr)->__atomic, __val, __ATOMIC_SEQ_CST); \
    })

/* Atomically subtracts val from current value and returns the result */
#define atomic_sub_fetch(atomic_ptr, val)                                     \
    ({                                                                        \
        typeof((*atomic_ptr).__atomic) __val = (val); /* type check val */    \
        __atomic_sub_fetch(&(atomic_ptr)->__atomic, __val, __ATOMIC_SEQ_CST); \
    })

/* Atomically performs AND to the current value with val and returns the result */
#define atomic_and_fetch(atomic_ptr, val)                                     \
    ({                                                                        \
        typeof((*atomic_ptr).__atomic) __val = (val); /* type check val */    \
        __atomic_and_fetch(&(atomic_ptr)->__atomic, __val, __ATOMIC_SEQ_CST); \
    })

/* Atomically performs OR to the current value with val and returns the result */
#define atomic_or_fetch(atomic_ptr, val)                                     \
    ({                                                                       \
        typeof((*atomic_ptr).__atomic) __val = (val); /* type check val */   \
        __atomic_or_fetch(&(atomic_ptr)->__atomic, __val, __ATOMIC_SEQ_CST); \
    })

/* Replace the current value within the atomic with val, returning the previous value */
#define atomic_exchange(atomic_ptr, val)                                       \
    ({                                                                         \
        typeof((*atomic_ptr).__atomic) __val = (val); /* type check val */     \
        __atomic_exchange_n(&(atomic_ptr)->__atomic, __val, __ATOMIC_SEQ_CST); \
    })

/* Checks if the atomic contains the expected value, if true val is written to the atomic, if false,
 * the value of the atomic is read into expected. Returns weather or not it succeeded */
#define atomic_compare_exchange(atomic_ptr, expected_ptr, val)                           \
    ({                                                                                   \
        /* Type check expected and val */                                                \
        typeof((*atomic_ptr).__atomic) *__expected = (expected_ptr);                     \
        typeof((*atomic_ptr).__atomic)  __val      = (val);                              \
        __atomic_compare_exchange_n(&(atomic_ptr)->__atomic, __expected, __val, false, \
                                      __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);               \
    })

/*
    Memory barriers, allowing non-atomic memory operations to be synchronized across threads
*/
#define mem_barrier_acquire() __atomic_thread_fence(__ATOMIC_ACQUIRE)
#define mem_barrier_release() __atomic_thread_fence(__ATOMIC_RELEASE)
#define mem_barrier_full()    __atomic_thread_fence(__ATOMIC_SEQ_CST)

/*
    READ_/WRITE_ONCE macros ensures that the compiler actually performs a read or write
    operation, disallowing it from trying to be clever reusing registers. Recommend when
    performing when reads or writes to global variables.
*/

/*
    Ensures that the read become an actual memory read instruction, the compiler are not
    allowed to optimize it by re-using a register value.

    This is enured by first converting the address of the variable to a volatile pointer
    that is than read. Since volatile variables might change unexpectedly, there value might
    have changed since the last access, so the compiler must generate a read to ensure that
    it gets the correct value.
*/
#define READ_ONCE(var) (*(volatile typeof(var) *)&var)

/*
    Ensures that the write become an actual memory write instruction, the compiler are not
    allowed to optimize it by only writing to a register.

    This is enured by first converting the address of the variable to a volatile pointer
    that is than write to. Since volatile variables might be read unexpectedly, so the
    compiler must generate a write to ensure that it propagates to those readers.
*/
#define WRITE_ONCE(var, val) (*(volatile typeof(var) *)&var = (val))

#endif /* ATOMICS_H */
