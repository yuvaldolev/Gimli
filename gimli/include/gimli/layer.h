#pragma once

typedef struct Layer {
  char *chain_id;
  char *diff_id;
  char *cache_id;
} Layer;

int layer_init(Layer *self, const char *chain_id,
               char const *layer_store_directory);

void layer_destroy(Layer *self);
