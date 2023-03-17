cmake_minimum_required(VERSION 3.18)

project(gimli)

add_executable(
    gimli
    include/gimli/cli.h
    include/gimli/gimli_directory.h
    include/gimli/io.h
    include/gimli/layer.h
    include/gimli/layer_store.h
    src/cli.c
    src/gimli_directory.c
    src/io.c
    src/layer.c
    src/layer_store.c
    src/main.c
)

target_include_directories(
    gimli
    PRIVATE
    include
)

target_link_libraries(
    gimli
    PRIVATE
    stb_ds
)

set_target_properties(
    gimli
    PROPERTIES
    C_STANDARD 99
)

target_compile_options(
    gimli
    PRIVATE
    -Weverything
    -Werror
    -Wno-declaration-after-statement
)
