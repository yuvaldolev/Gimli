#include "gimli_directory_accessor.h"

#include <limits.h>
#include <stddef.h>
#include <stdio.h>

static const char *const GIMLI_DIRECTORY = "/var/lib/gimli";

// TODO: We probably don't need this functionality in a centralized file.
// This file should be renamed to `gimli_directory.c` and should provide all
// functionality related to the Gimli directory (formatting sub-directory paths
// and so on...).
int gimli_directory_accessor_init(GimliDirectoryAccessor *self,
                                  const char *subdirectory) {
  // Format the full directory path.
  char path[PATH_MAX] = {0};
  snprintf(path, sizeof(path), "%s/%s", GIMLI_DIRECTORY, subdirectory);

  // Open the Gimli directory.
  self->directory = opendir(path);
  if (NULL == self->directory) {
    return 1;
  }

  return 0;
}

void gimli_directory_accessor_destroy(GimliDirectoryAccessor *self) {
  closedir(self->directory);
}
