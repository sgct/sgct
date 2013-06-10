# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

cmake_minimum_required(VERSION 2.8)
SET(APP_NAME example1_opengl3)

SET(CMAKE_DEBUG_POSTFIX "d" CACHE STRING "add a postfix, usually d on windows")
SET(CMAKE_RELEASE_POSTFIX "" CACHE STRING "add a postfix, usually empty on windows")

PROJECT(${APP_NAME})
set(AppPath ${CMAKE_BINARY_DIR})
set(EXECUTABLE_OUTPUT_PATH ${AppPath})

add_executable(${APP_NAME}
	main.cpp
	SimpleVertexShader.vertexshader
	SimpleFragmentShader.fragmentshader)	
	
find_library(SGCT_RELEASE REQUIRED)
find_library(SGCT_DEBUG REQUIRED)
find_path(SGCT_INCLUDE REQUIRED)
include_directories(${SGCT_INCLUDE})

find_package(OpenGL REQUIRED)

if( MSVC )
	set(DEBUG_LIBS
		${SGCT_DEBUG}
		${OPENGL_LIBRARY}
	)
	set(RELEASE_LIBS
		${SGCT_RELEASE}
		${OPENGL_LIBRARY}
	)
elseif( MINGW )
	set(DEBUG_LIBS
		${SGCT_DEBUG}
		${OPENGL_LIBRARY}
		ws_32
	)
	set(RELEASE_LIBS
		${SGCT_RELEASE}
		${OPENGL_LIBRARY}
		ws_32
	)
endif()

if(MSVC AND NOT "${MSVC_VERSION}" LESS 1400)
	add_definitions( "/MP" )
endif()

if( WIN32 )
	add_definitions(-D__WIN32__)
elseif( APPLE  )
	add_definitions(-D__APPLE__)
else()
	add_definitions(-D__LINUX__)
endif()
	
if( CMAKE_COMPILER_IS_GNUCXX )
	set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -s")
endif()

target_link_libraries(${APP_NAME} debug ${DEBUG_LIBS})
target_link_libraries(${APP_NAME} optimized ${RELEASE_LIBS})