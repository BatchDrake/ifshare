#ifndef _FQUEUE_H
#define _FQUEUE_H

#include <pthread.h>
#include <sys/time.h>
#include "frame.h"

struct fqueue_frame {
  struct fqueue_frame *prev;
  struct fqueue_frame *next;
  struct timeval tv;

  frame_t *frame;
};

struct fqueue {
  pthread_mutex_t mutex;
  pthread_cond_t  cond;

  struct fqueue_frame *first;
  struct fqueue_frame *last;
  struct fqueue_frame *free;
};

typedef struct fqueue fqueue_t;

INSTANCER(fqueue);
COLLECTOR(fqueue);

METHOD(fqueue, bool, push_frame, frame_t *);
METHOD(fqueue, frame_t *, pop_frame);

#endif /* _FQUEUE_H */
