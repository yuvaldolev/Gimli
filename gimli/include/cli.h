#pragma once

#include <stddef.h>

typedef struct Cli {
  char *image;
  char **command;
  size_t command_size;
} Cli;

int cli_init(Cli *self, int argc, const char *const argv[]);

void cli_destroy(Cli *self);

void cli_print_usage(const char *program);
