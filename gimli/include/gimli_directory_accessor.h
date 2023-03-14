#pragma once

#include <dirent.h>

typedef struct GimliDirectoryAccessor {
  DIR *directory;
} GimliDirectoryAccessor;

int gimli_directory_accessor_init(GimliDirectoryAccessor *self,
                                  const char *subdirectory);

void gimli_directory_accessor_destroy(GimliDirectoryAccessor *self);
