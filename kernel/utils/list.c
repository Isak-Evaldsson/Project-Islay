/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2025 Isak Evaldsson
*/
#include <list.h>

void list_add(struct list* list, struct list_entry* entry)
{
    entry->next = NULL;
    entry->prev = list->tail;

    if (list->tail) {
        list->tail->next = entry;
    }

    list->tail = entry;

    if (!list->head) {
        list->head  = entry;
        entry->prev = NULL;
    }
}

void list_remove(struct list* list, struct list_entry* entry)
{
    if (entry->prev) {
        entry->prev->next = entry->next;
    } else {
        list->head = entry->next;
    }

    if (entry->next) {
        entry->next->prev = entry->prev;
    } else {
        list->tail = entry->prev;
    }
}
