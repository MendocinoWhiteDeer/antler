cmake_minimum_required(VERSION 3.20)
project(antler C)
set(CMAKE_C_STANDARD 99)

message("Configuring antler ...")

# default to release build if unspecified
if(NOT CMAKE_BUILD_TYPE)
 set(CMAKE_BUILD_TYPE Release)
endif()
message("Build type: ${CMAKE_BUILD_TYPE}")

# compile flags
set(CMAKE_C_FLAGS_DEBUG "-g -Wall")

set(CMAKE_C_FLAGS_RELEASE "-O3")

add_library(antler SHARED
 "src/base.c")
target_include_directories(antler PRIVATE "${PROJECT_SOURCE_DIR}/src")
