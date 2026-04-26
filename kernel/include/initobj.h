/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2025 Isak Evaldsson
*/
#ifndef INITOBJ_H
#define INITOBJ_H
#include <compiler.h>
#include <list.h>

enum {
   /* Object type, assigns data->obj */
   INITOBJ_TYPE_TEST,
   INITOBJ_TYPE_FS,
   INITOBJ_LAST_OBJ_TYPE = INITOBJ_TYPE_FS,

   /* Functioon type, assings data->fn */
   INITOBJ_TYPE_COUNT
};


typedef int (*init_fn)(void);

struct init_object {
   unsigned int type;
   union {
      void *obj;
      init_fn fn;
   } data;
   struct list_entry entry;
};

#define __INITOBJ_NAME(prefix, type, tag) __initobj_##prefix##_##type_##tag


#define __INITOBJ_GENERIC(_type, _tag, _field, _data)                                  \
   static_assert(_type < INITOBJ_TYPE_COUNT);                                          \
   static volatile struct init_object __INITOBJ_NAME(struct, _type, _tag) = {          \
      .type = _type,                                                                   \
      .data._field = _data };                                                          \
      SECTION(".initobjs") const struct init_object *__INITOBJ_NAME(ptr, _type, _tag)= \
      (struct init_object*)&__INITOBJ_NAME(struct, _type, _tag);

/*
 * Init objects - Allows a object to be stored for proccessing upon init.
 * The object is guaranteed to only be called once, when is determined by
 * the type. The combination of tag + type must be unique.
 */
#define DEFINE_INITOBJ(_type, _tag, _obj) \
   static_assert(_type <= INITOBJ_LAST_OBJ_TYPE); \
   __INITOBJ_GENERIC(_type, _tag, obj, _obj)

/*
 * Function to be called during init, when is determined by the type. 
 * The function name must be unique for the specific type. 
 */
#define DEFINE_INITFUNC(_type, _fn) \
   static_assert(_type > INITOBJ_LAST_OBJ_TYPE); \
   __INITOBJ_GENERIC(_type, _tag, fn, _fn)

/*
 * Iterates over all init objects of the given type, executing the supplied handler
 * on each object. Each object is only executed once, calling this twice with the
 * same type will done nothting
 */
int call_init_objects(unsigned int type, int (*handler)(void*));

int call_init_functions(unsigned int type);

void parse_init_section(struct init_object **init_start, struct init_object **init_end);

#endif /* INITOBJ_H */
