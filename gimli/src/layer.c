#include "gimli/layer.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gimli/gimli_directory.h"
#include "gimli/io.h"

static int init_link_path(Layer *self) {
  int ret = 1;

  // Read the link file.
  char link_file_path[PATH_MAX];
  snprintf(link_file_path, sizeof(link_file_path), "%s/overlay2/%s/link",
           gimli_directory_get(), self->cache_id);

  // Read the link name.
  char *link_name;
  if (0 != io_file_to_string(link_file_path, &link_name)) {
    goto out;
  }

  // Calculate the link path size.
  // 1 is added for the null terminator which is appended by sprintf but
  // is not included in its return value.
  size_t link_path_size = (size_t)snprintf(NULL, 0, "%s/overlay2/l/%s",
                                           gimli_directory_get(), link_name) +
                          1;

  // Allocate a buffer to store link path.
  self->link_path = malloc(link_path_size);
  if (NULL == self->link_path) {
    goto out_free_link_name;
  }

  // Format the link path,.
  snprintf(self->link_path, link_path_size, "%s/overlay2/l/%s",
           gimli_directory_get(), link_name);

  ret = 0;

out_free_link_name:
  free(link_name);

out:
  return ret;
}

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

  // Initialize the layer's link path.
  if (0 != init_link_path(self)) {
    goto out_free_cache_id;
  }

  ret = 0;
  goto out;

out_free_cache_id:
  free(self->cache_id);

out_free_diff_id:
  free(self->diff_id);

out_free_chain_id:
  free(self->chain_id);

out:
  return ret;
}

void layer_destroy(Layer *self) {
  free(self->link_path);
  free(self->cache_id);
  free(self->diff_id);
  free(self->chain_id);
}
