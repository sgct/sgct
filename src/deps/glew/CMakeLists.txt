cmake_minimum_required(VERSION 2.6)
SET(CMAKE_DEBUG_POSTFIX "d" CACHE STRING "add a postfix, usually d on windows")
SET(CMAKE_RELEASE_POSTFIX "" CACHE STRING "add a postfix, usually empty on windows")
project(glew)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

if( MSVC )
	#disable pdb-file debugging
	#STRING(REPLACE "/Zi" "/Z7" replacementFlags ${CMAKE_CXX_FLAGS_DEBUG})
	#set(CMAKE_CXX_FLAGS_DEBUG "${replacementFlags}")
endif()

add_definitions(-DGLEW_STATIC)

if(MSVC AND NOT "${MSVC_VERSION}" LESS 1400)
	add_definitions( "/MP" )
endif()
add_library(glew STATIC src/glew.c include/GL/glew.h)