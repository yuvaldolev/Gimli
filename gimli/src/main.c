#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "cli.h"
#include "layer_store.h"

int main(int argc, const char *const argv[]) {
  int ret = 1;

  // Parse the command line arguments.
  Cli cli;
  if (0 != cli_init(&cli, argc, argv)) {
    cli_print_usage(argv[0]);
    goto out;
  }

  printf("Image: %s\n", cli.image);
  printf("Command: [");
  for (size_t index = 0; index < cli.command_size; ++index) {
    printf("\"%s\"", cli.command[index]);
    if ((cli.command_size - 1) > index) {
      printf(", ");
    }
  }
  printf("]\n");

  // Initialize the layer store.
  LayerStore layer_store;
  if (0 != layer_store_init(&layer_store)) {
    fprintf(stderr, "Failed initializing the layer store, error(%d): [%s]",
            errno, strerror(errno));
    goto out_destroy_cli;
  }

  ret = 0;

out_destroy_cli:
  cli_destroy(&cli);

out:
  return ret;
}
