#include "layer.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "io.h"

static int read_layer_property_file(const char *chain_id,
                                    const char *layer_store_directory,
                                    const char *property_file_name,
                                    char **out_data) {
  // Format the file's path.
  char path[PATH_MAX];
  snprintf(path, sizeof(path), "%s/%s/%s", layer_store_directory, chain_id,
           property_file_name);

  // Read the file.
  if (0 != file_to_string(path, out_data)) {
    return 1;
  }

  return 0;
}

int layer_init(Layer *self, const char *chain_id,
               const char *layer_store_directory) {
  int ret = 1;

  // Initialize the layer's chain ID.
  size_t chain_id_size = strlen(chain_id) + 1;

  self->chain_id = malloc(chain_id_size);
  if (NULL == self->chain_id) {
    goto out;
  }

  strncpy(self->chain_id, chain_id, chain_id_size);

  // Read the layer's diff ID.
  if (0 != read_layer_property_file(chain_id, layer_store_directory, "diff",
                                    &self->diff_id)) {
    goto out_free_chain_id;
  }

  // Read the layer's cache ID.
  if (0 != read_layer_property_file(chain_id, layer_store_directory, "cache-id",
                                    &self->cache_id)) {
    goto out_free_diff_id;
  }

  ret = 0;
  goto out;

out_free_diff_id:
  free(self->diff_id);

out_free_chain_id:
  free(self->chain_id);

out:
  return ret;
}

void layer_destroy(Layer *self) {
  free(self->cache_id);
  free(self->diff_id);
  free(self->chain_id);
}
