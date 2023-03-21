#include "gimli/io.h"

#define _XOPEN_SOURCE 500

#include <errno.h>
#include <fcntl.h>
#include <ftw.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static int nftw_remove(const char *path,
                       const struct stat *stat_buffer __attribute__((unused)),
                       int type __attribute__((unused)),
                       struct FTW *ftw_buffer __attribute__((unused))) {
  remove(path);

  return 0;
}

static int read_all(int fd, void *buffer, size_t size) {
  uint8_t *cursor = buffer;
  size_t bytes_remaining = size;

  while (0 < bytes_remaining) {
    ssize_t result = read(fd, cursor, bytes_remaining);
    if (-1 == result) {
      if (EINTR == errno) {
        // Interrupted while reading, try reading again.
        continue;
      }

      // An error occurred while reading.
      return 1;
    }

    cursor += result;
    bytes_remaining -= (size_t)result;
  }

  return 0;
}

int io_file_to_string(const char *path, char **out_data) {
  int ret = 1;

  // Open the file.
  int fd = open(path, O_RDONLY);
  if (-1 == fd) {
    goto out;
  }

  // Retrieve the file's size.
  struct stat stat_buffer;
  if (0 != fstat(fd, &stat_buffer)) {
    goto out_close_fd;
  }

  // Allocate a buffer to store the entire file data.
  *out_data = malloc(((size_t)stat_buffer.st_size) + 1);
  if (NULL == (*out_data)) {
    goto out_close_fd;
  }

  // Read the entire file.
  if (0 != read_all(fd, *out_data, (size_t)stat_buffer.st_size)) {
    goto out_free_out_data;
  }
  (*out_data)[stat_buffer.st_size] = '\0';

  ret = 0;
  goto out_close_fd;

out_free_out_data:
  free(*out_data);

out_close_fd:
  close(fd);

out:
  return ret;
}

void io_remove_directory_recursive(const char *path) {
  nftw(path, nftw_remove, 64, FTW_DEPTH | FTW_PHYS);
}
