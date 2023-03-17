cmake_minimum_required(VERSION 3.18)

project(gimli_root)

option(BUILD_SHARED_LIBS "Build using shared libraries" OFF)

add_subdirectory(gimli)
add_subdirectory(third_party)
