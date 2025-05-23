/*
  client.c: Client API
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

#define _DEFAULT_SOURCE

#include <client.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <util.h>
#include <arpa/inet.h>
#include <unistd.h>

static void *
client_thread(void *userdata)
{
  client_t *self = (client_t *) userdata;
  frame_t *frame = NULL;
  char ack;
  struct pollfd fds[2];
  bool running = true;

  fds[0].fd = self->cancelfd[0];
  fds[0].events = POLLIN;

  fds[1].fd = self->sfd;
  fds[1].events = POLLOUT;

  while (running && (frame = fqueue_pop_frame(self->queue)) != NULL) {
    ssize_t got,      p = 0;
    ssize_t size        = frame->size;
    const uint8_t *data = frame->data;

    struct timeval otv, tv, diff;

    otv = frame->timestamp;

    do {
      poll(fds, 2, 1000);

      if (fds[0].revents & POLLIN) {
        read(self->cancelfd[0], &ack, 1);
        Info("[%16s] Cancel request\n", self->name);
        running = false;
      } else if (fds[1].revents & POLLOUT) {
        got = send(self->sfd, data + p, size - p, MSG_NOSIGNAL);
        if (got <= 0) {
          Warn("[%16s] Client vanished\n", self->name);
          running = false;
        } else {
          p += got;
        }
      } else {
        gettimeofday(&tv, NULL);
        timersub(&tv, &otv, &diff);

        if (diff.tv_sec > 0)
          Info(
            "[%16s] Client slow (next frame is %ld.%06d s old)\n",
            self->name,
            diff.tv_sec,
            diff.tv_usec);
      }
    } while (running && p < size);

    frame_dec_ref(frame);
  }

done:
  self->thread_running = false;
  return NULL;
}

INSTANCER(client, int sfd, char *name)
{
  client_t *new = NULL;

  ALLOCATE_FAIL(new, client_t);

  new->sfd         = sfd;
  new->cancelfd[0] = -1;
  new->cancelfd[1] = -1;

  MAKE_FAIL(new->queue, fqueue);

  if (name != NULL) {
    TRY_FAIL(new->name = strdup(name));
  } else {
    struct sockaddr_in addr;
    socklen_t socklen;

    TRYC_FAIL(getsockname(sfd, (struct sockaddr *) &addr, &socklen));
    TRY_FAIL(
      new->name = strbuild(
        "%s:%d",
        inet_ntoa(addr.sin_addr),
        ntohs(addr.sin_port)));
  }

  TRYC_FAIL(pipe(new->cancelfd));
  TRYZ_FAIL(pthread_create(&new->client_thread, NULL, client_thread, new));
  
  new->thread_running = true;
  new->thread_started = true;
  
  Info("[%16s] New client\n", new->name);

  return new;

fail:
  if (new != NULL)
    DISPOSE(client, new);

  return NULL;
}

COLLECTOR(client)
{
  if (self->thread_started) {
    if (self->thread_running) {
      char b = 1;
      write(self->cancelfd[1], &b, 1); /* Force cancellation */
    }

    pthread_join(self->client_thread, NULL);
  }

  if (self->name != NULL)
    free(self->name);

  if (self->cancelfd[0] != -1)
    close(self->cancelfd[0]);
  else if (self->cancelfd[1] != -1)
    close(self->cancelfd[1]);

  if (self->queue != NULL)
    DISPOSE(fqueue, self->queue);
  
  close(self->sfd);

  free(self);
}

METHOD(client, bool, push_frame, frame_t *frame)
{
  if (frame != NULL)
    frame_inc_ref(frame);

  return fqueue_push_frame(self->queue, frame);
}
