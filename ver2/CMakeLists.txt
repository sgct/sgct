# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

#
# General project settings
#
cmake_minimum_required(VERSION 2.8)

SET(BUILD_SHARED_LIBS OFF)
SET(LIB_NAME sgct_light)
PROJECT(${LIB_NAME})

SET(CMAKE_DEBUG_POSTFIX "d" CACHE STRING "add a postfix, usually d on windows")
SET(CMAKE_RELEASE_POSTFIX "" CACHE STRING "add a postfix, usually empty on windows")

option(SGCT_BUILD_DEPS "Build dependencies" ON)
option(SGCT_BUILD_ALUT "Build ALUT" OFF)
option(SGCT_PLACE_TARGETS_IN_SOURCE_TREE "Place targets in source tree" ON)

if( APPLE  )
	#OSG is not compatible with c++11, it also makes it harder for the users defining all flags
	option(SGCT_CPP11 "Use c++11" OFF)
	set(CMAKE_OSX_ARCHITECTURES "x86_64")
	if(CMAKE_GENERATOR STREQUAL Xcode)
		set(CMAKE_OSX_DEPLOYMENT_TARGET "10.8")
	endif()
endif()

if( SGCT_PLACE_TARGETS_IN_SOURCE_TREE )
	set(SGCT_TARGET_PATH ${PROJECT_SOURCE_DIR}) 
else()
	set(SGCT_TARGET_PATH ${PROJECT_BINARY_DIR}) 
endif()

if( APPLE )
	set(SGCT_LIB_PATH "${SGCT_TARGET_PATH}/lib/mac" CACHE PATH "SGCT lib path")
	set(SGCT_LIB_SRC_PATH "${PROJECT_SOURCE_DIR}/lib/mac" CACHE PATH "SGCT src lib path")
elseif( WIN32 AND CMAKE_COMPILER_IS_GNUCXX )
	set(SGCT_LIB_PATH "${SGCT_TARGET_PATH}/lib/mingw" CACHE PATH "SGCT lib path")
	set(SGCT_LIB_SRC_PATH "${PROJECT_SOURCE_DIR}/lib/mac" CACHE PATH "SGCT src lib path")
elseif( MSVC )
	if( CMAKE_GENERATOR STREQUAL "Visual Studio 9 2008" OR CMAKE_GENERATOR STREQUAL "Visual Studio 9 2008" )
		set(SGCT_LIB_PATH "${SGCT_TARGET_PATH}/lib/msvc9" CACHE PATH "SGCT lib path")
		set(SGCT_LIB_SRC_PATH "${PROJECT_SOURCE_DIR}/lib/msvc9" CACHE PATH "SGCT src lib path")
	elseif( CMAKE_GENERATOR STREQUAL "Visual Studio 9 2008 Win64" OR CMAKE_GENERATOR STREQUAL "Visual Studio 9 2008 Win64" )
		set(SGCT_LIB_PATH "${SGCT_TARGET_PATH}/lib/msvc9_x64" CACHE PATH "SGCT lib path")
		set(SGCT_LIB_SRC_PATH "${PROJECT_SOURCE_DIR}/lib/msvc9_x64" CACHE PATH "SGCT src lib path")
	elseif( CMAKE_GENERATOR STREQUAL "Visual Studio 10" OR CMAKE_GENERATOR STREQUAL "Visual Studio 10 2010" )
		set(SGCT_LIB_PATH "${SGCT_TARGET_PATH}/lib/msvc10" CACHE PATH "SGCT lib path")
		set(SGCT_LIB_SRC_PATH "${PROJECT_SOURCE_DIR}/lib/msvc10" CACHE PATH "SGCT src lib path")
	elseif( CMAKE_GENERATOR STREQUAL "Visual Studio 10 Win64" OR CMAKE_GENERATOR STREQUAL "Visual Studio 10 2010 Win64" )
		set(SGCT_LIB_PATH "${SGCT_TARGET_PATH}/lib/msvc10_x64" CACHE PATH "SGCT lib path")
		set(SGCT_LIB_SRC_PATH "${PROJECT_SOURCE_DIR}/lib/msvc10_x64" CACHE PATH "SGCT src lib path")
	elseif( CMAKE_GENERATOR STREQUAL "Visual Studio 11" OR CMAKE_GENERATOR STREQUAL "Visual Studio 11 2012" )
		set(SGCT_LIB_PATH "${SGCT_TARGET_PATH}/lib/msvc11" CACHE PATH "SGCT lib path")
		set(SGCT_LIB_SRC_PATH "${PROJECT_SOURCE_DIR}/lib/msvc11" CACHE PATH "SGCT src lib path")
	elseif( CMAKE_GENERATOR STREQUAL "Visual Studio 11 Win64" OR CMAKE_GENERATOR STREQUAL "Visual Studio 11 2012 Win64" )
		set(SGCT_LIB_PATH "${SGCT_TARGET_PATH}/lib/msvc11_x64" CACHE PATH "SGCT lib path")
		set(SGCT_LIB_SRC_PATH "${PROJECT_SOURCE_DIR}/lib/msvc11_x64" CACHE PATH "SGCT src lib path")
	elseif( CMAKE_GENERATOR STREQUAL "Visual Studio 12" OR CMAKE_GENERATOR STREQUAL "Visual Studio 12 2013")
		set(SGCT_LIB_PATH "${SGCT_TARGET_PATH}/lib/msvc12" CACHE PATH "SGCT lib path")
		set(SGCT_LIB_SRC_PATH "${PROJECT_SOURCE_DIR}/lib/msvc12" CACHE PATH "SGCT src lib path")
	elseif( CMAKE_GENERATOR STREQUAL "Visual Studio 12 Win64" OR CMAKE_GENERATOR STREQUAL "Visual Studio 12 2013 Win64")
		set(SGCT_LIB_PATH "${SGCT_TARGET_PATH}/lib/msvc12_x64" CACHE PATH "SGCT lib path")
		set(SGCT_LIB_SRC_PATH "${PROJECT_SOURCE_DIR}/lib/msvc12_x64" CACHE PATH "SGCT src lib path")
	else()
		set(SGCT_LIB_PATH "${SGCT_TARGET_PATH}/lib/${CMAKE_GENERATOR}" CACHE PATH "SGCT lib path")
		set(SGCT_LIB_SRC_PATH "${PROJECT_SOURCE_DIR}/lib/${CMAKE_GENERATOR}" CACHE PATH "SGCT src lib path")
	endif()
elseif( UNIX AND NOT APPLE )
	if( CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(SGCT_LIB_PATH "${SGCT_TARGET_PATH}/lib/linux_x64" CACHE PATH "SGCT lib path")
		set(SGCT_LIB_SRC_PATH "${PROJECT_SOURCE_DIR}/lib/linux_x64" CACHE PATH "SGCT src lib path")
	elseif( CMAKE_SYSTEM_NAME STREQUAL "Linux")
		set(SGCT_LIB_PATH "${SGCT_TARGET_PATH}/lib/linux" CACHE PATH "SGCT lib path")
		set(SGCT_LIB_SRC_PATH "${PROJECT_SOURCE_DIR}/lib/linux" CACHE PATH "SGCT src lib path")
	else()
		set(SGCT_LIB_PATH "${SGCT_TARGET_PATH}/lib/unix" CACHE PATH "SGCT lib path")
		set(SGCT_LIB_SRC_PATH "${PROJECT_SOURCE_DIR}/lib/unix" CACHE PATH "SGCT src lib path")
	endif()
else()
	set(SGCT_LIB_PATH "${SGCT_TARGET_PATH}/lib/${CMAKE_SYSTEM_NAME}/${CMAKE_GENERATOR}" CACHE PATH "SGCT lib path")
	set(SGCT_LIB_SRC_PATH "${PROJECT_SOURCE_DIR}/lib/${CMAKE_SYSTEM_NAME}/${CMAKE_GENERATOR}" CACHE PATH "SGCT src lib path")
endif()

if(APPLE AND SGCT_CPP11)
	set(SGCT_LIB_PATH "${SGCT_LIB_PATH}_cpp11")
	set(SGCT_LIB_SRC_PATH "${SGCT_LIB_SRC_PATH}_cpp11")
endif()

set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${SGCT_LIB_PATH}" )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${SGCT_LIB_PATH}" )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${SGCT_LIB_PATH}" )

set(LibPath "${SGCT_LIB_PATH}")
set(EXECUTABLE_OUTPUT_PATH ${LibPath})

file (GLOB_RECURSE sgctSRC "${CMAKE_CURRENT_SOURCE_DIR}/src/sgct/*.cpp")
file (GLOB_RECURSE sgctInc "${CMAKE_CURRENT_SOURCE_DIR}/include/sgct/*.h")

Set(ALL_SRC ${sgctSRC} ${sgctInc} "${CMAKE_CURRENT_SOURCE_DIR}/include/sgct.h")

#print all
FOREACH(ALL_SRCName ${ALL_SRC})
	MESSAGE(STATUS "Adding file: ${ALL_SRCName}")
ENDFOREACH(ALL_SRCName)

add_library(${LIB_NAME} STATIC
	${ALL_SRC}
	)
 
INCLUDE_DIRECTORIES(
  include
)

find_package(OpenGL REQUIRED)

if(SGCT_BUILD_DEPS)
	set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${SGCT_LIB_PATH}/deps" )
	set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${SGCT_LIB_PATH}/deps" )
	set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${SGCT_LIB_PATH}/deps" )
	set( CMAKE_EXECUTABLE_OUTPUT_DIRECTORY "${PROJECT_BUILD_DIR}/bin" )
	set( CMAKE_EXECUTABLE_OUTPUT_DIRECTORY_DEBUG "${PROJECT_BUILD_DIR}/bin" )
	set( CMAKE_EXECUTABLE_OUTPUT_DIRECTORY_RELEASE "${PROJECT_BUILD_DIR}/bin" )
	set( CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BUILD_DIR}/bin" )
	set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${PROJECT_BUILD_DIR}/bin" )
	set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${PROJECT_BUILD_DIR}/bin" )

	ADD_SUBDIRECTORY(src/deps)
	
	if(SGCT_BUILD_ALUT)
		ADD_SUBDIRECTORY(additional_deps/freealut-1.1.0-src)
	endif()
	
	set(COMMON_DEBUG_LIBS
		${OPENGL_LIBRARY}
	)

	set(COMMON_RELEASE_LIBS
		${OPENGL_LIBRARY}
	)
else()
	#find_library(GLEW_DEBUG_LIBRARY REQUIRED)
	#find_library(GLEW_RELEASE_LIBRARY REQUIRED)
	#find_library(GLFW3_DEBUG_LIBRARY REQUIRED)
	#find_library(GLFW3_RELEASE_LIBRARY REQUIRED)
	#find_library(GLFW3_DEBUG_LIBRARY REQUIRED)
	#find_library(GLFW3_RELEASE_LIBRARY REQUIRED)
	#find_library(FREETYPE2_DEBUG_LIBRARY REQUIRED)
	#find_library(FREETYPE2_RELEASE_LIBRARY REQUIRED)
	#find_library(VRPN_DEBUG_LIBRARY REQUIRED)
	#find_library(VRPN_RELEASE_LIBRARY REQUIRED)
	#find_library(PNG_DEBUG_LIBRARY REQUIRED)
	#find_library(PNG_RELEASE_LIBRARY REQUIRED)
	#find_library(Z_DEBUG_LIBRARY REQUIRED)
	#find_library(Z_RELEASE_LIBRARY REQUIRED)
	#find_library(TINYXML2_DEBUG_LIBRARY REQUIRED)
	#find_library(TINYXML2_RELEASE_LIBRARY REQUIRED)
	#find_library(TINYTHREAD_DEBUG_LIBRARY REQUIRED)
	#find_library(TINYTHREAD_RELEASE_LIBRARY REQUIRED)
	
	#set(COMMON_DEBUG_LIBS
	#	${GLEW_DEBUG_LIBRARY}
	#	${GLFW3_DEBUG_LIBRARY}
	#	${OPENGL_LIBRARY}
	#	${FREETYPE2_DEBUG_LIBRARY}
	#	${VRPN_DEBUG_LIBRARY}
	#	${PNG_DEBUG_LIBRARY}
	#	${Z_DEBUG_LIBRARY}
	#	${TINYXML2_DEBUG_LIBRARY}
	#	${TINYTHREAD_DEBUG_LIBRARY}
	#)

	#set(COMMON_RELEASE_LIBS
	#	${GLEW_RELEASE_LIBRARY}
	#	${GLFW3_RELEASE_LIBRARY}
	#	${OPENGL_LIBRARY}
	#	${FREETYPE2_RELEASE_LIBRARY}
	#	${VRPN_RELEASE_LIBRARY}
	#	${PNG_RELEASE_LIBRARY}
	#	${Z_RELEASE_LIBRARY}
	#	${TINYXML2_RELEASE_LIBRARY}
	#	${TINYTHREAD_RELEASE_LIBRARY}
	#)
	
	set(COMMON_DEBUG_LIBS
		${OPENGL_LIBRARY}
	)

	set(COMMON_RELEASE_LIBS
		${OPENGL_LIBRARY}
	)
endif()

if( MSVC )
	set(DEBUG_LIBS
		${COMMON_DEBUG_LIBS}
		ws2_32
	)
	
	set(RELEASE_LIBS
		${COMMON_RELEASE_LIBS}
		ws2_32
	)
elseif( WIN32 ) #MINGW or similar
	set(DEBUG_LIBS
		${COMMON_DEBUG_LIBS}
		ws2_32
	)
	
	set(RELEASE_LIBS
		${COMMON_RELEASE_LIBS}
		ws2_32
	)
elseif( APPLE )
	find_library(COCOA_LIBRARY Cocoa REQUIRED)
	find_library(IOKIT_LIBRARY IOKit REQUIRED)

	set(DEBUG_LIBS
		${COMMON_DEBUG_LIBS}
		${COCOA_LIBRARY}
		${IOKIT_LIBRARY}
	)
	
	set(RELEASE_LIBS
		${COMMON_RELEASE_LIBS}
		${COCOA_LIBRARY}
		${IOKIT_LIBRARY}
	)
else() #linux
	find_library(X11_LIBRARY X11 REQUIRED)
    find_library(XRANDR_LIBRARY Xrandr REQUIRED)
	find_library(XI_LIBRARY Xi REQUIRED)

	set(DEBUG_LIBS
		${COMMON_DEBUG_LIBS}
		${X11_LIBRARY}
		${XRANDR_LIBRARY}
		${XI_LIBRARY}
	)
	
	set(RELEASE_LIBS
		${COMMON_RELEASE_LIBS}
		${X11_LIBRARY}
		${XRANDR_LIBRARY}
		${XI_LIBRARY}
	)
endif()

if( MSVC )
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LTCG")
	#disable pdb-file debugging
	#STRING(REPLACE "/Zi" "/Z7" replacementFlags ${CMAKE_CXX_FLAGS_DEBUG})
	#set(CMAKE_CXX_FLAGS_DEBUG "${replacementFlags}")
endif()

if(MSVC AND NOT "${MSVC_VERSION}" LESS 1400)
	add_definitions( "/MP" )
endif()

if( WIN32 )
	add_definitions(-D__WIN32__)
elseif( APPLE  )
	add_definitions(-D__APPLE__)
	if( SGCT_CPP11 )
    	set(CMAKE_CXX_FLAGS "-std=c++11 -stdlib=libc++ ${CMAKE_CXX_FLAGS}")
	else()
		set(CMAKE_CXX_FLAGS "-std=c++0x -stdlib=libstdc++ ${CMAKE_CXX_FLAGS}")
	endif()
else()
	add_definitions(-D__LINUX__)
	set(CMAKE_CXX_FLAGS "-std=c++0x ${CMAKE_CXX_FLAGS}")
endif()

add_definitions(-DGLEW_STATIC)
	
if( CMAKE_COMPILER_IS_GNUCXX )
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++ -s")
endif()

target_link_libraries( ${LIB_NAME} debug ${DEBUG_LIBS} optimized ${RELEASE_LIBS})

#to make sure that everything is build when post_build events runs
add_dependencies(${LIB_NAME} freetype glew glfw png15_static tinythreadpp tinyxml2static zlibstatic vrpn)

if(WIN32)
	add_custom_command(TARGET ${LIB_NAME}
		POST_BUILD
		COMMAND "merge_libs.bat"
		WORKING_DIRECTORY ${SGCT_LIB_PATH})
	
	#copy stuff
	if( NOT SGCT_PLACE_TARGETS_IN_SOURCE_TREE )
		add_custom_command(TARGET ${LIB_NAME}
			PRE_BUILD
			COMMAND "merge_libs.bat"
			WORKING_DIRECTORY ${SGCT_LIB_PATH})
	endif()
else()
	add_custom_command(TARGET ${LIB_NAME}
		POST_BUILD
		COMMAND "./merge_libs.sh"
		WORKING_DIRECTORY ${SGCT_LIB_PATH})
endif()
