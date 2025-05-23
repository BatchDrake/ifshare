#ifndef _SERVER_H
#define _SERVER_H

#include <util.h>
#include <client.h>
#include <pthread.h>

#define IFSHARE_SERVER_PORT 5665
#define IFSHARE_MAX_MTU     4096

struct server {
  int listenfd;
  int cancelfd[2];
  PTR_LIST(client_t, client);
  pthread_mutex_t client_mutex;

  pthread_t acceptor_thread;
  bool      thread_started;
  bool      thread_running;
};

typedef struct server server_t;

INSTANCER(server);
COLLECTOR(server);

METHOD(server, bool, loop, const char *);

#endif /* _SERVER_H */
