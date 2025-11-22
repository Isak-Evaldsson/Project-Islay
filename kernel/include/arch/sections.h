/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2025 Isak Evaldsson
*/
#ifndef SECTIONS_H
#define SECTIONS_H
#include <compiler.h>
#include <stdint.h>

#define GET_LINKER_SYMBOL(name) ({ extern int name; (uintptr_t)&name; })

#define TEXT_SECTION_START  GET_LINKER_SYMBOL(_text_start) 
#define TEXT_SECTION_END    GET_LINKER_SYMBOL(_text_end) 
#define DATA_SECTION_START  GET_LINKER_SYMBOL(_data_start) 
#define DATA_SECTION_END    GET_LINKER_SYMBOL(_data_end) 

#define PTR_CHECK_RANGE(ptr, start, end) ({\
    static_assert(IS_PTR(ptr)); \
    void *__start = ptr; \
    void *__end = __start + sizeof((*ptr)); \
    __start >= (void*)(start) && __end <= (void*)(end); })

#define IS_TEXT_PTR(ptr) PTR_CHECK_RANGE(ptr, TEXT_SECTION_START, TEXT_SECTION_END)
#define IS_DATA_PTR(ptr) PTR_CHECK_RANGE(ptr, DATA_SECTION_START, DATA_SECTION_END)

#endif /* SECTIONS_H */