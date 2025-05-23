/*
  fqueue.h: Frame queue API
  Copyright (C) 2025 Gonzalo José Carracedo Carballal
  
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation, version 3.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program.  If not, see
  <http://www.gnu.org/licenses/>

*/

#ifndef _FQUEUE_H
#define _FQUEUE_H

#include <pthread.h>
#include <sys/time.h>
#include "frame.h"

struct fqueue_frame {
  struct fqueue_frame *prev;
  struct fqueue_frame *next;

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
