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

  for (ptrdiff_t repository_to_id_pair_index = 0;
       repository_to_id_pair_index < shlen(image_store.repository_to_id);
       ++repository_to_id_pair_index) {
    const char *repository =
        image_store.repository_to_id[repository_to_id_pair_index].key;
    const char *id =
        image_store.repository_to_id[repository_to_id_pair_index].value;
    Image *image =
        &(image_store.id_to_image[shgeti(image_store.id_to_image, id)].value);

    printf("%s: Image{ id=%s, layers=[", repository, image->id);

    for (size_t layer_index = 0; layer_index < image->layers_size;
         ++layer_index) {
      printf("%s", image->layers[layer_index]);

      if (layer_index < (image->layers_size - 1)) {
        printf(", ");
      }
    }

    printf("] }\n");
  }

  ret = 0;

  image_store_destroy(&image_store);

out_destroy_layer_store:
  layer_store_destroy(&layer_store);

out_destroy_cli:
  cli_destroy(&cli);

out:
  return ret;
}
