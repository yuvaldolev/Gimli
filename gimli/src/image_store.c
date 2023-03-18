#include "gimli/image_store.h"

#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>

#include "gimli/gimli_directory.h"

static int read_image_store(ImageStore *self, const char *path,
                            DIR *directory) {
  int ret = 1;

  // TODO: Remove.
  (void)self;
  (void)path;

  for (;;) {
    // Set `errno` to 0 before reading the next directory entry.
    errno = 0;

    // Read the next directory entry.
    struct dirent *entry = readdir(directory);
    if (NULL == entry) {
      // Check if an error occurred while reading the directory entry.
      if (0 != errno) {
        // TODO: Handle error.
      }

      // Completed reading all directory entries.
      break;
    }

    // Skip non regular file entries.
    if (DT_REG != entry->d_type) {
      continue;
    }

    // Read the image information.
    Image image;
    if (0 != image_init(&image, entry->d_name, path)) {
      // TODO: handle error.
    }

    printf("Image{ id=%s }\n", image.id);
  }

  ret = 0;

  return ret;
}

int image_store_init(ImageStore *self) {
  int ret = 1;

  // Format the image store directory path.
  char image_store_directory_path[PATH_MAX];
  snprintf(image_store_directory_path, sizeof(image_store_directory_path),
           "%s/image/overlay2/imagedb/content/sha256", gimli_directory_get());

  // Open the image store directory.
  DIR *image_store_directory = opendir(image_store_directory_path);
  if (NULL == image_store_directory) {
    goto out;
  }

  // Read the image store.
  if (0 != read_image_store(self, image_store_directory_path,
                            image_store_directory)) {
    goto out_close_image_store_directory;
  }

  ret = 0;

out_close_image_store_directory:
  closedir(image_store_directory);

out:
  return ret;
}
