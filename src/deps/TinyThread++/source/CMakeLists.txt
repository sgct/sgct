# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

cmake_minimum_required(VERSION 2.8)

SET(BUILD_SHARED_LIBS OFF)
SET(LIB_NAME tinythreadpp)

SET(CMAKE_DEBUG_POSTFIX "d" CACHE STRING "add a postfix, usually d on windows")
SET(CMAKE_RELEASE_POSTFIX "" CACHE STRING "add a postfix, usually empty on windows")

macro (set_xcode_property TARGET XCODE_PROPERTY XCODE_VALUE)
set_property (TARGET ${TARGET} PROPERTY XCODE_ATTRIBUTE_${XCODE_PROPERTY}
	    ${XCODE_VALUE})
endmacro (set_xcode_property)

PROJECT(${LIB_NAME})
set(LibPath ${CMAKE_BINARY_DIR})
set(EXECUTABLE_OUTPUT_PATH ${LibPath})
	
add_library(${LIB_NAME} STATIC tinythread.cpp)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

find_package(Threads REQUIRED)

if(MSVC AND NOT "${MSVC_VERSION}" LESS 1400)
	add_definitions( "/MP" )
endif()

if(MSVC)
	#disable pdb-file debugging
	#STRING(REPLACE "/Zi" "/Z7" replacementFlags ${CMAKE_CXX_FLAGS_DEBUG})
	#set(CMAKE_CXX_FLAGS_DEBUG "${replacementFlags}")
endif()

if( WIN32 )
	add_definitions(-D__WIN32__)
	if( MINGW )
		set(CMAKE_CXX_FLAGS "-std=c++0x ${CMAKE_CXX_FLAGS}")
	endif()
elseif( APPLE  )
	add_definitions(-D__APPLE__)
	if(SGCT_CPP11)
		set(CMAKE_CXX_FLAGS "-std=c++11 -stdlib=libc++ ${CMAKE_CXX_FLAGS}")
		set_xcode_property(${LIB_NAME} CLANG_CXX_LANGUAGE_STANDARD "c++11")
		set_xcode_property(${LIB_NAME} CLANG_CXX_LIBRARY "libc++")
	else()
		set(CMAKE_CXX_FLAGS "-std=c++11 -stdlib=libstdc++ ${CMAKE_CXX_FLAGS}")
		set_xcode_property(${LIB_NAME} CLANG_CXX_LANGUAGE_STANDARD "c++11")
		set_xcode_property(${LIB_NAME} CLANG_CXX_LIBRARY "libstdc++")
	endif()
else()
	add_definitions(-D__LINUX__)
	set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")
endif()
	
#if( CMAKE_COMPILER_IS_GNUCXX )
#	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++ -static")
#endif()

target_link_libraries( ${LIB_NAME} ${THREADS_LIBRARY} )
