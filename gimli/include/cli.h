#pragma once

#include <stddef.h>

typedef struct Cli {
  char *image;
  char **command;
  size_t command_size;
} Cli;

int cli_parse(Cli *out_cli, int argc, const char *const argv[]);

void cli_destroy(Cli *cli);

void cli_print_usage(const char *program);
