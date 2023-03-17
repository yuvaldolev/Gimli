#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "gimli/cli.h"
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

  for (ptrdiff_t i = 0; i < shlen(layer_store.layers); ++i) {
    LayerItem *item = &(layer_store.layers[i]);
    printf("%s: Layer{ chain_id=%s, diff_id=%s, cache_id=%s }\n", item->key,
           item->value.chain_id, item->value.diff_id, item->value.cache_id);
  }

  // TODO: Move to a cleanup label.
  layer_store_destroy(&layer_store);

  ret = 0;

out_destroy_cli:
  cli_destroy(&cli);

out:
  return ret;
}
