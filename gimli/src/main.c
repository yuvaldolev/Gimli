#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "gimli/cli.h"
#include "gimli/image_store.h"
#include "gimli/layer_store.h"
#include "stb_ds/stb_ds.h"

int main(int argc, const char *const argv[]) {
  int ret = 1;

  // Parse the command line arguments.
  Cli cli;
  if (0 != cli_init(&cli, argc, argv)) {
    cli_print_usage(argv[0]);
    goto out;
  }

  // Initialize the layer store.
  LayerStore layer_store;
  if (0 != layer_store_init(&layer_store)) {
    fprintf(stderr, "Failed initializing layer store, error(%d): [%s]\n", errno,
            strerror(errno));
    goto out_destroy_cli;
  }

  // Initialize the image store.
  ImageStore image_store;
  if (0 != image_store_init(&image_store)) {
    fprintf(stderr, "Failed initializing image store, error(%d): [%s]\n", errno,
            strerror(errno));
    goto out_destroy_layer_store;
  }

  ret = 0;

out_destroy_layer_store:
  layer_store_destroy(&layer_store);

out_destroy_cli:
  cli_destroy(&cli);

out:
  return ret;
}
