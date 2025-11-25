/* SPDX-License-Identifier: BSD-3-Clause

   See README.md and LICENSE.txt for license details.

   Copyright (C) 2025 Isak Evaldsson
*/
#ifndef INITOBJ_H
#define INITOBJ_H
#include <compiler.h>
#include <list.h>

enum {
   INITOBJ_TYPE_TEST,
   INITOBJ_TYPE_FS,
   INITOBJ_TYPE_COUNT
};

typedef void (*initobj_fn)(void *arg);

struct init_object {
   unsigned int type;
   void *arg;
   initobj_fn fn; 
   struct list_entry entry;
};

#define __INITOBJ_NAME(prefix, type, tag) __initobj_##prefix##_##type_##tag

/*
 * Init objects - Allows a function to be called with the supplied argument during the
 * init process. The object is guaranteed to only be called once, when is determined by 
 * the type. The combination of tag + type must be unique.
 */
#define DEFINE_INITOBJ(_type, _tag, _fn, _arg) \
   static_assert(_type < INITOBJ_TYPE_COUNT); \
   static volatile struct init_object __INITOBJ_NAME(struct, _type, _tag) = { \
      .type = _type, \
      .arg = _arg,  \
      .fn = _fn }; \
      SECTION(".initobjs") const struct init_object *__INITOBJ_NAME(ptr, _type, _tag)= \
      (struct init_object*)&__INITOBJ_NAME(struct, _type, _tag);


typedef void (*init_fn)(void);

/*
 * Function to be called during init, when is determined by the type. 
 * The function name must be unique for the specific type. 
 */
#define DEFINE_INITFUNC(type, fn) \
   static_assert(IS_SAME_TYPE(&fn, init_fn)); \
   DEFINE_INITOBJ(type, fn, (initobj_fn)fn, NULL)

/*
 * Iterates over all init objects of the given type, each object is only executed
 * once, calling this twice with the same type will done nothting 
 */
void call_init_objects(unsigned int type);

void parse_init_section(struct init_object **init_start, struct init_object **init_end);

#endif /* INITOBJ_H */
