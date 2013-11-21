# CMakeLists.txt

# Copyright (C) 2007-2011 Glenn Randers-Pehrson

# This code is released under the libpng license.
# For conditions of distribution and use, see the disclaimer
# and license in png.h

cmake_minimum_required(VERSION 2.4.4)
set(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS true)
SET(CMAKE_DEBUG_POSTFIX "d" CACHE STRING "add a postfix, usually d on windows")
SET(CMAKE_RELEASE_POSTFIX "" CACHE STRING "add a postfix, usually empty on windows")

ADD_SUBDIRECTORY(../zlib "${CMAKE_CURRENT_BINARY_DIR}/zlib")

if(UNIX AND NOT DEFINED CMAKE_BUILD_TYPE)
  if(CMAKE_MAJOR_VERSION EQUAL 2 AND CMAKE_MINOR_VERSION EQUAL 4)
    # workaround CMake 2.4.x bug
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING
        "Choose the type of build, options are:
           None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used)
           Debug
           Release
           RelWithDebInfo
           MinSizeRel.")
  else()
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING
        "Choose the type of build, options are:
           None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used)
           Debug
           Release
           RelWithDebInfo
           MinSizeRel.")
  endif()
endif()

project(libpng C)
enable_testing()

set(PNGLIB_MAJOR 1)
set(PNGLIB_MINOR 5)
set(PNGLIB_RELEASE 14)
set(PNGLIB_NAME libpng${PNGLIB_MAJOR}${PNGLIB_MINOR})
set(PNGLIB_VERSION ${PNGLIB_MAJOR}.${PNGLIB_MINOR}.${PNGLIB_RELEASE})

# needed packages
#find_package(ZLIB REQUIRED)
include_directories(../zlib)
#include zconf.h
include_directories("${CMAKE_CURRENT_BINARY_DIR}/zlib")

if(NOT WIN32)
  find_library(M_LIBRARY
    NAMES m
    PATHS /usr/lib /usr/local/lib
  )
  if(NOT M_LIBRARY)
    message(STATUS
      "math library 'libm' not found - floating point support disabled")
  endif()
else()
  # not needed on windows
  set(M_LIBRARY "")
endif()

# SET LIBNAME
set(PNG_LIB_NAME png${PNGLIB_MAJOR}${PNGLIB_MINOR})

# Use the prebuilt pnglibconf.h file from the scripts folder
# TODO: fix this by building with awk; without this no cmake build can be
# configured directly (to do so indirectly use your local awk to build a
# pnglibconf.h in the build directory.)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/scripts/pnglibconf.h.prebuilt
               ${CMAKE_CURRENT_BINARY_DIR}/pnglibconf.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# OUR SOURCES
set(libpng_public_hdrs
  png.h
  pngconf.h
  ${CMAKE_CURRENT_BINARY_DIR}/pnglibconf.h
)
set(libpng_sources
  ${libpng_public_hdrs}
  pngdebug.h
  pnginfo.h
  pngpriv.h
  pngstruct.h
  png.c
  pngerror.c
  pngget.c
  pngmem.c
  pngpread.c
  pngread.c
  pngrio.c
  pngrtran.c
  pngrutil.c
  pngset.c
  pngtrans.c
  pngwio.c
  pngwrite.c
  pngwtran.c
  pngwutil.c
)
# SOME NEEDED DEFINITIONS

add_definitions(-DPNG_CONFIGURE_LIBPNG)

if(MSVC)
  add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
  
  #disable pdb-file debugging
  #STRING(REPLACE "/Zi" "/Z7" replacementFlags ${CMAKE_CXX_FLAGS_DEBUG})
  #set(CMAKE_CXX_FLAGS_DEBUG "${replacementFlags}")
  
  if(NOT "${MSVC_VERSION}" LESS 1400)
	add_definitions( "/MP" )
  endif()
endif(MSVC)

# NOW BUILD OUR TARGET
include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${ZLIB_SRCS})

# does not work without changing name
set(PNG_LIB_NAME_STATIC ${PNG_LIB_NAME}_static)
add_library(${PNG_LIB_NAME_STATIC} STATIC ${libpng_sources})
if(MSVC)
	# msvc does not append 'lib' - do it here to have consistent name
	set_target_properties(${PNG_LIB_NAME_STATIC} PROPERTIES PREFIX "lib")
endif()
target_link_libraries(${PNG_LIB_NAME_STATIC} ${zlibstatic} ${M_LIBRARY})

# Ensure the CMAKE_LIBRARY_OUTPUT_DIRECTORY is set
IF(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
  SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY "lib")
ENDIF(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)

# Set a variable with CMake code which:
# Creates a symlink from src to dest (if possible) or alternatively
# copies if different.
macro(CREATE_SYMLINK SRC_FILE DEST_FILE)
  FILE(REMOVE ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${DEST_FILE})
  if(WIN32 AND NOT CYGWIN AND NOT MSYS AND NOT MINGW)
    ADD_CUSTOM_COMMAND(
        OUTPUT ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${DEST_FILE}   ${CMAKE_CURRENT_BINARY_DIR}/${DEST_FILE}
        COMMAND ${CMAKE_COMMAND} -E copy        ${CMAKE_CURRENT_BINARY_DIR}/${SRC_FILE} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${DEST_FILE}
        COMMAND ${CMAKE_COMMAND} -E copy        ${CMAKE_CURRENT_BINARY_DIR}/${SRC_FILE} ${CMAKE_CURRENT_BINARY_DIR}/${DEST_FILE}
        DEPENDS ${PNG_LIB_NAME} ${PNG_LIB_NAME_STATIC}
        )
    ADD_CUSTOM_TARGET(${DEST_FILE}_COPY ALL DEPENDS ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${DEST_FILE})
  else()
    execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${SRC_FILE} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${DEST_FILE} WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${SRC_FILE} ${DEST_FILE} WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
  endif()
endmacro()

# libpng is a library so default to 'lib'
if(NOT DEFINED CMAKE_INSTALL_LIBDIR)
  set(CMAKE_INSTALL_LIBDIR lib)
endif(NOT DEFINED CMAKE_INSTALL_LIBDIR)

# CREATE PKGCONFIG FILES
# we use the same files like ./configure, so we have to set its vars
# Only do this on Windows for Cygwin - the files don't make much sense outside
# a UNIX look alike
if(NOT WIN32 OR CYGWIN OR MINGW) 
  set(prefix      ${CMAKE_INSTALL_PREFIX})
  set(exec_prefix ${CMAKE_INSTALL_PREFIX})
  set(libdir      ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR})
  set(includedir  ${CMAKE_INSTALL_PREFIX}/include)
  set(LIBS        "-lz -lm")
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/libpng.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/${PNGLIB_NAME}.pc @ONLY)
  CREATE_SYMLINK(${PNGLIB_NAME}.pc libpng.pc)

  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/libpng-config.in
    ${CMAKE_CURRENT_BINARY_DIR}/${PNGLIB_NAME}-config @ONLY)
  CREATE_SYMLINK(${PNGLIB_NAME}-config libpng-config)
endif(NOT WIN32 OR CYGWIN OR MINGW)

# MSVC doesn't use a different file extension for shared vs. static
# libs.  We are able to change OUTPUT_NAME to remove the _static
# for all other platforms.
if(NOT MSVC)
set_target_properties(${PNG_LIB_NAME_STATIC} PROPERTIES
  OUTPUT_NAME ${PNG_LIB_NAME}
  CLEAN_DIRECT_OUTPUT 1)
endif()

# If CMake > 2.4.x, we set a variable used below to export
# targets to an export file.
# TODO: Use VERSION_GREATER after our cmake_minimum_required >= 2.6.2
if(CMAKE_MAJOR_VERSION GREATER 1 AND CMAKE_MINOR_VERSION GREATER 4)
  set(PNG_EXPORT_RULE EXPORT libpng)
elseif(CMAKE_MAJOR_VERSION GREATER 2) # future proof
  set(PNG_EXPORT_RULE EXPORT libpng)
endif()

# INSTALL
if(NOT SKIP_INSTALL_LIBRARIES AND NOT SKIP_INSTALL_ALL )
install(TARGETS ${PNG_LIB_NAME_STATIC}
	${PNG_EXPORT_RULE}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
if(NOT WIN32 OR CYGWIN OR MINGW)
  get_target_property(BUILD_TARGET_LOCATION ${PNG_LIB_NAME_STATIC} LOCATION_${CMAKE_BUILD_TYPE})
  get_filename_component(BUILD_TARGET_FILE ${BUILD_TARGET_LOCATION} NAME)
  CREATE_SYMLINK(${BUILD_TARGET_FILE} libpng${CMAKE_STATIC_LIBRARY_SUFFIX})
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libpng${CMAKE_STATIC_LIBRARY_SUFFIX}
	 DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif(NOT WIN32 OR CYGWIN OR MINGW)
endif()

if(NOT SKIP_INSTALL_HEADERS AND NOT SKIP_INSTALL_ALL )
  install(FILES ${libpng_public_hdrs}   DESTINATION include)
  install(FILES ${libpng_public_hdrs}   DESTINATION include/${PNGLIB_NAME})
endif()
if(NOT SKIP_INSTALL_EXECUTABLES AND NOT SKIP_INSTALL_ALL )
  if(NOT WIN32 OR CYGWIN OR MINGW)
    install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/libpng-config DESTINATION bin)
    install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${PNGLIB_NAME}-config
            DESTINATION bin)
  endif(NOT WIN32 OR CYGWIN OR MINGW)
endif()

if(NOT SKIP_INSTALL_FILES AND NOT SKIP_INSTALL_ALL )
  # Install man pages
  if(NOT PNG_MAN_DIR)
    set(PNG_MAN_DIR "share/man")
  endif()
  install(FILES libpng.3 libpngpf.3      DESTINATION ${PNG_MAN_DIR}/man3)
  install(FILES png.5                    DESTINATION ${PNG_MAN_DIR}/man5)
  # Install pkg-config files
  if(NOT WIN32 OR CYGWIN OR MINGW)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libpng.pc
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
    install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/libpng-config
            DESTINATION bin)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PNGLIB_NAME}.pc
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
    install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${PNGLIB_NAME}-config
            DESTINATION bin)
  endif(NOT WIN32 OR CYGWIN OR MINGW)
endif()

# what's with libpng-$VER%.txt and all the extra files?

# UNINSTALL
# do we need this?

# DIST
# do we need this?

# to create msvc import lib for mingw compiled shared lib
# pexports libpng.dll > libpng.def
# lib /def:libpng.def /machine:x86

