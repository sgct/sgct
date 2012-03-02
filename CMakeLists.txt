# Copyright Linköping University 2011

project(SGCT)
cmake_minimum_required(VERSION 2.6.2)

#---------------------------------------------------------------------
# Path to the install directory
#
if(WIN32 AND CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files/SGCT"
    CACHE PATH "Where SGCT will be installed" FORCE)
endif(WIN32 AND CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

mark_as_advanced(CMAKE_INSTALL_PREFIX)

#--------------------------------------------------------------------
# Output paths for the executables and libraries
#
if(NOT EXECUTABLE_OUTPUT_PATH)
  set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin CACHE PATH 
    "Single output directory for building all executables.")
endif(NOT EXECUTABLE_OUTPUT_PATH)

if(NOT LIBRARY_OUTPUT_PATH)
  set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib CACHE PATH
    "Single output directory for building all libraries.")
endif(NOT LIBRARY_OUTPUT_PATH)

mark_as_advanced(EXECUTABLE_OUTPUT_PATH LIBRARY_OUTPUT_PATH)


#
# Path for custom cmake modules, like FindXPackage.cmake
#
#
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")


#
# Find modules, ex third party libs, include files
#
##TODO LINK WITH CORRECT LIBRARIES
find_package(GLFW REQUIRED)
INCLUDE_DIRECTORIES(${GLFW_INCLUDE_DIR})
find_package(GLEW REQUIRED)
INCLUDE_DIRECTORIES(${GLEW_INCLUDE_DIR})
find_package(PNG REQUIRED)
INCLUDE_DIRECTORIES(${PNG_INCLUDE_DIR})
find_package(freetype REQUIRED)	
INCLUDE_DIRECTORIES(${FREETYPE_INCLUDE_DIR})
find_package(TinyXML REQUIRED)		
INCLUDE_DIRECTORIES(${TINYXML_INCLUDE_DIR})

set(SGCT_DEPENDENCIES ${GLFW_LIBRARY} ${TINYXML_LIBRARY} ${GLEW_LIBRARY} ${PNG_LIBRARY} ${ZLIB_LIBRARY} ${FREETYPE_LIBRARY} )

message("${SGCT_DEPENDENCIES}")
set(SGCT_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/deps/tinyxml/" ${GLFW_INCLUDE_DIR} ${GLEW_INCLUDE_DIR} ${PNG_PNG_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR} ${FREETYPE_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/sgct/include )
message("${SGCT_INCLUDE_DIRS}")
set(SGCT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/sgct/src)
set(SGCT_BINARY_DIR ${CMAKE_BINARY_DIR})
set(SGCT_LIBRARY_DIR ${LIBRARY_OUTPUT_PATH})
set(SGCT_EXECUTABLE_DIR ${EXECUTABLE_OUTPUT_PATH})

#--------------------------------------------------------------------
# Boolean values to build the different projects
#
option(HAS_SGCT   "Required for SGCT modules"    ON)
option(HAS_EXAMPLE1   "Simple example"    ON)
#--------------------------------------------------------------------
# Build shared libs or static libs
option(BUILD_SHARED_LIBS "Build shared libs or static libs" ON)
#mark_as_advanced(BUILD_SHARED_LIBS)

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
# Locate SGCT libraries and include directories
#

# OpenGL
IF(APPLE)
   INCLUDE_DIRECTORIES(/System/Library/Frameworks)
   FIND_LIBRARY(GLUT_LIBRARY GLUT)
   FIND_LIBRARY(OpenGL_LIBRARY OpenGL)
	FIND_LIBRARY(COCOA_LIBRARY Cocoa)
   SET(EXTRA_LIBS ${COCOA_LIBRARY} ${GLUT_LIBRARY} ${OpenGL_LIBRARY})
   SET( OpenGL_INCLUDE_DIR /usr/include/GL)
   INCLUDE_DIRECTORIES(${OpenGL_INCLUDE_DIR})
ELSE(APPLE)
   SET( OpenGL_INCLUDE_DIR /usr/include/GL)
   INCLUDE_DIRECTORIES(${OpenGL_INCLUDE_DIR})
   SET( OpenGL_LIBRARIES GL)
ENDIF(APPLE)

#--------------------------------------------------------------------
# Set preprocessor definition for static or shared build
# This variable should be used by all libraries in the subdirectories.
# Set the LIB_TYPE variable to STATIC

#SET (LIB_TYPE STATIC)
#IF (BUILD_SHARED_LIBS)
#  # User wants to build Dynamic Libraries, so change the LIB_TYPE variable to CMake keyword 'SHARED'
#  SET (LIB_TYPE SHARED)
#  add_definitions(-DSGCT_ALL_DYN_LINK)
#ENDIF (BUILD_SHARED_LIBS)

IF (BUILD_SHARED_LIBS)
  add_definitions(-DSGCT_ALL_DYN_LINK)
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
# Build modules
#
add_subdirectory(deps/tinyxml)
add_subdirectory(sgct)
IF (HAS_EXAMPLE1)
  add_subdirectory(example1)
ENDIF (HAS_EXAMPLE1)
#--------------------------------------------------------------------
# Write config.xml
#
#--------------------------------------------------------------------

