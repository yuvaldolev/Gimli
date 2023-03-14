#include "cli.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum Argument {
  ARGUMENT_PROGRAM = 0,
  ARGUMENT_IMAGE,
  ARGUMENT_FIRST_COMMAND_PART,

  ARGUMENT_MINIMUM_COUNT,
};

static int parse_string_argument(const char *argument, char **out) {
  size_t argument_size = strlen(argument) + 1;

  *out = malloc(argument_size);
  if (NULL == (*out)) {
    return 1;
  }

  memcpy(*out, argument, argument_size);

  return 0;
}

static int parse_string_array_argument(const char *const *argument, size_t size,
                                       char ***out_array, size_t *out_size) {
  int ret = 1;

  // Allocate the output array.
  *out_array = malloc(size * sizeof(*(*out_array)));
  if (NULL == out_array) {
    goto out;
  }

  // The array size will be incremented each time we successfully
  // parse a single item.
  *out_size = 0;

  for (size_t item_index = 0; item_index < size; ++item_index) {
    if (0 != parse_string_argument(argument[item_index],
                                   &((*out_array)[item_index]))) {
      goto out_free_array;
    }

    ++(*out_size);
  }

  ret = 0;
  goto out;

out_free_array:
  // Free all allocated items.
  for (size_t item_index = 0; item_index < (*out_size); ++item_index) {
    free((*out_array)[item_index]);
    (*out_array)[item_index] = NULL;
  }

  // Free the array.
  free(*out_array);
  *out_array = NULL;

  // Set the array size to 0.
  *out_size = 0;

out:
  return ret;
}

int cli_init(Cli *self, int argc, const char *const argv[]) {
  int ret = 1;

  // Ensure that the correct number of arguments has been passed in.
  if (ARGUMENT_MINIMUM_COUNT > argc) {
    goto out;
  }

  // Parse the image argument.
  if (0 != parse_string_argument(argv[ARGUMENT_IMAGE], &self->image)) {
    goto out;
  }

  // Parse the command argument.
  if (0 !=
      parse_string_array_argument(argv + ARGUMENT_FIRST_COMMAND_PART,
                                  (size_t)(argc - ARGUMENT_FIRST_COMMAND_PART),
                                  &self->command, &self->command_size)) {
    goto out_free_image;
  }

  ret = 0;
  goto out;

out_free_image:
  free(self->image);
  self->image = NULL;

out:
  return ret;
}

void cli_destroy(Cli *self) {
  // Free the command.
  for (size_t item_index = 0; item_index < self->command_size; ++item_index) {
    free(self->command[item_index]);
  }

  free(self->command);

  // Free the image.
  free(self->image);
}

void cli_print_usage(const char *program) {
  fprintf(stderr, "USAGE: %s <image> <command>...\n", program);
}
