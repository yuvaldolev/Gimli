cmake_minimum_required(VERSION 3.18)

project(third_party)

option(JANSSON_BUILD_DOCS "Build the Jansson documentation" OFF)
add_subdirectory(jansson)
add_subdirectory(stb_ds)
