/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#ifndef ARCH_PLATFORM_H
#define ARCH_PLATFORM_H

#ifdef __i686__
#define PLATFORM_IS_ARCH_i686() 1
#else
#define PLATFORM_IS_ARCH_i686() 0
#endif

#define ARCH(arch) (PLATFORM_IS_ARCH_##arch())

#endif /* ARCH_PLATFORM_H */
