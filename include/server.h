/*
  server.h: Server implementation
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


#ifndef _SERVER_H
#define _SERVER_H

#include <util.h>
#include <client.h>
#include <pthread.h>

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
