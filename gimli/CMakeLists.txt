cmake_minimum_required(VERSION 3.18)

project(gimli)

add_executable(
    gimli
    include/gimli/cli.h
    include/gimli/gimli_directory.h
    include/gimli/image.h
    include/gimli/image_store.h
    include/gimli/io.h
    include/gimli/layer.h
    include/gimli/layer_store.h
    include/gimli/uuid.h
    src/cli.c
    src/gimli_directory.c
    src/image.c
    src/image_store.c
    src/io.c
    src/layer.c
    src/layer_store.c
    src/main.c
    src/uuid.c
)

target_include_directories(
    gimli
    PRIVATE
    include
)

target_link_libraries(
    gimli
    PRIVATE
    jansson
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
    -Wno-cast-qual
    -Wno-declaration-after-statement
    -Wno-padded
)
