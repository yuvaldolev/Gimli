cmake_minimum_required(VERSION 3.18)

project(gimli)

add_executable(
    gimli
    src/cli.c
    src/main.c
)

target_include_directories(
    gimli
    PRIVATE
    include
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
