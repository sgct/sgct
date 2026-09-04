##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2026                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

cmake_minimum_required(VERSION 4.0)

# The options gating optional dependencies have to be declared before the first project()
# call: that is when vcpkg's toolchain file runs and reads VCPKG_MANIFEST_FEATURES.
option(SGCT_FREETYPE_SUPPORT "Build SGCT with Freetype2" ON)
option(SGCT_OPENXR_SUPPORT "SGCT OpenXR support" OFF)
option(SGCT_VRPN_SUPPORT "SGCT VRPN support" OFF)
option(SGCT_TRACY_SUPPORT "Build SGCT with Tracy" OFF)

# WIN32 is only set by project(), so the host has to stand in for it here
if (CMAKE_HOST_WIN32)
  option(SGCT_SPOUT_SUPPORT "SGCT Spout support" ON)
  option(SGCT_NDI_SUPPORT "SGCT NDI Support" ON)
  option(SGCT_SCALABLE_SUPPORT "SGCT Scalable SDK support" ON)
endif ()

# Tracy's own switch is honoured so that a surrounding project can turn on profiling globally
if (TRACY_ENABLE)
  set(SGCT_TRACY_SUPPORT ON CACHE BOOL "" FORCE)
endif ()

if (SGCT_FREETYPE_SUPPORT)
  list(APPEND VCPKG_MANIFEST_FEATURES "freetype")
endif ()
if (SGCT_OPENXR_SUPPORT)
  list(APPEND VCPKG_MANIFEST_FEATURES "openxr")
endif ()
if (SGCT_SPOUT_SUPPORT)
  list(APPEND VCPKG_MANIFEST_FEATURES "spout2")
endif ()
if (SGCT_TRACY_SUPPORT)
  list(APPEND VCPKG_MANIFEST_FEATURES "tracy")
endif ()
# SGCT_VRPN_SUPPORT has no counterpart here as vcpkg carries no VRPN port; VRPN stays
# vendored in ext/vrpn

project(sgct VERSION 4.0.0 LANGUAGES C CXX)
enable_testing()

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_FIND_PACKAGE_PREFER_CONFIG ON)

option(SGCT_EXAMPLES "Build SGCT examples" OFF)

option(SGCT_MEMORY_PROFILING "Override new and delete for memory profiling in Tracy" OFF)

if (WIN32)
  option(SGCT_ENABLE_EDIT_CONTINUE "Enable Edit&Continue" ON)

  if (SGCT_TRACY_SUPPORT AND SGCT_ENABLE_EDIT_CONTINUE)
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
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
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
