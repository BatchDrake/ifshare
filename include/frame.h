#ifndef _FRAME_H
#define _FRAME_H

#include <pthread.h>
#include <stdint.h>
#include <defs.h>

struct frame {
  pthread_mutex_t mutex;
  uint32_t        refcnt;
  struct frame   *next;

  size_t          size;
  size_t          alloc;

  uint8_t        *data; /* Public */
};

typedef struct frame frame_t;

INSTANCER(frame, size_t size);
COLLECTOR(frame);

METHOD(frame, bool, resize, size_t);
GETTER(frame, size_t, size);
GETTER(frame, size_t, allocation);

METHOD(frame, void, inc_ref);
METHOD(frame, bool, dec_ref);

#endif /* _FRAME_H */
