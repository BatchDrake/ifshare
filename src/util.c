#include <util.h>
#include <stdio.h>
#include <stdarg.h>
#include <defs.h>

#define STRBUILD_BSIZ 16

int
is_asciiz(const char *buf, int lbound, int ubound)
{
  register int i;

  for (i = lbound; i < ubound; i++)
    if (!buf[i])
      return i + 1;
  return 0;
}

char *
vstrbuild(const char *fmt, va_list ap)
{
  char *out = NULL, *tmp = NULL;
  char *result = NULL;

  int size, zeroindex;
  int last;
  va_list copy;

  last = 0;

  if (fmt != NULL) {
    if (!*fmt) {
      result = strdup("");
      goto done;
    }

    va_copy(copy, ap);
    size = vsnprintf(NULL, 0, fmt, copy) + 1;
    va_end(copy);

    if ((out = malloc(size)) == NULL)
      goto done;

    va_copy(copy, ap);
    vsnprintf(out, size, fmt, copy);
    va_end(copy);

    for (;;) {
      if ((zeroindex = is_asciiz(out, last, size)) != 0)
        break;

      last = size;
      size += STRBUILD_BSIZ;

      tmp = realloc(out, size);
      if (tmp == NULL)
        goto done;

      out = tmp;

      va_copy(copy, ap);
      vsnprintf(out, size, fmt, copy);
      va_end(copy);
    }

    result = out;
    out = NULL;
  }

done:
  if (out != NULL)
    free(out);

  return result;
}

char *
strbuild(const char *fmt, ...)
{
  char *out;
  va_list ap;

  va_start(ap, fmt);
  out = vstrbuild(fmt, ap);
  va_end(ap);

  return out;
}


/* Para manipular arrays de punteros */
int
ptr_list_append_check(void ***list, unsigned int *count, void *new)
{
  unsigned int i;
  void **reallocd_list;

  for (i = 0; i < *count; i++)
    if ((*list)[i] == NULL)
      break;

  if (i == *count) {
    if ((reallocd_list = realloc(*list, (1 + *count) * sizeof(void *)))
        == NULL)
      return -1;
    else {
      ++(*count);
      *list = reallocd_list;
    }
  }

  (*list)[i] = new;

  return i;
}

void
ptr_list_append(void ***list, unsigned int *count, void *new)
{
  (void) ptr_list_append_check(list, count, new);
}

int
ptr_list_remove_first(void ***list, unsigned int *count, void *ptr)
{
  unsigned int i;
  int found;

  found = 0;

  for (i = 0; i < *count; i++)
    if ((*list)[i] == ptr || ptr == NULL) {
      (*list)[i] = NULL;
      found++;

      break;
    }

  return found;
}

int
ptr_list_remove_all(void ***list, int *count, void *ptr)
{
  int i;
  int found;

  found = 0;

  for (i = 0; i < *count; i++)
    if ((*list)[i] == ptr || ptr == NULL) {
      (*list)[i] = NULL;
      found++;
    }

  return found;
}

