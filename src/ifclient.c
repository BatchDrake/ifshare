/*
  ifclient.c: ifshare client application
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

#include <linux/if_packet.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ether.h>
#include <libnet.h>
#include <sys/poll.h>

#include <ifshare.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

static int
tcp_connect(const char *host, uint16_t port)
{
  struct sockaddr_in srvAddr;
  int csfd = -1;
  struct hostent *ent;
  struct in_addr *addr;
  bool ok = false;

  Info("Resolving `%s'...\n", host);

  if ((ent = gethostbyname(host)) == NULL) {
    switch (h_errno) {
      case HOST_NOT_FOUND:
        Err("Failed to resolve hostname `%s'\n", host);
        break;

      default:
        Err("Unexpected error while resolving (h_errno = %d)\n", h_errno);
    }

    goto done;
  }
  
  if ((addr = (struct in_addr *) ent->h_addr_list[0]) == NULL) {
    Err("Invalid response by gethostbyname()\n");
    goto done;
  }
  
  Info("Done, connecting to %s:%d...\n", inet_ntoa(*addr), port);

  srvAddr.sin_family = AF_INET;
  srvAddr.sin_addr   = *addr;
  srvAddr.sin_port   = htons(port);

  if ((csfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
    Err("Failed to create socket: %s\n", strerror(errno));
    goto done;
  }
  
  if (connect(
    csfd,
    (struct sockaddr *) &srvAddr,
    sizeof (struct sockaddr_in)) == -1) {
    Err("Failed to connect to host: %s\n", strerror(errno));
    goto done;
  }

  ok = true;

done:
  if (!ok && csfd != -1) {
    close(csfd);
    csfd = -1;
  }
  
  return csfd;
}


static int
open_tap(const char *tap)
{
  struct ifreq ifr;
  int fd = -1;
  int rfd = -1;
  bool ok = false;

  if (strlen(tap) + 1 >= IFNAMSIZ) {
    Err("Tap device name too long (must be up to %d characters)\n", IFNAMSIZ - 1);
    goto done;
  }

  if ((fd = open("/dev/net/tun", O_RDWR)) == -1) {
    Err("Cannot open /dev/net/tun: %s\n", strerror(errno));
    goto done;
  }

  memset(&ifr, 0, sizeof(ifr));

  ifr.ifr_flags = IFF_TAP | IFF_NO_PI;

  strncpy(ifr.ifr_name, tap, IFNAMSIZ);

  int res = ioctl(fd, TUNSETIFF, &ifr);
  if (res == -1 && errno != EBUSY) {
    Err("Cannot change name of network tap to `%s': %s\n", tap, strerror(errno));
    goto done;
  }

  if ((rfd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_IP))) == -1) {
    Err("Failed to create raw socket: %s\n", strerror(errno));
    goto done;
  }

  TRYC(ioctl(rfd, SIOCGIFFLAGS, &ifr));
  ifr.ifr_flags |= IFF_PROMISC | IFF_UP;
  TRYC(ioctl(rfd, SIOCSIFFLAGS, &ifr));

  ok = true;

done:
  if (rfd != -1)
    close(rfd);
  
  if (!ok && fd != -1) {
    close(fd);
    fd = -1;
  }

  return fd;
}

static bool
read_all(int fd, void *buf, size_t size)
{
  uint8_t *as_bytes = (uint8_t *) buf;
  ssize_t p = 0, got;

  while (p < size) {
    got = recv(fd, as_bytes + p, size - p, MSG_NOSIGNAL);
    if (got <= 0)
      return false;

    p += got;
  }

  return true;
}

static bool
consume_tap(int fd)
{
  struct pollfd fds;
  static uint8_t frame[IFSHARE_MAX_MTU];
  ssize_t size;

  fds.fd = fd;
  fds.events = POLLIN;

  while (poll(&fds, 1, IFSHARE_MAX_MTU) == 1) {
    if ((size = read(fd, frame, IFSHARE_MAX_MTU)) == -1) {
      Err("Failed to consume packet\n");
      return false;
    }

    log_hexdump(LogDebug, frame, size);
  }

  return true;
}

int
main(int argc, char *argv[])
{
  struct ifshare_pdu header;
  static uint8_t frame[IFSHARE_MAX_MTU];
  uint16_t port;
  const char *tap = "tap0";
  int tapfd = -1;
  int srvfd = -1;
  ssize_t written;
  int code = EXIT_FAILURE;

  if (argc < 3) {
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "\t %s HOST PORT [TAP]\n", argv[0]);
    goto done;
  }

  if (argc > 3)
    tap = argv[3];

  if (sscanf(argv[2], "%hu", &port) != 1) {
    Err("Invalid port `%s'\n", argv[2]);
    goto done;
  }

  Info("Ifshare version 0.1\n");
  Info("This is the IF client program\n");

  TRYC(tapfd = open_tap(tap));
  Info("Tap device opened: %s\n", tap);

  TRYC(srvfd = tcp_connect(argv[1], port));

  Info("Done. Forwarding frames to %s\n", tap);

  while (read_all(srvfd, &header, sizeof(struct ifshare_pdu))) {
    if (header.is_magic != IFSHARE_MAGIC) {
      Err("SERVER ERROR: Invalid PDU magic (0x%x)\n", header.is_magic);
      goto done;
    }

    if (header.is_size > IFSHARE_MAX_MTU) {
      Err("SERVER ERROR: Invalid PDU size\n");
      goto done;
    }

    if (!read_all(srvfd, frame, header.is_size)) {
      Err("SERVER ERROR: Failed to receive %d bytes\n", header.is_size);
      goto done;
    }

    if ((written = write(tapfd, frame, header.is_size)) != header.is_size) {
      if (written >= 0)
        break;
      
      Err(
        "write(%s): cannot write %d bytes: %s\n",
        tap,
        header.is_size,
        strerror(errno));
      goto done;
    }
  }

  code = EXIT_SUCCESS;

done:
  exit(code);
}
