#pragma once

#include <stddef.h>

typedef struct Image {
  char *id;
  char **layers;
  size_t layers_size;
} Image;

int image_init(Image *self, const char *id, const char *image_store_directory);

void image_destroy(Image *self);
