cmake_minimum_required(VERSION 3.18)

project(stb_ds)

add_library(
    stb_ds
    include/stb_ds/stb_ds.h
    src/stb_ds.c
)

target_include_directories(
    stb_ds
    PUBLIC
    include
)

set_target_properties(
    stb_ds
    PROPERTIES
    C_STANDARD 99
)
