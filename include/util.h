#ifndef _UTIL_H
#define _UTIL_H

#include "defs.h"
#include <stdarg.h>

#define PTR_LIST(type, name) \
  type **name##_list;        \
  unsigned int name##_count;

#define PTR_LIST_PRIVATE(type, name) \
  SUPRIVATE type **name##_list;      \
  SUPRIVATE unsigned int name##_count;

#define PTR_LIST_CONST(type, name) \
  const type **name##_list;        \
  unsigned int name##_count;

#define PTR_LIST_PRIVATE_CONST(type, name) \
  SUPRIVATE const type **name##_list;      \
  SUPRIVATE unsigned int name##_count;

#define PTR_LIST_LOCAL(type, name) \
  type **name##_list = NULL;       \
  unsigned int name##_count = 0;

#define PTR_LIST_EXTERN(type, name) \
  extern type **name##_list;        \
  extern unsigned int name##_count;

#define PTR_LIST_INIT(where, name) \
  where->name##_list = NULL;       \
  where->name##_count = 0;

#define PTR_LIST_APPEND(name, ptr) \
  ptr_list_append((void ***)&JOIN(name, _list), &JOIN(name, _count), ptr)

#define PTR_LIST_APPEND_CHECK(name, ptr) \
  ptr_list_append_check((void ***)&JOIN(name, _list), &JOIN(name, _count), ptr)

#define PTR_LIST_REMOVE(name, ptr) \
  ptr_list_remove_first((void ***)&JOIN(name, _list), &JOIN(name, _count), ptr)

#define FOR_EACH_PTR_STANDALONE(this, name)                             \
  unsigned int JOIN(_idx_, __LINE__);                                   \
  for (JOIN(_idx_, __LINE__) = 0; JOIN(_idx_, __LINE__) < name##_count; \
       JOIN(_idx_, __LINE__)++)                                         \
    if ((this = name##_list[JOIN(_idx_, __LINE__)]) != NULL)

#define FOR_EACH_PTR(this, where, name)                                        \
  unsigned int JOIN(_idx_, __LINE__);                                          \
  for (JOIN(_idx_, __LINE__) = 0; JOIN(_idx_, __LINE__) < where->name##_count; \
       JOIN(_idx_, __LINE__)++)                                                \
    if ((this = where->name##_list[JOIN(_idx_, __LINE__)]) != NULL)


char *vstrbuild(const char *fmt, va_list ap);
char *strbuild(const char *fmt, ...);

void ptr_list_append(void ***, unsigned int *, void *);
int  ptr_list_append_check(void ***, unsigned int *, void *);
int  ptr_list_remove_first(void ***, unsigned int *, void *);
int  ptr_list_remove_all(void ***, int *, void *);

#endif /* _UTIL_H */
