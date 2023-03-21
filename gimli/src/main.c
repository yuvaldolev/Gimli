#define _GNU_SOURCE

#include <errno.h>
#include <limits.h>
#include <sched.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "gimli/cli.h"
#include "gimli/gimli_directory.h"
#include "gimli/image.h"
#include "gimli/image_store.h"
#include "gimli/io.h"
#include "gimli/layer_store.h"
#include "gimli/uuid.h"
#include "stb_ds/stb_ds.h"

typedef struct ContainerConfiguration {
  Image *image;
  char *directory;
  char *hostname;
  char **command;
  LayerStore *layer_store;
} ContainerConfiguration;

static const size_t CLONE_STACK_SIZE = 1024 * 1024;

static const char *LOWERDIR_MOUNT_DATA_PREFIX = "lowerdir=";
static const char *UPPERDIR_MOUNT_DATA_PREFIX = "upperdir=";
static const char *WORKDIR_MOUNT_DATA_PREFIX = "workdir=";

static int mount_container_image(const Image *image, const char *directory,
                                 LayerStore *layer_store) {
  int ret = 1;

  (void)image;
  (void)directory;

  // Remount everything as private.
  if (0 != mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL)) {
    goto out;
  }

  // Setup the image overlayfs.
  // Create the upperdir.
  char upperdir[PATH_MAX];
  snprintf(upperdir, sizeof(upperdir), "%s/diff", directory);
  if (0 != mkdir(upperdir, 0755)) {
    goto out;
  }

  // Create the workdir.
  char workdir[PATH_MAX];
  snprintf(workdir, sizeof(workdir), "%s/work", directory);
  if (0 != mkdir(workdir, 0755)) {
    goto out;
  }

  // Create the merged directory.
  char merged_directory[PATH_MAX];
  snprintf(merged_directory, sizeof(merged_directory), "%s/merged", directory);
  if (0 != mkdir(merged_directory, 0755)) {
    goto out;
  }

  // Prepare the mount data.
  // The lowerdir data is formatted as follows:
  // lowerdir=<directory-path>:<directory-path>:...
  // `layers_size - 1` is the number of ':' characters that need to be
  // appended.
  size_t lowerdir_mount_data_prefix_size = strlen(LOWERDIR_MOUNT_DATA_PREFIX);
  size_t lowerdir_mount_data_size =
      lowerdir_mount_data_prefix_size + image->layers_size - 1;
  for (size_t layer_index = 0; layer_index < image->layers_size;
       ++layer_index) {
    Layer *layer = layer_store_get_layer_by_diff_id(layer_store,
                                                    image->layers[layer_index]);
    if (NULL == layer) {
      goto out;
    }

    lowerdir_mount_data_size += strlen(layer->link_path);
  }

  // The upperdir data if formatted as follows:
  // upperdir=<directory-path>
  size_t upperdir_mount_data_prefix_size = strlen(UPPERDIR_MOUNT_DATA_PREFIX);
  size_t upperdir_size = strlen(upperdir);
  size_t upperdir_mount_data_size =
      upperdir_mount_data_prefix_size + upperdir_size;

  // The workdir data if formatted as follows:
  // workdir=<directory-path>
  size_t workdir_mount_data_prefix_size = strlen(WORKDIR_MOUNT_DATA_PREFIX);
  size_t workdir_size = strlen(workdir);
  size_t workdir_mount_data_size =
      workdir_mount_data_prefix_size + workdir_size;

  // Calculate the overall size of the mount data.
  // Add 2 extra bytes for the commas between the mount data parts and an extra
  // byte for the null terminator at the end of the mount data.
  size_t mount_data_size = lowerdir_mount_data_size + upperdir_mount_data_size +
                           workdir_mount_data_size + 3;

  // Allocate the mount data string.
  char *mount_data = malloc(mount_data_size);
  if (NULL == mount_data) {
    return 1;
  }

  // Format the lowerdir mount data.
  char *cursor = mount_data;

  memcpy(cursor, LOWERDIR_MOUNT_DATA_PREFIX, lowerdir_mount_data_prefix_size);
  cursor += lowerdir_mount_data_prefix_size;

  // Format the layers in reverse order at the left-most lowerdir in the overlay
  // data is the top-most layer, and the right-most is the bottom-most.
  for (size_t layer_index = image->layers_size; layer_index > 0;
       --layer_index) {
    Layer *layer = layer_store_get_layer_by_diff_id(
        layer_store, image->layers[layer_index - 1]);

    const char *link_path = layer->link_path;
    size_t link_path_size = strlen(link_path);

    memcpy(cursor, layer->link_path, link_path_size);
    cursor += link_path_size;

    if (0 != (layer_index - 1)) {
      *cursor = ':';
      ++cursor;
    }
  }

  *cursor = ',';
  ++cursor;

  // Format the upperdir mount data.
  memcpy(cursor, UPPERDIR_MOUNT_DATA_PREFIX, upperdir_mount_data_prefix_size);
  cursor += upperdir_mount_data_prefix_size;

  memcpy(cursor, upperdir, upperdir_size);
  cursor += upperdir_size;

  *cursor = ',';
  ++cursor;

  // Format the workdir mount data.
  memcpy(cursor, WORKDIR_MOUNT_DATA_PREFIX, workdir_mount_data_prefix_size);
  cursor += workdir_mount_data_prefix_size;

  memcpy(cursor, workdir, workdir_size);
  cursor += workdir_size;

  // Add a null terminator to the end of the mount data.
  *cursor = '\0';

  // Perform the overlayfs mount.
  printf("Mount data: %s\n", mount_data);
  if (0 != mount("overlay", merged_directory, "overlay", 0, mount_data)) {
    goto out_free_mount_data;
  }

  ret = 0;

out_free_mount_data:
  free(mount_data);

out:
  return ret;
}

static int child(void *argument) {
  ContainerConfiguration *container_configuration = argument;

  // Set the container hostname.
  printf("=> setting container hostname... ");

  if (-1 == sethostname(container_configuration->hostname,
                        strlen(container_configuration->hostname))) {
    printf("failed, error(%d): [%s]\n", errno, strerror(errno));
    return 1;
  }

  printf("done\n");

  // Mount the image.
  printf("=> mounting container image... ");

  if (0 != mount_container_image(container_configuration->image,
                                 container_configuration->directory,
                                 container_configuration->layer_store)) {
    printf("failed, error(%d): [%s]\n", errno, strerror(errno));
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

  // Create the container directory.
  printf("=> creating container directory... ");

  // The directory is allocated on the heap as it is passed to the child
  // process.
  char container_directory[PATH_MAX];
  snprintf(container_directory, sizeof(container_directory), "%s/container/%s",
           gimli_directory_get(), container_hostname);

  if (0 != mkdir(container_directory, 0755)) {
    printf("failed, error(%d): [%s]", errno, strerror(errno));
    goto out_free_container_hostname;
  }

  printf("done\n");

  // Setup the container configuration.
  printf("=> setting up the container configuration... ");

  ContainerConfiguration container_configuration = {
      .image = container_image,
      .directory = container_directory,
      .hostname = container_hostname,
      .command = cli.command,
      .layer_store = &layer_store,
  };

  printf("done\n");

  // Clone a child process in new namespaces.
  uint8_t *clone_stack = malloc(CLONE_STACK_SIZE);
  if (NULL == clone_stack) {
    printf("failed, error(%d): [%s]\n", errno, strerror(errno));
    goto out_remove_container_directory;
  }

  int clone_flags =
      CLONE_NEWNS | CLONE_NEWPID | CLONE_NEWIPC | CLONE_NEWNET | CLONE_NEWUTS;

  int child_pid = clone(child, clone_stack + CLONE_STACK_SIZE,
                        clone_flags | SIGCHLD, &container_configuration);
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

out_remove_container_directory:
  io_remove_directory_recursive(container_directory);

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
