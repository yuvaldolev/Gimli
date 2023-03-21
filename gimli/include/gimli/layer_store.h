#pragma once

#include "gimli/layer.h"

typedef struct DiffIdToLayerPair {
  char *key;
  Layer value;
} DiffIdToLayerPair;

typedef struct LayerStore {
  DiffIdToLayerPair *diff_id_to_layer;
} LayerStore;

int layer_store_init(LayerStore *self);

void layer_store_destroy(LayerStore *self);

Layer *layer_store_get_layer_by_diff_id(LayerStore *self, const char *diff_id);
