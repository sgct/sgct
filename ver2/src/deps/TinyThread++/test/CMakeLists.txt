# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

cmake_minimum_required(VERSION 2.8)
SET(APP_NAME fractal)

SET(CMAKE_DEBUG_POSTFIX "d" CACHE STRING "add a postfix, usually d on windows")
SET(CMAKE_RELEASE_POSTFIX "" CACHE STRING "add a postfix, usually empty on windows")

PROJECT(${APP_NAME})
set(AppPath ${CMAKE_BINARY_DIR})
set(EXECUTABLE_OUTPUT_PATH ${AppPath})

add_executable(${APP_NAME}
	fractal.cpp)	
	
find_library(TINY_THREAD_RELEASE_LIBRARY REQUIRED)
find_library(TINY_THREAD_DEBUG_LIBRARY REQUIRED)
find_path(TINY_THREAD_INCLUDE_DIRECTORY REQUIRED)
include_directories(${TINY_THREAD_INCLUDE_DIRECTORY})

find_package(OpenGL REQUIRED)

set(DEBUG_LIBS
	${TINY_THREAD_DEBUG_LIBRARY}
)

set(RELEASE_LIBS
	${TINY_THREAD_RELEASE_LIBRARY}
)

if(MSVC AND NOT "${MSVC_VERSION}" LESS 1400)
	add_definitions( "/MP" )
endif()
	
if( CMAKE_COMPILER_IS_GNUCXX )
	set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -s")
endif()

target_link_libraries(${APP_NAME} debug ${DEBUG_LIBS})
target_link_libraries(${APP_NAME} optimized ${RELEASE_LIBS})