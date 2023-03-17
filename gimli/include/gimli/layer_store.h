#pragma once

#include "layer.h"

typedef struct LayerItem {
  char *key;
  Layer value;
} LayerItem;

typedef struct LayerStore {
  LayerItem *layers;
} LayerStore;

int layer_store_init(LayerStore *self);

void layer_store_destroy(LayerStore *self);
