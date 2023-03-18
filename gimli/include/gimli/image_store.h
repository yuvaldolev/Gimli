#pragma once

#include "gimli/image.h"

typedef struct ImageIdToImagePair {
  char *key;
  Image value;
} ImageIdToImagePair;

typedef struct RepositoryToImageIdPair {
  char *key;
  char *value;
} RepositoryToImageIdPair;

typedef struct ImageStore {
  ImageIdToImagePair *image_id_to_image;
  RepositoryToImageIdPair *repository_to_image_id;
} ImageStore;

int image_store_init(ImageStore *self);
