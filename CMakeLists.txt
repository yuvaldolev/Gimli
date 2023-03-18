cmake_minimum_required(VERSION 3.18)

project(gimli_root)

option(BUILD_SHARED_LIBS "Build using shared libraries" OFF)
option(JANSSON_BUILD_DOCS "Build the Jansson documentation" OFF)
option(JANSSON_INSTALL "Generate Jansson installation target" OFF)

add_subdirectory(gimli)
add_subdirectory(third_party)
