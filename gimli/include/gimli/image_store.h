#pragma once

typedef struct ImageRepositoryItem {
  char *key;
  Image *value;
} ImageItem;

typedef struct ImageStore {
  Image *images;
  ImageRepositoryItem *repository_to_image;
} ImageStore;
