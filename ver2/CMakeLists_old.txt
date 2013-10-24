# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

#
# General project settings
#
cmake_minimum_required(VERSION 2.6.2)

project(SGCT C CXX)

#
# Path for custom cmake modules, like FindXPackage.cmake
#
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

#------------------------------------
#  Build Type
#  Release/Debug/RelWithDebInfo/MinSizeRel...
set(CMAKE_BUILD_TYPE RelWithDebInfo)

#--------------------------------------------------------------------
# Output paths for the executables and libraries
#
if(NOT EXECUTABLE_OUTPUT_PATH)
  set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin CACHE PATH 
    "Single output directory for building all executables.")
endif(NOT EXECUTABLE_OUTPUT_PATH)

if(NOT LIBRARY_OUTPUT_PATH)
  set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib CACHE PATH
    "DEFAULT LIBRARY PATH.")
endif(NOT LIBRARY_OUTPUT_PATH)

mark_as_advanced(EXECUTABLE_OUTPUT_PATH LIBRARY_OUTPUT_PATH)

#
# Build Dependencies
# 
INCLUDE_DIRECTORIES(
  src/
  src/deps/freetype/include
  src/deps/glew/include
  src/deps/glfw/include
  src/deps/lpng/include
  src/deps/tinyxml/include
  src/deps/zlib/include
)


#
# Find modules, ex third party libs, include files
#
##TODO BUILD FROM SOURCE
#find_package(GLEW REQUIRED)
#INCLUDE_DIRECTORIES(${GLEW_INCLUDE_DIR})
#havent been able to make a cmake file for GLFW 2.7 yet
find_package(GLFW REQUIRED)
INCLUDE_DIRECTORIES(${GLFW_INCLUDE_DIR})


# OpenGL
find_package(OpenGL REQUIRED)
INCLUDE_DIRECTORIES(${OPENGL_INCLUDE_DIR})
SET(GL_LIBS ${OPENGL_gl_LIBRARY})

#USE x11
IF(UNIX AND NOT APPLE)
	INCLUDE_DIRECTORIES(${X11_X11_INCLUDE_PATH})
    list(APPEND GL_LIBS ${X11_X11_LIB} ${OPENGL_gl_LIBRARY})
ENDIF(UNIX AND NOT APPLE)

#USE COCOA
IF(UNIX AND APPLE)
   FIND_LIBRARY(COCOA_LIBRARY Cocoa)
   list(APPEND GL_LIBS ${COCOA_LIBRARY})
ENDIF(UNIX AND APPLE)

set(SGCT_DEPENDENCIES ${GL_LIBS} ${GLFW_LIBRARY} glew tinyxml zlib png15 freetype)

#--------------------------------------------------------------------
# Project Paths
#
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/sgct/include)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include)
LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/lib)

set(SGCT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/sgct/src)
set(SGCT_BINARY_DIR ${CMAKE_BINARY_DIR})
set(SGCT_LIBRARY_DIR ${LIBRARY_OUTPUT_PATH})
set(SGCT_EXECUTABLE_DIR ${EXECUTABLE_OUTPUT_PATH})

#--------------------------------------------------------------------
# Boolean values to build the different projects
option(WITH_EXAMPLES  "Compile example projects"   ON)

#--------------------------------------------------------------------
# Build shared libs or static libs
option(BUILD_SHARED_LIBS "Build shared libs or static libs" OFF)
mark_as_advanced(BUILD_SHARED_LIBS)

#--------------------------------------------------------------------
# If the generator is VS8, set the compiler to VS8
#
if(${CMAKE_GENERATOR} MATCHES "Visual Studio 8 2005")
  if(NOT ${CMAKE_CXX_COMPILER} MATCHES "8")
    string(REPLACE "9.0" "8" CMAKE_CXX_COMPILER ${CMAKE_CXX_COMPILER})
    string(REPLACE "9.0" "8" CMAKE_C_COMPILER   ${CMAKE_C_COMPILER})
  endif(NOT ${CMAKE_CXX_COMPILER} MATCHES "8")
endif(${CMAKE_GENERATOR} MATCHES "Visual Studio 8 2005")

#--------------------------------------------------------------------
# Disable deprecation warnings for standard C functions.
#
if(CMAKE_COMPILER_2005)
  add_definitions(-D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE)
endif(CMAKE_COMPILER_2005)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  add_definitions(-DDARWIN)
endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

#--------------------------------------------------------------------
# Set preprocessor definition for static or shared build
# This variable should be used by all libraries in the subdirectories.
# Set the LIB_TYPE variable to STATIC
#--------------------------------------------------------------------
SET(BUILD_SHARED_LIBS OFF)
IF (BUILD_SHARED_LIBS)
  add_definitions(-DSGCT_ALL_DYN_LINK)
  SET(BUILD_SHARED_LIBS ON)
ENDIF (BUILD_SHARED_LIBS)

#--------------------------------------------------------------------
# Disable deprecation warnings for standard C functions
#
if(CMAKE_COMPILER_2005)
  add_definitions(-D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE)
endif(CMAKE_COMPILER_2005)

#--------------------------------------------------------------------
# Set preprocessor definition to indicate whether 
# to use the debug postfix
#
if(DEBUG_POSTFIX)
	add_definitions(-D_DEBUG_POSTFIX)
endif(DEBUG_POSTFIX)

#--------------------------------------------------------------------
# Build
#--------------------------------------------------------------------
ADD_SUBDIRECTORY(src/deps)
ADD_SUBDIRECTORY(src/sgct)

IF(WITH_EXAMPLES)
  add_subdirectory(src/apps)
ENDIF(WITH_EXAMPLES)

