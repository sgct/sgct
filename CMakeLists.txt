##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2026                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

cmake_minimum_required(VERSION 4.0)

project(sgct VERSION 4.0.0 LANGUAGES C CXX)

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# option() leaves a value alone that an enclosing project has already set (CMP0077)
option(SGCT_BUILD_TESTS "Build the SGCT unit tests" ${PROJECT_IS_TOP_LEVEL})
option(SGCT_BUILD_CALIBRATOR "Build the calibrator application" ON)
option(SGCT_TRACY_SUPPORT "Build SGCT with Tracy" OFF)

# Tracy's own switch is checked so that another project can enable profiling globally
if (TRACY_ENABLE)
  set(SGCT_TRACY_SUPPORT ON CACHE BOOL "" FORCE)
endif ()

option(SGCT_MEMORY_PROFILING "Override new and delete for memory profiling in Tracy" OFF)
if (SGCT_MEMORY_PROFILING AND NOT SGCT_TRACY_SUPPORT)
  message(WARNING "SGCT_MEMORY_PROFILING requires Tracy, so it is disabled")
  set(SGCT_MEMORY_PROFILING OFF CACHE BOOL "" FORCE)
endif ()

if (WIN32)
  # The NDI and Scalable SDKs are proprietary and cannot be redistributed, so they are
  # opt-in and off by default
  option(SGCT_NDI_SUPPORT "SGCT NDI Support" OFF)
  option(SGCT_SCALABLE_SUPPORT "SGCT Scalable SDK support" OFF)
  option(SGCT_ENABLE_EDIT_CONTINUE "Enable Edit&Continue" ON)

  if (SGCT_TRACY_SUPPORT AND SGCT_ENABLE_EDIT_CONTINUE)
    message(WARNING "Tracy does not support compiling with Edit&Continue, so we have to disable that feature")
    set(SGCT_ENABLE_EDIT_CONTINUE OFF CACHE BOOL "" FORCE)
  endif ()
endif ()

if (UNIX)
  option(SGCT_ENABLE_STATIC_ANALYZER "Enable Static analyzer" OFF)
endif ()

# Redirecting the output would leak into a surrounding project that vendors SGCT
if (PROJECT_IS_TOP_LEVEL)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
endif ()

add_subdirectory(ext SYSTEM)
add_subdirectory(src)
if (SGCT_BUILD_CALIBRATOR)
  add_subdirectory(calibrator)
endif ()
if (SGCT_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif ()
