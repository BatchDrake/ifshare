/*
  server.c: Server implementation
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

#include <ifshare.h>

#include <linux/if_packet.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ether.h>

#include <server.h>

#include <sys/poll.h>
#include <util.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>


METHOD(server, static void, cleanup_clients)
{
  unsigned int i;

  pthread_mutex_lock(&self->client_mutex);

  for (i = 0; i < self->client_count; ++i) {
    if (self->client_list[i] != NULL && !client_running(self->client_list[i])) {
      DISPOSE(client, self->client_list[i]);
      self->client_list[i] = NULL;
    }
  }

  pthread_mutex_unlock(&self->client_mutex);
}

METHOD(server, static bool, accept_client)
{
  int sfd;
  bool ok = false;
  bool acquired = false;
  
  struct sockaddr_in addr;
  client_t *client = NULL;
  socklen_t len;

  TRYC(sfd = accept(self->listenfd, (struct sockaddr *) &addr, &len));

  MAKE(client, client, sfd, NULL);

  pthread_mutex_lock(&self->client_mutex);
  acquired = true;

  TRYC(PTR_LIST_APPEND_CHECK(self->client, client));
  client = NULL;

  ok = true;

done:
  if (acquired)
    pthread_mutex_unlock(&self->client_mutex);
  
  if (client != NULL)
    DISPOSE(client, client);
  
  return ok;
}

static void *
acceptor_thread(void *userdata)
{
  server_t *self = (server_t *) userdata;
  struct pollfd fds[2];
  char ack;

  fds[0].fd     = self->cancelfd[0];
  fds[0].events = POLLIN;

  fds[1].fd     = self->listenfd;
  fds[1].events = POLLIN;

  Info("Acceptor thread started\n");

  while (poll(fds, 2, 1000) != -1) {
    if (fds[0].revents & POLLIN) {
      read(self->cancelfd[0], &ack, 1);
      break;
    } else if (fds[1].revents & POLLIN) {
      TRY(server_accept_client(self));
    } else {
      server_cleanup_clients(self);
    }
  }

done:
  self->thread_running = false;
}

METHOD(server, static bool, broadcast, frame_t *frame)
{
  bool ok = false;
  unsigned int i;

  pthread_mutex_lock(&self->client_mutex);

  for (i = 0; i < self->client_count; ++i)
    if (self->client_list[i] != NULL && client_running(self->client_list[i]))
      TRY(client_push_frame(self->client_list[i], frame));

  ok = true;

done:
  pthread_mutex_unlock(&self->client_mutex);
  return ok;
}

METHOD(server, static bool, init_listener)
{
  struct sockaddr_in serv_addr;
  int reuse = 1;
  
  self->listenfd = socket(AF_INET, SOCK_STREAM, 0);

  if (self->listenfd == -1) {
    Err("socket(AF_INET, SOCK_STREAM, 0) failed: %s\n", strerror(errno));
    return false;
  }
  
  if (setsockopt(
    self->listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
    Err("set SO_REUSEADDR failed: %s\n", strerror(errno));
    close(self->listenfd);
    return false;
  }

  memset(&serv_addr, 0, sizeof(serv_addr));
   
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(IFSHARE_SERVER_PORT);
   
  if (bind(self->listenfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1) {
    Err("bind(self->listenfd, port = %d) failed: %s\n", IFSHARE_SERVER_PORT, strerror(errno));
    close(self->listenfd);
    return false;
  }
   
  if (listen(self->listenfd, 10) == -1) {
    Err("listen(self->listenfd, 10) failed: %s\n", strerror(errno));
    close(self->listenfd);
    return false;
  }

  return true;
}

INSTANCER(server)
{
  server_t *new = NULL;

  ALLOCATE_FAIL(new, server_t);

  new->cancelfd[0] = -1;
  new->cancelfd[1] = -1;
  new->listenfd    = -1;

  TRY_FAIL(server_init_listener(new));

  TRYC_FAIL(pipe(new->cancelfd));
  TRYZ_FAIL(pthread_create(&new->acceptor_thread, NULL, acceptor_thread, new));
  
  new->thread_running = true;
  new->thread_started = true;
  
  return new;

fail:
  if (new != NULL)
    DISPOSE(server, new);

  return NULL;
}

COLLECTOR(server)
{
  unsigned int i;

  if (self->thread_started) {
    if (self->thread_running) {
      char b = 1;
      write(self->cancelfd[1], &b, 1); /* Force cancellation */
    }

    pthread_join(self->acceptor_thread, NULL);
  }

  if (self->cancelfd[0] != -1)
    close(self->cancelfd[0]);
  else if (self->cancelfd[1] != -1)
    close(self->cancelfd[1]);

  pthread_mutex_destroy(&self->client_mutex);

  for (i = 0; i < self->client_count; ++i)
    if (self->client_list[i] != NULL)
      DISPOSE(client, self->client_list[i]);
  
  if (self->client_list != NULL)
    free(self->client_list);
  
  if (self->listenfd != -1)
    close(self->listenfd);

  free(self);
}

METHOD(server, static int, open_raw_socket, const char *eth)
{
  struct ifreq if_idx;
  struct ifreq if_mac;
  struct ifreq ifopts;
  struct packet_mreq mr;
  struct sockaddr_ll sll;

        
  int sockopt = -1;
  char if_name[IFNAMSIZ];

  int fd;
  bool ok = false;

  if ((fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) == -1) {
    Err("Failed to create raw socket: %s\n", strerror(errno));
    goto done;
  }

  /* Retrieve NIC properties */
  memset(if_name, 0, IFNAMSIZ);
  strncpy(if_name, eth, strlen(eth));

  memset(&if_idx, 0, sizeof(struct ifreq));
  strncpy(if_idx.ifr_name, if_name, IFNAMSIZ - 1);
  TRYC(ioctl(fd, SIOCGIFINDEX, &if_idx));
  
  memset(&if_mac, 0, sizeof(struct ifreq));
  strncpy(if_mac.ifr_name, if_name, IFNAMSIZ - 1);
  TRYC(ioctl(fd, SIOCGIFHWADDR, &if_mac));

  Info(
    "%s opened (hwaddr %02x:%02x:%02x:%02x:%02x:%02x)\n",
    eth,
    (uint8_t) if_mac.ifr_hwaddr.sa_data[0],
    (uint8_t) if_mac.ifr_hwaddr.sa_data[1],
    (uint8_t) if_mac.ifr_hwaddr.sa_data[2],
    (uint8_t) if_mac.ifr_hwaddr.sa_data[3],
    (uint8_t) if_mac.ifr_hwaddr.sa_data[4],
    (uint8_t) if_mac.ifr_hwaddr.sa_data[5]);

  memset(&sll, 0, sizeof(sll));
  sll.sll_family = AF_PACKET;
  sll.sll_ifindex = if_idx.ifr_ifindex;
  sll.sll_protocol = htons(ETH_P_ALL);
  
  if (bind(fd, (struct sockaddr *)&sll, sizeof(sll)) < 0) {
    Err("bind(link layer): %s\n", strerror(errno));
    goto done;
  }
  
  memset(&mr, 0, sizeof(mr));
  mr.mr_ifindex = if_idx.ifr_ifindex;
  mr.mr_type = PACKET_MR_PROMISC;
  if (setsockopt(fd, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr)) < 0) {
    Err("setsockopt(PACKET_ADD_MEMBERSHIP): %s\n", strerror(errno));
    goto done;
  }
        
  ok = true;

done:
  if (!ok && fd != -1) {
    close(fd);
    fd = -1;
  }

  return fd;
}

METHOD(server, bool, loop, const char *eth)
{
  bool ok = false;
  int rawfd = -1;
  ssize_t ret;
  struct pollfd fd;
  frame_t *frame = NULL;
  const char *message = "Hello world (broadcast)\n";

  TRYC(rawfd = server_open_raw_socket(self, eth));

  fd.fd = rawfd;
  fd.events = POLLIN;

  for (;;) {
    TRYC(poll(&fd, 1, -1));

    MAKE(frame, frame, IFSHARE_MAX_MTU + sizeof(struct ifshare_pdu));

    struct ifshare_pdu *pdu = (struct ifshare_pdu *) frame->data;

    pdu->is_magic = IFSHARE_MAGIC;

    ret = recv(rawfd, pdu->is_data, IFSHARE_MAX_MTU, 0);

    if (ret == -1) {
      Err("recv RAW failed: %s\n", strerror(errno));
      goto done;
    } else if (ret == 0) {
      Warn("Interface `%s' vanished\n", eth);
      break;
    }

    pdu->is_size = ret;

    //Debug("FRAME[0x%x, %d] (%d)\n", pdu->is_magic, pdu->is_size, sizeof(struct ifshare_pdu) + ret);

    TRY(frame_resize(frame, sizeof(struct ifshare_pdu) + ret));
    TRY(server_broadcast(self, frame));

    frame_dec_ref(frame);
  }

  ok = true;

done:
  if (rawfd != -1)
    close(rawfd);
  
  return ok;
}
