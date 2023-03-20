#pragma once

#include "gimli/image.h"

typedef struct IdToImagePair {
  char *key;
  Image value;
} IdToImagePair;

typedef struct RepositoryToIdPair {
  char *key;
  char *value;
} RepositoryToIdPair;

typedef struct ImageStore {
  IdToImagePair *id_to_image;
  RepositoryToIdPair *repository_to_id;
} ImageStore;

int image_store_init(ImageStore *self);

void image_store_destroy(ImageStore *self);

Image *image_store_get_image_by_repository(ImageStore *self,
                                           const char *repository);
