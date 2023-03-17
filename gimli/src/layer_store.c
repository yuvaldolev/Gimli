#include "layer_store.h"

#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "gimli_directory.h"
#include "layer.h"

static int read_layer_store(LayerStore *self, const char *path,
                            DIR *directory) {
  for (;;) {
    // Set `errno` to 0 before reading the next directory entry.
    errno = 0;

    // Read the next directory entry.
    struct dirent *entry = readdir(directory);
    if (NULL == entry) {
      // Check if an error occurred while reading the directory entry.
      if (0 != errno) {
        return 1;
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
      return 1;
    }

    printf("Layer{ chain_id=%s, diff_id=%s, cache_id=%s }\n", layer.chain_id,
           layer.diff_id, layer.cache_id);

    layer_destroy(&layer);
  }

  return 0;
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

void layer_store_destroy(LayerStore *self) { (void)self; }
