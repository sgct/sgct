cmake_minimum_required(VERSION 2.6)
project(GLEW)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
add_library(GLEW STATIC src/glew.c include/GL/glew.h)