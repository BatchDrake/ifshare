/*
  ifserver.c: ifshare server application
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
#include <server.h>

#include <stdio.h>

int
main(int argc, char *argv[])
{
  int code = EXIT_FAILURE;
  server_t *server = NULL;

  if (argc != 2) {
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "\t%s IFACE\n", argv[0]);
    goto done;
  }

  Info("Ifshare version 0.1\n");
  Info("This is the IF server program\n");

  MAKE(server, server);

  Info("Server started, listening on *:%d\n", IFSHARE_SERVER_PORT);

  TRY(server_loop(server, argv[1]));

  code = EXIT_SUCCESS;

done:
  if (server != NULL)
    DISPOSE(server, server);

  exit(code);
}
