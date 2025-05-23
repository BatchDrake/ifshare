/*
  frame.h: Frame API
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

#ifndef _FRAME_H
#define _FRAME_H

#include <pthread.h>
#include <stdint.h>
#include <defs.h>
#include <sys/time.h>

struct frame {
  struct timeval  timestamp;
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
