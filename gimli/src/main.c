#include <stdio.h>

#include "cli.h"

int main(int argc, const char *const argv[]) {
  int ret = 1;

  // Parse the command line arguments.
  Cli cli;
  if (0 != cli_parse(&cli, argc, argv)) {
    cli_print_usage(argv[0]);
    goto out;
  }

  printf("Image: %s\n", cli.image);
  printf("Command: [");
  for (size_t index = 0; index < cli.command_size; ++index) {
    printf("\"%s\"", cli.command[index]);
    if ((cli.command_size - 1) > index) {
      printf(", ");
    }
  }
  printf("]\n");

  ret = 0;

  /* out_destroy_cli: */
  cli_destroy(&cli);

out:
  return ret;
}
