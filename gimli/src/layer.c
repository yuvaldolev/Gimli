#include "gimli/layer.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gimli/io.h"

static int read_layer_property_file(const char *chain_id,
                                    const char *layer_store_directory,
                                    const char *property_file_name,
                                    char **out_data) {
  // Format the file's path.
  char path[PATH_MAX];
  snprintf(path, sizeof(path), "%s/%s/%s", layer_store_directory, chain_id,
           property_file_name);

  // Read the file.
  if (0 != io_file_to_string(path, out_data)) {
    return 1;
  }

  return 0;
}

int layer_init(Layer *self, const char *chain_id,
               const char *layer_store_directory) {
  int ret = 1;

  // Initialize the chain ID.
  self->chain_id = strdup(chain_id);
  if (NULL == self->chain_id) {
    goto out;
  }

  // Read the diff ID.
  if (0 != read_layer_property_file(chain_id, layer_store_directory, "diff",
                                    &self->diff_id)) {
    goto out_free_chain_id;
  }

  // Read the cache ID.
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
