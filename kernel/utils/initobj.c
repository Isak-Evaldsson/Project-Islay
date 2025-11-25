/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2025 Isak Evaldsson
*/
#include <initobj.h>
#include <utils.h>

#define LOG(fmt, ...) __LOG(1, "[INITOBJ]", fmt, ##__VA_ARGS__)

static struct list init_objs[INITOBJ_TYPE_COUNT];

void parse_init_section(struct init_object **init_start, struct init_object **init_end)
{
    struct init_object **ptr, *obj;
    
    for (size_t i = 0; i < COUNT_ARRAY_ELEMS(init_objs); i++) {
        init_objs[i] = LIST_INIT(init_objs[i]);
    }

    for (ptr = init_start; ptr < init_end; ptr++) {
        obj = *ptr;
        if (!IS_DATA_PTR(obj)) {
            kpanic("init section contaning non-data pointer %x\n", obj);
        }
        
        if (obj->type >= INITOBJ_TYPE_COUNT) {
            kpanic("%x contains init object of invalid type %u", obj, obj->type);
        }

        if (!IS_TEXT_PTR(obj->fn)) {
            kpanic("%x contains init object with invalid function pointer %x", obj, obj->fn);
        }

        if (obj->arg && !IS_DATA_PTR(obj->arg)) {
            kpanic("%x contains init object with invalid argument pointer %x", obj, obj->arg);
        }

        list_add_last(init_objs + obj->type, &obj->entry);
    }
}

void call_init_objects(unsigned int type) 
{
    struct list_entry *entry;
    struct init_object *obj;
    
    if (type >= INITOBJ_TYPE_COUNT) {
        LOG("invalid type %u", type);
        return;
    }

    if (LIST_EMPTY(init_objs + type)) {
        LOG("no initobjects for this type %u, called twice?", type);
        return;
    }

    while ((entry = list_remove_first(init_objs + type))) {
        obj = GET_STRUCT(struct init_object, entry, entry);
        obj->fn(obj->arg);
    }
}
