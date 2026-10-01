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
  # Spout's SDK is x86-only
  if (DEFINED CMAKE_CXX_COMPILER_ARCHITECTURE_ID)
    string(TOLOWER "${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}" sgct_target_arch)
  else ()
    string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" sgct_target_arch)
  endif ()
  if (sgct_target_arch STREQUAL "arm64" OR sgct_target_arch STREQUAL "aarch64")
    option(SGCT_SPOUT_SUPPORT "SGCT Spout support" OFF)
  else ()
    option(SGCT_SPOUT_SUPPORT "SGCT Spout support" ON)
  endif ()
  unset(sgct_target_arch)

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
# CPack configuration for package generation
if (PROJECT_IS_TOP_LEVEL)
  # CPack configuration variables
  set(CPACK_PACKAGE_NAME "sgct")
  set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
  set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Simple Graphics Cluster Toolkit")
  set(CPACK_PACKAGE_VENDOR "SGCT")
  set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/SGCT/SGCT")
  set(CPACK_PACKAGE_LICENSE "BSD-3-Clause")
  set(CPACK_PACKAGE_FILE_NAME "sgct-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}")
  set(CPACK_PACKAGE_INSTALL_DIRECTORY "sgct")
  set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
  set(CPACK_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install-root")
  set(CPACK_PACKAGE_CONTACT "SGCT Development Team <alexander.bock@liu.se>")

  # For multi-config generators (MSVC), set the install configuration BEFORE CPack
  # This is required because MSVC uses RelWithDebInfo, not Release
  if (MSVC)
    set(CMAKE_INSTALL_CONFIG_NAME "RelWithDebInfo" CACHE STRING "Install configuration" FORCE)
    # For Visual Studio multi-config generators, specify the config name in CPACK_INSTALL_CMAKE_PROJECTS
    # and use CPACK_INSTALLED_DIRECTORIES to tell CPack where the installed files are.
    set(CPACK_INSTALL_CMAKE_PROJECTS "${CMAKE_BINARY_DIR};sgct;RelWithDebInfo;/")
    set(CPACK_INSTALLED_DIRECTORIES "${CMAKE_BINARY_DIR}/install-root;/")
  else ()
    # For single-config generators, use the default install mechanism
    set(CPACK_INSTALL_CMAKE_PROJECTS "${CMAKE_BINARY_DIR};sgct;ALL;/")
  endif ()

  # Platform-specific generators
  if (WIN32)
    set(CPACK_GENERATOR "ZIP" "NSIS")
    set(CPACK_NSIS_PACKAGE_NAME "SGCT ${PROJECT_VERSION}")
    set(CPACK_NSIS_CONTACT "alexander.bock@liu.se")
    set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/SGCT/SGCT")
    set(CPACK_NSIS_DISPLAY_NAME "SGCT ${PROJECT_VERSION}")
    set(_NSIS_FINISHPAGE_RUN "\$INSTDIR\\\\bin\\\\calibrator.exe")
    set(CPACK_NSIS_DEFINES "
      !define MUI_STARTMENUPAGE_DEFAULTFOLDER SGCT
      !define MUI_FINISHPAGE_RUN ${_NSIS_FINISHPAGE_RUN}
    ")
  elseif (UNIX)
    set(CPACK_GENERATOR "ZIP" "TGZ" "DEB" "RPM")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
    set(CPACK_RPM_PACKAGE_ARCHITECTURE "x86_64")
  endif ()

  set(CPACK_SOURCE_GENERATOR "ZIP")
  set(CPACK_SOURCE_IGNORE_FILES
    "\\\\."
    "\\\\.gitignore"
    "\\\\.gitattributes"
    "/build/"
    "/\\.vs/"
    "\\\\.swp$"
    "\\\\(build\\\\)"
  )

  include(CPack)
endif ()





