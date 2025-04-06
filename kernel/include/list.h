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
    list_add_first(&test_list, elem);
    ...

    struct list_entry* e = list_remove_first(&test_list);
    struct test* first = GET_STRUCT(struct test, entry, e);

    // Remove entry we know are within the list
    struct list_entry* in_list = ...;

*/

/* The node representing each entry within the list. */
struct list_entry {
    struct list_entry* next;
    struct list_entry* prev;
};

/* Object storing the state of the list */
struct list {
    // Implemented as a circular linked list with the list struct acting as sentinel/dummy node,
    // so the first real element is head.next. This approach makes list manipulation faster since we
    // avoid the need of doing NULL checking. For an empty list the head.next == head.prev.
    struct list_entry head;
};

#define LIST_ENTRY_INIT(entry) {.next = &entry, .prev = &entry}

/* Ensure a list to be properly initialised */
#define LIST_INIT(l)                    \
    (struct list)                       \
    {                                   \
        .head = LIST_ENTRY_INIT(l.head) \
    }

/* Create a statically allocated list */
#define DEFINE_LIST(l) struct list l = {.head = LIST_ENTRY_INIT(l.head)}

/* Check if list is empty */
#define LIST_EMPTY(list_ptr)                      \
    ({                                            \
        struct list* _list_ptr = (list_ptr);      \
        _list_ptr->head.next == &_list_ptr->head; \
    })

/* Macro to simplify list iteration */
#define LIST_ITER(list_ptr, entry_ptr)                                      \
    for (entry_ptr = (list_ptr)->head.next; entry_ptr != &(list_ptr)->head; \
         entry_ptr = entry_ptr->next)

/*
    High level list operations, designed to be easy to use and safe
 */

/* Add entry to the start of the list */
void list_add_first(struct list* list, struct list_entry* entry);

/* Add item to end of list */
void list_add_last(struct list* list, struct list_entry* entry);

/* Remove first item from list, returns NULL if empty */
struct list_entry* list_remove_first(struct list* list);

/* Remove last item from list, returns NULL if empty */
struct list_entry* list_remove_last(struct list* list);

/*
    Operations on individual list entries, here it's up to the user to ensure that the links are
   consistent
*/

/* Removes list_entry from it's list */
void list_entry_remove(struct list_entry* entry);

/* Append new_entry to entry within the list, new_entry may itself be a head of circular list */
void list_entry_append(struct list_entry* entry, struct list_entry* new_entry);

/* Specialization of list_entry_append, if one knows that new_entry is a single element (i.e next
 * and prev points to itself), we can save some memory lookups. */
void list_entry_append_single_element(struct list_entry* entry, struct list_entry* new_entry);

#endif /* LIST_H */
