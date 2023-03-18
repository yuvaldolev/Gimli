#pragma once

#include "gimi/image.h"

typedef struct RepositoryToImagePair {
  char *key;
  Image *value;
} RepositoryToImagePair;

typedef struct ImageStore {
  Image *images;
  RepositoryToImagePair *repository_to_image;
} ImageStore;
