#include "gimli/image.h"

#include <limits.h>
#include <stdio.h>

#include "gimli/io.h"
#include "jansson.h"

int image_init(Image *self, const char *id, const char *image_store_directory) {
  // TODO: Remove.
  (void)self;

  // Format the metadata file path.
  char metadata_file_path[PATH_MAX];
  snprintf(metadata_file_path, sizeof(metadata_file_path), "%s/%s",
           image_store_directory, id);

  // Read the metadata file.
  char *metadata_file;
  if (0 != file_to_string(metadata_file_path, &metadata_file)) {
    // TODO: Handle errors.
  }

  // Parse the metadata.

  return 0;
}
