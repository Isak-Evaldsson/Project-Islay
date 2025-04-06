/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2025 Isak Evaldsson
*/
#include <list.h>

#include "test.h"

/* Ensure the list to be correctly linked */
static int verify_list(struct list* list)
{
    struct list_entry* e;
    struct list_entry* prev    = &list->head;
    bool               failure = false;

    LIST_ITER(list, e)
    {
        // verify the link prev <--> e
        if (prev->next != e) {
            TEST_LOG("List illformated: prev->next != e, prev %x, e %x", prev, e);
            failure = true;
        }

        if (e->prev != prev) {
            TEST_LOG("List illformated: e->prev != prev, prev %x, e %x", prev, e);
            failure = true;
        }

        prev = e;
    }

    if (failure) {
        TEST_LOG("Dumping list %x:", list);
        LIST_ITER(list, e)
        {
            TEST_LOG("Found entry %x, with prev %x, next %x", e, e->prev, e->next);
        }
        return -1;
    }

    return 0;
}

static int test_add_and_remove()
{
    DEFINE_LIST(l);
    struct list_entry  entries[5];
    struct list_entry* e;

    for (size_t i = 0; i < 5; i++) {
        e = entries + i;
        list_add_first(&l, e);
    }
    TEST_ERRNO_FUNC(verify_list(&l));

    do {
        e = list_remove_last(&l);
    } while (e);
    TEST_RETURN_IF_FALSE(LIST_EMPTY(&l));
    TEST_RETURN_IF_FALSE(l.head.next == l.head.prev);
    return 0;
}

static int test_ordering()
{
    DEFINE_LIST(l);
    struct list_entry first;
    struct list_entry last;
    struct list_entry mid;

    // Add in specific order
    list_add_first(&l, &first);
    list_add_last(&l, &mid);
    list_add_last(&l, &last);

    TEST_RETURN_IF_FALSE(l.head.next == &first);
    TEST_RETURN_IF_FALSE(l.head.prev == &last);
    TEST_ERRNO_FUNC(verify_list(&l));

    // Deletion...
    TEST_RETURN_IF_FALSE(list_remove_first(&l) == &first);
    TEST_RETURN_IF_FALSE(list_remove_last(&l) == &last);

    list_entry_remove(&mid);
    TEST_RETURN_IF_FALSE(LIST_EMPTY(&l));
    return 0;
}

static struct test_func list_tests[] = {
    CREATE_TEST_FUNC(test_add_and_remove),
    CREATE_TEST_FUNC(test_ordering),
};

struct test_suite list_test_suite = {
    .name     = "list_tests",
    .setup    = NULL,
    .teardown = NULL,
    .tests    = list_tests,
    .n_tests  = COUNT_ARRAY_ELEMS(list_tests),
};
