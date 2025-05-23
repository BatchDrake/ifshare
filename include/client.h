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
