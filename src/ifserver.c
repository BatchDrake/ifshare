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

  Info("Server started\n");

  TRY(server_loop(server, argv[1]));

  code = EXIT_SUCCESS;

done:
  if (server != NULL)
    DISPOSE(server, server);

  exit(code);
}
