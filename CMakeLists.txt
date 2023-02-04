##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2023                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

cmake_minimum_required(VERSION 3.25 FATAL_ERROR)
cmake_policy(SET CMP0048 NEW)
cmake_policy(SET CMP0063 NEW)
cmake_policy(SET CMP0072 NEW)

project(sgct VERSION 3.0.0)

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)


include(support/cmake/register_package.cmake)
file(REMOVE_RECURSE ${PROJECT_BINARY_DIR}/pkg)
list(APPEND CMAKE_MODULE_PATH
  ${PROJECT_SOURCE_DIR}/support/cmake/modules
  ${PROJECT_BINARY_DIR}/pkg
)

if (APPLE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15")
  set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE STRING "OSX Architectures" FORCE)
  mark_as_advanced(CMAKE_OSX_ARCHITECTURES)
endif ()

option(SGCT_EXAMPLES "Build SGCT examples" OFF)

option(SGCT_FREETYPE_SUPPORT "Build SGCT with Freetype2" ON)
option(SGCT_OPENVR_SUPPORT "SGCT OpenVR support" OFF)
option(SGCT_VRPN_SUPPORT "SGCT VRPN support" OFF)
if (WIN32)
  option(SGCT_SPOUT_SUPPORT "SGCT Spout support" OFF)
endif ()

# Exceptions for external libraries
option(SGCT_DEP_INCLUDE_CATCH2 "Include Catch2 library" ON)
option(SGCT_DEP_INCLUDE_FMT "Include FMT library" ON)
option(SGCT_DEP_INCLUDE_SCN "Include SCN library" ON)
option(SGCT_DEP_INCLUDE_GLFW "Include GLFW library" ON)
option(SGCT_DEP_INCLUDE_LIBPNG "Include LibPNG library" ON)
option(SGCT_DEP_INCLUDE_FREETYPE "Include FreeType library" ON)
option(SGCT_DEP_INCLUDE_TINYXML "Include TinyXML library" ON)
option(SGCT_DEP_INCLUDE_VRPN "Include VRPN library" OFF)
option(SGCT_DEP_INCLUDE_GLAD "Include GLAD library" ON)
option(SGCT_DEP_INCLUDE_ZLIB "Include ZLIB library" ON)
option(SGCT_DEP_INCLUDE_GLM "Include GLM library" ON)
option(SGCT_DEP_INCLUDE_OPENVR "Include OpenVR library" ON)

option(SGCT_DEP_ENABLE_TRACY "Enable Tracy Profiler" OFF)
if (WIN32)
  option(SGCT_ENABLE_EDIT_CONTINUE "Enable Edit&Continue" ON)

  if (SGCT_DEP_ENABLE_TRACY)
    message(WARNING "Tracy does not support compiling with Edit&Continue, so we have to disable that feature")
    set(SGCT_ENABLE_EDIT_CONTINUE OFF CACHE BOOL "" FORCE)
  endif ()
endif ()

if (UNIX)
  option(SGCT_ENABLE_STATIC_ANALYZER "Enable Static analyzer" OFF)
endif ()

add_subdirectory(ext SYSTEM)
add_subdirectory(src)

add_subdirectory(tests)

if (SGCT_EXAMPLES)
  add_subdirectory(apps)
endif ()
