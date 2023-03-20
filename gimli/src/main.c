#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "gimli/cli.h"
#include "gimli/image_store.h"
#include "gimli/layer_store.h"
#include "gimli/uuid.h"
#include "stb_ds/stb_ds.h"

int main(int argc, const char *const argv[]) {
  int ret = 1;

  // Initialize the random seed.
  srand((unsigned int)(time(NULL)));

  // Parse the command line arguments.
  Cli cli;
  if (0 != cli_init(&cli, argc, argv)) {
    cli_print_usage(argv[0]);
    goto out;
  }

  // Initialize the layer store.
  printf("=> initializing layer store... ");

  LayerStore layer_store;
  if (0 != layer_store_init(&layer_store)) {
    printf("failed, error(%d): [%s]\n", errno, strerror(errno));
    goto out_destroy_cli;
  }

  printf("done\n");

  // Initialize the image store.
  printf("=> initializing image store... ");

  ImageStore image_store;
  if (0 != image_store_init(&image_store)) {
    printf("failed, error(%d): [%s]\n", errno, strerror(errno));
    goto out_destroy_layer_store;
  }

  printf("done\n");

  // Generate the container hostname.
  printf("=> generating container hostname... ");

  char *container_hostname;
  if (0 != uuid_generate(&container_hostname)) {
    printf("failed, error(%d): [%s]\n", errno, strerror(errno));
    goto out_destroy_image_store;
  }

  printf("%s... done\n", container_hostname);

  ret = 0;

  free(container_hostname);

out_destroy_image_store:
  image_store_destroy(&image_store);

out_destroy_layer_store:
  layer_store_destroy(&layer_store);

out_destroy_cli:
  cli_destroy(&cli);

out:
  return ret;
}
