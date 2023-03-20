#include "gimli/uuid.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

static const size_t UUID_SIZE = 12;

int uuid_generate(char **out) {
  // Generate the higher UUID bytes.
  int higher_bytes = rand() & 0xFFFF;

  // Generate the lower UUID bytes.
  int lower_bytes = rand();

  // Allocate a buffer to store the UUID.
  *out = malloc(UUID_SIZE + 1);
  if (NULL == (*out)) {
    return 1;
  }

  // Format the UUID.
  snprintf(*out, UUID_SIZE + 1, "%x%x", higher_bytes, lower_bytes);

  return 0;
}
