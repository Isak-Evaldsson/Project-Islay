/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2025 Isak Evaldsson
*/
#ifndef LIST_H
#define LIST_H

#include <stddef.h>

/*
    Generic list implementation

    Embedded the list_entry within a struct and use the supplied list
    api to add/remove these objects to a list object, eg.

    struct test {
        ...
        struct list_entry entry;
    };

    struct list test_list = LIST_INIT();
    list_add(&test_list, elem);
    ...
    struct test* first = GET_STRUCT(struct test, entry, test_list.head);
*/

/* The node representing each entry within the list. */
struct list_entry {
    struct list_entry* next;
    struct list_entry* prev;
};

/* Object storing the state of the list */
struct list {
    struct list_entry* head;
    struct list_entry* tail;
};

/* Ensure a list to be properly initialised */
#define LIST_INIT() {.head = NULL, .tail = NULL}

/* Check if list is empty */
#define LIST_EMPTY(list_ptr)                 \
    ({                                       \
        struct list* _list_ptr = (list_ptr); \
        _list_ptr->head == NULL;             \
    })

/* Get first item in list, or NULL if empty */
#define LIST_FIRST(list_ptr)                 \
    ({                                       \
        struct list* _list_ptr = (list_ptr); \
        _list_ptr->head;                     \
    })

/* Add item to end of list */
void list_add(struct list* list, struct list_entry* entry);

/* Remove item from list */
void list_remove(struct list* list, struct list_entry* entry);

/* Macro to simplify list iteration */
#define LIST_ITER(list_ptr, entry_ptr) \
    for (entry_ptr = (list_ptr)->head; entry_ptr != NULL; entry_ptr = entry_ptr->next)

#endif /* LIST_H */
