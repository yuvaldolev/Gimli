#define _GNU_SOURCE

#include <errno.h>
#include <libgen.h>
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
#include <sys/syscall.h>
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
  char *root_fs_directory;
  char *hostname;
  char **command;
  LayerStore *layer_store;
} ContainerConfiguration;

static const size_t CLONE_STACK_SIZE = 1024 * 1024;

static const char *LOWERDIR_MOUNT_DATA_PREFIX = "lowerdir=";
static const char *UPPERDIR_MOUNT_DATA_PREFIX = "upperdir=";
static const char *WORKDIR_MOUNT_DATA_PREFIX = "workdir=";

static int setup_image_overlayfs(const Image *image, const char *directory,
                                 const char *merged_directory,
                                 LayerStore *layer_store) {
  int ret = 1;

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
    goto out;
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
  if (0 != mount("overlay", merged_directory, "overlay", 0, mount_data)) {
    goto out_free_mount_data;
  }

  ret = 0;

out_free_mount_data:
  free(mount_data);

out:
  return ret;
}

#if 0
static int filter_syscalls() {
  scmp_filter_ctx ctx = NULL;
  fprintf(stderr, "=> filtering syscalls...");
  if (!(ctx = seccomp_init(SCMP_ACT_ALLOW)) ||
      seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(chmod), 1,
                       SCMP_A1(SCMP_CMP_MASKED_EQ, S_ISUID, S_ISUID)) ||
      seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(chmod), 1,
                       SCMP_A1(SCMP_CMP_MASKED_EQ, S_ISGID, S_ISGID)) ||
      seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(fchmod), 1,
                       SCMP_A1(SCMP_CMP_MASKED_EQ, S_ISUID, S_ISUID)) ||
      seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(fchmod), 1,
                       SCMP_A1(SCMP_CMP_MASKED_EQ, S_ISGID, S_ISGID)) ||
      seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(fchmodat), 1,
                       SCMP_A2(SCMP_CMP_MASKED_EQ, S_ISUID, S_ISUID)) ||
      seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(fchmodat), 1,
                       SCMP_A2(SCMP_CMP_MASKED_EQ, S_ISGID, S_ISGID)) ||
      seccomp_rule_add(
          ctx, SCMP_FAIL, SCMP_SYS(unshare), 1,
          SCMP_A0(SCMP_CMP_MASKED_EQ, CLONE_NEWUSER, CLONE_NEWUSER)) ||
      seccomp_rule_add(
          ctx, SCMP_FAIL, SCMP_SYS(clone), 1,
          SCMP_A0(SCMP_CMP_MASKED_EQ, CLONE_NEWUSER, CLONE_NEWUSER)) ||
      seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(ioctl), 1,
                       SCMP_A1(SCMP_CMP_MASKED_EQ, TIOCSTI, TIOCSTI)) ||
      seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(keyctl), 0) ||
      seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(add_key), 0) ||
      seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(request_key), 0) ||
      seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(ptrace), 0) ||
      seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(mbind), 0) ||
      seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(migrate_pages), 0) ||
      seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(move_pages), 0) ||
      seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(set_mempolicy), 0) ||
      seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(userfaultfd), 0) ||
      seccomp_rule_add(ctx, SCMP_FAIL, SCMP_SYS(perf_event_open), 0) ||
      seccomp_attr_set(ctx, SCMP_FLTATR_CTL_NNP, 0) || seccomp_load(ctx)) {
    if (ctx) seccomp_release(ctx);
    fprintf(stderr, "failed: %m\n");
    return 1;
  }
  seccomp_release(ctx);
  fprintf(stderr, "done.\n");
  return 0;
}
#endif

static int mount_container_image(const Image *image, const char *directory,
                                 const char *root_fs_directory,
                                 LayerStore *layer_store) {
  // Remount everything as private.
  if (0 != mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL)) {
    return 1;
  }

  // Create the merged directory.
  if (0 != mkdir(root_fs_directory, 0755)) {
    return 1;
  }

  // Setup the image overlayfs.
  if (0 !=
      setup_image_overlayfs(image, directory, root_fs_directory, layer_store)) {
    return 1;
  }

  // Create a temporary directory to move the old root directory to.
  char old_root_fs_directory[PATH_MAX];
  snprintf(old_root_fs_directory, sizeof(old_root_fs_directory), "%s/old_root",
           root_fs_directory);

  if (0 != mkdir(old_root_fs_directory, 0755)) {
    return 1;
  }

  // Pivot to the merged directory.
  if (0 != syscall(SYS_pivot_root, root_fs_directory, old_root_fs_directory)) {
    return 1;
  }

  // Change directory to the newly pivoted root directory,
  if (0 != chdir("/")) {
    return 1;
  }

  // Unmount the old root directory.
  // The basename of the old root directory is retrieved here, as the root
  // directory has been pivoted.
  char *old_root_fs_directory_name = basename(old_root_fs_directory);
  if (0 != umount2(old_root_fs_directory_name, MNT_DETACH)) {
    return 1;
  }

  // Remove the old root directory.
  if (0 != rmdir(old_root_fs_directory_name)) {
    return 1;
  }

  // Mount `/proc`.
  if (0 != mount("proc", "/proc", "proc", 0, NULL)) {
    return 1;
  }

  return 0;
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
                                 container_configuration->root_fs_directory,
                                 container_configuration->layer_store)) {
    printf("failed, error(%d): [%s]\n", errno, strerror(errno));
    return 1;
  }

  printf("done\n");

  // Drop capabilities.
  printf("=> filtering syscalls... ");

#if 0
  if (0 != filter_syscalls()) {
    printf("failed, error(%d): [%s]\n", errno, strerror(errno));
    return 1;
  }
#endif

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

  char root_fs_directory[PATH_MAX];
  snprintf(root_fs_directory, sizeof(root_fs_directory), "%s/merged",
           container_directory);

  printf("done\n");

  // Setup the container configuration.
  printf("=> setting up the container configuration... ");

  ContainerConfiguration container_configuration = {
      .image = container_image,
      .directory = container_directory,
      .root_fs_directory = root_fs_directory,
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
  // Try unmounting the root directory of the container's file system.
  umount(root_fs_directory);

  // Remove the container directory.
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
