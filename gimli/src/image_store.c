#include "gimli/image_store.h"

#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>

#include "gimli/gimli_directory.h"
#include "gimli/io.h"
#include "jansson.h"
#include "stb_ds/stb_ds.h"

static int parse_repository(ImageStore *self, json_t *repository) {
  const char *repository_name;
  json_t *image_id_json;
  json_object_foreach(repository, repository_name, image_id_json) {
    if (!json_is_string(image_id_json)) {
      return 1;
    }

    char *store_repository_name = strdup(repository_name);
    if (NULL == store_repository_name) {
      return 1;
    }

    char *store_image_id = strdup(json_string_value(image_id_json));
    if (NULL == store_image_id) {
      free(store_repository_name);
      return 1;
    }

    shput(self->repository_to_id, store_repository_name, store_image_id);
  }

  return 0;
}

static int parse_repositories(ImageStore *self, const char *repositories_file) {
  int ret = 1;

  // Parse the repositories root JSON.
  json_t *repositories_root = json_loads(repositories_file, 0, NULL);
  if (NULL == repositories_root) {
    goto out;
  }

  if (!json_is_object(repositories_root)) {
    goto out_decref_repositories_root;
  }

  // Parse the repositories.
  json_t *repositories = json_object_get(repositories_root, "Repositories");
  if (!json_is_object(repositories)) {
    goto out_decref_repositories_root;
  }

  // Reset the repository to ID map (required to be initialized to NULL by
  // stb_ds).
  self->repository_to_id = NULL;

  // Parse each repository.
  const char *repository_kind;
  json_t *repository;
  json_object_foreach(repositories, repository_kind, repository) {
    if (!json_is_object(repository)) {
      goto out_free_repository_to_id;
    }

    if (0 != parse_repository(self, repository)) {
      goto out_free_repository_to_id;
    }
  }

  ret = 0;
  goto out_decref_repositories_root;

out_free_repository_to_id:
  for (ptrdiff_t pair_index = 0; pair_index < shlen(self->repository_to_id);
       ++pair_index) {
    free(self->repository_to_id[pair_index].key);
    free(self->repository_to_id[pair_index].value);
  }

  shfree(self->repository_to_id);

out_decref_repositories_root:
  json_decref(repositories_root);

out:
  return ret;
}

static int read_image_store(ImageStore *self, const char *path,
                            DIR *directory) {
  int ret = 1;

  // Reset the ID to image map (required to be initialized to NULL by stb_ds).
  self->id_to_image = NULL;

  for (;;) {
    // Set `errno` to 0 before reading the next directory entry.
    errno = 0;

    // Read the next directory entry.
    struct dirent *entry = readdir(directory);
    if (NULL == entry) {
      // Check if an error occurred while reading the directory entry.
      if (0 != errno) {
        goto out_free_id_to_image;
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
      goto out_free_id_to_image;
    }

    // Add the image to the ID to image map.
    shput(self->id_to_image, image.id, image);
  }

  ret = 0;
  goto out;

out_free_id_to_image:
  for (ptrdiff_t pair_index = 0; pair_index < shlen(self->id_to_image);
       ++pair_index) {
    image_destroy(&(self->id_to_image[pair_index].value));
  }

  shfree(self->id_to_image);

out:
  return ret;
}

static int init_repository_to_id(ImageStore *self) {
  int ret = 1;

  // Format the repositories file path.
  char repositories_file_path[PATH_MAX];
  snprintf(repositories_file_path, sizeof(repositories_file_path),
           "%s/image/overlay2/repositories.json", gimli_directory_get());

  // Read the repositories file.
  char *repositories_file;
  if (0 != io_file_to_string(repositories_file_path, &repositories_file)) {
    goto out;
  }

  // Parse the repositories file.
  if (0 != parse_repositories(self, repositories_file)) {
    goto out_free_repositories_file;
  }

  ret = 0;

out_free_repositories_file:
  free(repositories_file);

out:
  return ret;
}

static int init_id_to_image(ImageStore *self) {
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

int image_store_init(ImageStore *self) {
  // Initialize the ID to image map.
  if (0 != init_id_to_image(self)) {
    return 1;
  }

  // Initialize the repository to ID map.
  if (0 != init_repository_to_id(self)) {
    // Free the ID to image map.
    for (ptrdiff_t pair_index = 0; pair_index < shlen(self->id_to_image);
         ++pair_index) {
      image_destroy(&(self->id_to_image[pair_index].value));
    }

    shfree(self->id_to_image);

    return 1;
  }

  return 0;
}

void image_store_destroy(ImageStore *self) {
  // Free the repository to ID map.
  for (ptrdiff_t pair_index = 0; pair_index < shlen(self->repository_to_id);
       ++pair_index) {
    free(self->repository_to_id[pair_index].key);
    free(self->repository_to_id[pair_index].value);
  }

  shfree(self->repository_to_id);

  // Free the ID to image map.
  for (ptrdiff_t pair_index = 0; pair_index < shlen(self->id_to_image);
       ++pair_index) {
    image_destroy(&(self->id_to_image[pair_index].value));
  }

  shfree(self->id_to_image);
}
