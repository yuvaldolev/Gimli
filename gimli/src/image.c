#include "gimli/image.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "gimli/io.h"
#include "jansson.h"

#define IMAGE_ID_PREFIX "sha256:"

static int init_image_layers(Image *self, const char *id,
                             const char *image_store_directory) {
  int ret = 1;

  // Format the metadata file path.
  char metadata_file_path[PATH_MAX];
  snprintf(metadata_file_path, sizeof(metadata_file_path), "%s/%s",
           image_store_directory, id);

  // Read the metadata file.
  char *metadata_file;
  if (0 != io_file_to_string(metadata_file_path, &metadata_file)) {
    goto out;
  }

  // Parse the metadata.
  json_t *metadata = json_loads(metadata_file, 0, NULL);
  if (NULL == metadata) {
    goto out_free_metadata_file;
  }

  if (!json_is_object(metadata)) {
    goto out_decref_metadata;
  }

  // Parse the RootFS object,
  json_t *rootfs = json_object_get(metadata, "rootfs");
  if (!json_is_object(rootfs)) {
    goto out_decref_metadata;
  }

  // Parse the RootFS layers diff IDs array.
  json_t *layers = json_object_get(rootfs, "diff_ids");
  if (!json_is_array(layers)) {
    goto out_decref_metadata;
  }

  // Initialize the image layers.
  self->layers = malloc(json_array_size(layers) * sizeof(*self->layers));
  if (NULL == self->layers) {
    goto out_decref_metadata;
  }

  // The layers array size will be incremented each time we successfully
  // parse a single item.
  self->layers_size = 0;

  for (size_t layer_index = 0; layer_index < json_array_size(layers);
       ++layer_index) {
    // Retrieve the layer JSON from the array.
    json_t *layer_json = json_array_get(layers, layer_index);
    if (!json_is_string(layer_json)) {
      goto out_free_layers;
    }

    // Initialize the layer.
    self->layers[layer_index] = strdup(json_string_value(layer_json));
    if (NULL == self->layers[layer_index]) {
      goto out_free_layers;
    }

    // Increment the size of the layers array after successfully parsing an
    // item.
    ++self->layers_size;
  }

  ret = 0;
  goto out_decref_metadata;

out_free_layers:
  for (size_t layer_index = 0; layer_index < self->layers_size; ++layer_index) {
    free(self->layers[layer_index]);
  }

  free(self->layers);

out_decref_metadata:
  json_decref(metadata);

out_free_metadata_file:
  free(metadata_file);

out:
  return ret;
}

static int init_image_id(Image *self, const char *id) {
  // `sizeof(IMAGE_ID_PREFIX)` includes the null terminator, so no need to add 1
  // to the size.
  size_t id_size = sizeof(IMAGE_ID_PREFIX) + strlen(id);

  self->id = malloc(id_size);
  if (NULL == self->id) {
    return 1;
  }

  snprintf(self->id, id_size, "%s%s", IMAGE_ID_PREFIX, id);

  return 0;
}

int image_init(Image *self, const char *id, const char *image_store_directory) {
  // Initialize the ID.
  if (0 != init_image_id(self, id)) {
    return 1;
  }

  // Initialize the RootFS layers.
  if (0 != init_image_layers(self, id, image_store_directory)) {
    free(self->id);
    return 1;
  }

  return 0;
}

void image_destroy(Image *self) {
  // Free the layers array.
  for (size_t layer_index = 0; layer_index < self->layers_size; ++layer_index) {
    free(self->layers[layer_index]);
  }

  free(self->layers);

  // Free the ID.
  free(self->id);
}
