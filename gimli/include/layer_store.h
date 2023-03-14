#pragma once

#include "layer.h"

typedef struct LayerStore {
  Layer *layers;
} LayerStore;

int layer_store_init(LayerStore *self);

void layer_store_destroy(LayerStore *self);
