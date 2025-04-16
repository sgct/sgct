##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2025                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

cmake_minimum_required(VERSION 3.25 FATAL_ERROR)
cmake_policy(VERSION 3.25)
enable_testing()

project(sgct VERSION 3.0.0)

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)


include(support/cmake/register_package.cmake)
set(CMAKE_FIND_PACKAGE_PREFER_CONFIG ON)
file(REMOVE_RECURSE ${CMAKE_BINARY_DIR}/pkg)
list(APPEND CMAKE_MODULE_PATH
  ${PROJECT_SOURCE_DIR}/support/cmake/modules
  ${CMAKE_BINARY_DIR}/pkg
)

if (APPLE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15")
  if (NOT CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES "x86_64")
  endif ()
  mark_as_advanced(CMAKE_OSX_ARCHITECTURES)
endif ()

option(SGCT_EXAMPLES "Build SGCT examples" OFF)

option(SGCT_FREETYPE_SUPPORT "Build SGCT with Freetype2" ON)
option(SGCT_OPENVR_SUPPORT "SGCT OpenVR support" OFF)
option(SGCT_VRPN_SUPPORT "SGCT VRPN support" OFF)
option(SGCT_TRACY_SUPPORT "Build SGCT with Tracy" OFF)
option(SGCT_MEMORY_PROFILING "Override new and delete for memory profiling in Tracy" OFF)

if (WIN32)
  option(SGCT_SPOUT_SUPPORT "SGCT Spout support" ON)
  option(SGCT_NDI_SUPPORT "SGCT NDI Support" ON)
endif ()

if (WIN32)
  option(SGCT_GSTREAMER_SUPPORT "SGCT GStreamer support" OFF)
endif ()

# Exceptions for external libraries
option(SGCT_DEP_INCLUDE_CATCH2 "Include Catch2 library" ON)
option(SGCT_DEP_INCLUDE_FMT "Include FMT library" ON)
option(SGCT_DEP_INCLUDE_FREETYPE "Include FreeType library" ON)
option(SGCT_DEP_INCLUDE_GLAD "Include GLAD library" ON)
option(SGCT_DEP_INCLUDE_GLFW "Include GLFW library" ON)
option(SGCT_DEP_INCLUDE_GLM "Include GLM library" ON)
option(SGCT_DEP_INCLUDE_JSON "Include JSON library" ON)
option(SGCT_DEP_INCLUDE_JSON_VALIDATOR "Include JSON-Validator library" ON)
option(SGCT_DEP_INCLUDE_LIBPNG "Include LibPNG library" ON)
option(SGCT_DEP_INCLUDE_OPENVR "Include OpenVR library" ON)
option(SGCT_DEP_INCLUDE_SCN "Include SCN library" ON)
option(SGCT_DEP_INCLUDE_STB "Include STB library" ON)
option(SGCT_DEP_INCLUDE_TINYXML "Include TinyXML library" ON)
option(SGCT_DEP_INCLUDE_TRACY "Include Tracy library" OFF)
option(SGCT_DEP_INCLUDE_VRPN "Include VRPN library" OFF)
option(SGCT_DEP_INCLUDE_ZLIB "Include ZLIB library" ON)

if (WIN32)
  option(SGCT_DEP_INCLUDE_SCALABLE "Include Scalable SDK" ON)
endif ()

if (WIN32)
  option(SGCT_ENABLE_EDIT_CONTINUE "Enable Edit&Continue" ON)

  if (TRACY_ENABLE AND SGCT_ENABLE_EDIT_CONTINUE)
    message(WARNING "Tracy does not support compiling with Edit&Continue, so we have to disable that feature")
    set(SGCT_ENABLE_EDIT_CONTINUE OFF CACHE BOOL "" FORCE)
  endif ()
endif ()

if (UNIX)
  option(SGCT_ENABLE_STATIC_ANALYZER "Enable Static analyzer" OFF)
endif ()

option(SGCT_INSTALL "Install SGCT library" OFF)
option(SGCT_BUILD_TESTS "Build SGCT tests" ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# add dummy uninstall target to avoid it beeing added by dependencies
if (NOT TARGET uninstall)
  add_custom_target(uninstall)
endif()

add_subdirectory(ext SYSTEM)
add_subdirectory(src)
if (SGCT_BUILD_TESTS)
  add_subdirectory(tests)
endif()
if (SGCT_EXAMPLES)
  add_subdirectory(apps)
endif ()
