#define _GNU_SOURCE

#include <errno.h>
#include <sched.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "gimli/cli.h"
#include "gimli/image.h"
#include "gimli/image_store.h"
#include "gimli/layer_store.h"
#include "gimli/uuid.h"
#include "stb_ds/stb_ds.h"

typedef struct ContainerConfiguration {
  Image *image;
  char *hostname;
  char **command;
} ContainerConfiguration;

static const size_t CLONE_STACK_SIZE = 1024 * 1024;

static int child(void *argument) {
  ContainerConfiguration *container_configuration = argument;

  // Set the container hostname.
  printf("=> setting container hostname... ");

  if (-1 == sethostname(container_configuration->hostname,
                        strlen(container_configuration->hostname))) {
    printf("failed, error(%d): [%s]", errno, strerror(errno));
    return 1;
  }

  printf("done\n");

  // Execute the user command.
  execve(container_configuration->command[0], container_configuration->command,
         NULL);

  // If this line is reached, it means that `execve` failed.
  // Exit with failure.
  printf("failed executing user command, error(%d): [%s]\n", errno,
         strerror(errno));
  return 1;
}

int main(int argc, const char *const argv[]) {
  int ret = 1;

  // Initialize the random seed.
  srand((unsigned int)(time(NULL)));

  // Parse the command line arguments.
  Cli cli;
  if (0 != cli_init(&cli, argc, argv)) {
    cli_print_usage(argv[0]);
    goto out;
  }

  // Initialize the layer store.
  printf("=> initializing layer store... ");

  LayerStore layer_store;
  if (0 != layer_store_init(&layer_store)) {
    printf("failed, error(%d): [%s]\n", errno, strerror(errno));
    goto out_destroy_cli;
  }

  printf("done\n");

  // Initialize the image store.
  printf("=> initializing image store... ");

  ImageStore image_store;
  if (0 != image_store_init(&image_store)) {
    printf("failed, error(%d): [%s]\n", errno, strerror(errno));
    goto out_destroy_layer_store;
  }

  printf("done\n");

  // Locate the container image by its repository.
  printf("=> locating image for repository [%s]... ", cli.image);

  Image *container_image =
      image_store_get_image_by_repository(&image_store, cli.image);
  if (NULL == container_image) {
    printf("failed, no such repository\n");
    goto out_destroy_image_store;
  }

  printf("done\n");

  // Generate the container hostname.
  printf("=> generating container hostname... ");

  char *container_hostname;
  if (0 != uuid_generate(&container_hostname)) {
    printf("failed, error(%d): [%s]\n", errno, strerror(errno));
    goto out_destroy_image_store;
  }

  printf("%s... done\n", container_hostname);

  // Setup the container configuration.
  printf("=> setting up the container configuration... ");

  ContainerConfiguration *container_configuration =
      malloc(sizeof(*container_configuration));
  if (NULL == container_configuration) {
    printf("failed, error(%d): [%s]\n", errno, strerror(errno));
    goto out_free_container_hostname;
  }

  container_configuration->image = container_image;
  container_configuration->hostname = container_hostname;
  container_configuration->command = cli.command;

  printf("done\n");

  // Clone a child process in new namespaces.
  uint8_t *clone_stack = malloc(CLONE_STACK_SIZE);
  if (NULL == clone_stack) {
    printf("failed, error(%d): [%s]\n", errno, strerror(errno));
    goto out_free_container_configuration;
  }

  int clone_flags =
      CLONE_NEWNS | CLONE_NEWPID | CLONE_NEWIPC | CLONE_NEWNET | CLONE_NEWUTS;

  int child_pid = clone(child, clone_stack + CLONE_STACK_SIZE,
                        clone_flags | SIGCHLD, container_configuration);
  if (-1 == child_pid) {
    printf("failed, error(%d): [%s]\n", errno, strerror(errno));
    goto out_free_clone_stack;
  }

  // Wait for the child process to exit.
  int waitpid_status;
  if (-1 == waitpid(child_pid, &waitpid_status, 0)) {
    printf("failed waiting for child process (%d), error(%d): [%s]\n",
           child_pid, errno, strerror(errno));
    goto out_free_clone_stack;
  }

  if (!WIFEXITED(waitpid_status)) {
    printf("child process (%d) has not exited normally\n", child_pid);
    goto out_free_clone_stack;
  }

  int child_exit_code = WEXITSTATUS(waitpid_status);
  printf("=> container process exited with code (%d)\n", child_exit_code);

  ret = child_exit_code;

out_free_clone_stack:
  free(clone_stack);

out_free_container_configuration:
  free(container_configuration);

out_free_container_hostname:
  free(container_hostname);

out_destroy_image_store:
  image_store_destroy(&image_store);

out_destroy_layer_store:
  layer_store_destroy(&layer_store);

out_destroy_cli:
  cli_destroy(&cli);

out:
  return ret;
}
