/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2025 Isak Evaldsson
*/
#ifndef COMPILER_H
#define COMPILER_H
#include <typeclass.h>

#ifndef __GNUC__
#error Islay kernel currently relies on gcc specific extensions
#endif

#define IS_PTR(ptr) (__builtin_classify_type((ptr)) == pointer_type_class)
#define IS_FUNC(func) (__builtin_classify_type((func)) == function_type_class)
#define IS_CONSTEXPR(expr) (__builtin_constant_p(expr))
#define IS_SAME_TYPE(expr1, expr2) (__builtin_types_compatible_p(typeof(expr1), typeof(expr2)))

#define SECTION(sec) [[gnu::section(sec)]]
#endif /* COMPILER_H */