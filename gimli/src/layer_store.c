#include "gimli/layer_store.h"

#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "gimli/gimli_directory.h"
#include "gimli/layer.h"
#include "stb_ds/stb_ds.h"

static int read_layer_store(LayerStore *self, const char *path,
                            DIR *directory) {
  int ret = 1;

  // Reset the diff ID to layer map (required to be initialized to NULL by
  // stb_ds).
  self->diff_id_to_layer = NULL;

  for (;;) {
    // Set `errno` to 0 before reading the next directory entry.
    errno = 0;

    // Read the next directory entry.
    struct dirent *entry = readdir(directory);
    if (NULL == entry) {
      // Check if an error occurred while reading the directory entry.
      if (0 != errno) {
        goto out_free_diff_id_to_layer;
      }

      // Completed reading all directory entries.
      break;
    }

    // Skip non-directory entries.
    if (DT_DIR != entry->d_type) {
      continue;
    }

    // Skip the "." and ".." directories.
    if ((0 == strcmp(entry->d_name, ".")) ||
        (0 == strcmp(entry->d_name, ".."))) {
      continue;
    }

    // Read the layer information.
    Layer layer;
    if (0 != layer_init(&layer, entry->d_name, path)) {
      goto out_free_diff_id_to_layer;
    }

    // Add the layer to the diff ID to layer map.
    shput(self->diff_id_to_layer, layer.diff_id, layer);
  }

  ret = 0;
  goto out;

out_free_diff_id_to_layer:
  for (ptrdiff_t pair_index = 0; pair_index < shlen(self->diff_id_to_layer);
       ++pair_index) {
    layer_destroy(&(self->diff_id_to_layer[pair_index].value));
  }

  shfree(self->diff_id_to_layer);

out:
  return ret;
}

int layer_store_init(LayerStore *self) {
  int ret = 1;

  // Format the layer store directory path.
  char layer_store_directory_path[PATH_MAX];
  snprintf(layer_store_directory_path, sizeof(layer_store_directory_path),
           "%s/image/overlay2/layerdb/sha256", gimli_directory_get());

  // Open the layer store directory.
  DIR *layer_store_directory = opendir(layer_store_directory_path);
  if (NULL == layer_store_directory) {
    goto out;
  }

  // Read the layer store.
  if (0 != read_layer_store(self, layer_store_directory_path,
                            layer_store_directory)) {
    goto out_close_layer_store_directory;
  }

  ret = 0;

out_close_layer_store_directory:
  closedir(layer_store_directory);

out:
  return ret;
}

void layer_store_destroy(LayerStore *self) {
  for (ptrdiff_t pair_index = 0; pair_index < shlen(self->diff_id_to_layer);
       ++pair_index) {
    layer_destroy(&(self->diff_id_to_layer[pair_index].value));
  }

  shfree(self->diff_id_to_layer);
}

Layer *layer_store_get_layer_by_diff_id(LayerStore *self, const char *diff_id) {
  // Find the layer by the diff ID.
  ptrdiff_t id_pair_index = shgeti(self->diff_id_to_layer, diff_id);
  if (-1 == id_pair_index) {
    return NULL;
  }

  return &(self->diff_id_to_layer[id_pair_index].value);
}
