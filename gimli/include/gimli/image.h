#pragma once

typedef struct Image {
  char *id;
  char **layer_diff_ids;
} Image;

int image_init(Image *self, const char *id, const char *image_store_directory);
