/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2025 Isak Evaldsson
*/
#include <list.h>

/* Removes list_entry from it's list */
void list_entry_remove(struct list_entry* entry)
{
    // Remove form list
    entry->prev->next = entry->next;
    entry->next->prev = entry->prev;

    // Ensure the removed entry to still be circular
    entry->next = entry->prev = entry;
}

/* Append new_entry to entry within the list, new_entry may itself be a head of circular list */
void list_entry_append(struct list_entry* entry, struct list_entry* new_entry)
{
    // Adjust the circle to include the new nodes
    entry->next->prev     = new_entry->prev;
    new_entry->prev->next = entry->next;

    // Create link entry <--> new_entry
    entry->next     = new_entry;
    new_entry->prev = entry;
}

/*
    Specialization of list_entry_append, if one knows that new_entry is a single element (i.e next
   and prev points to itself), we can save some memory lookups.
*/
void list_entry_append_single_element(struct list_entry* entry, struct list_entry* new_entry)
{
    // new_entry->prev == new_entry simply the circle inclusion operations
    entry->next->prev = new_entry;
    new_entry->next   = entry->next;

    // Create link entry <--> new_entry
    entry->next     = new_entry;
    new_entry->prev = entry;
}

/* Add entry to the start of the list */
void list_add_first(struct list* list, struct list_entry* entry)
{
    list_entry_append_single_element(&list->head, entry);
}

/* Add item to end of list */
void list_add_last(struct list* list, struct list_entry* entry)
{
    list_entry_append_single_element(list->head.prev, entry);
}

/* Remove first item from list, returns NULL if empty */
struct list_entry* list_remove_first(struct list* list)
{
    struct list_entry* e;

    if (LIST_EMPTY(list)) {
        return NULL;
    }

    e = list->head.next;
    list_entry_remove(e);
    return e;
}

/* Remove last item from list, returns NULL if empty */
struct list_entry* list_remove_last(struct list* list)
{
    struct list_entry* e;

    if (LIST_EMPTY(list)) {
        return NULL;
    }

    e = list->head.prev;
    list_entry_remove(e);
    return e;
}
