#pragma once

int io_file_to_string(const char *path, char **out_data);

void io_remove_directory_recursive(const char *path);
