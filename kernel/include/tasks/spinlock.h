/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2024 Isak Evaldsson
*/
#ifndef TASK_SPINLOCK_H
#define TASK_SPINLOCK_H

#define SPINLOCK_INIT()       {.flag = 0}
#define SPINLOCK_DEFINE(name) struct spinlock name = SPINLOCK_INIT()

/*
 * Classic spinlock, ensures mutual exclusion and non-preemption, works in both
 * irq and thread context.
 *
 * To be used for protecting critical, but fast, sections.
 */
struct spinlock {
    unsigned int flag;
};

/* Lock spinlock */
void spinlock_lock(struct spinlock *spinlock, uint32_t *irqflags);

/* Unlock spinlock */
void spinlock_unlock(struct spinlock *spinlock, uint32_t irqflags);

#endif /* TASK_SPINLOCK_H */
