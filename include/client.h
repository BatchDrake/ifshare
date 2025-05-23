/*
  client.h: Client API
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

#ifndef _CLIENT_H
#define _CLIENT_H

#include <pthread.h>

#include "fqueue.h"

struct client {
  int   sfd;
  int   cancelfd[2];
  char *name;

  fqueue_t *queue;

  pthread_t client_thread;
  bool      thread_started;
  bool      thread_running;
};

typedef struct client client_t;

INSTANCER(client, int sfd, char *);
COLLECTOR(client);

METHOD(client, bool, push_frame, frame_t *);

METHOD(client, static inline bool, running)
{
  return self->thread_running;
}

#endif /* _CLIENT_H */
