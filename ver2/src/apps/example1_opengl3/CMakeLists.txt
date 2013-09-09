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
	main.cpp)	
	
find_library(SGCT_RELEASE_LIBRARY REQUIRED)
find_library(SGCT_DEBUG_LIBRARY REQUIRED)
find_path(SGCT_INCLUDE_DIRECTORY 
	NAMES sgct
	PATHS $ENV{SGCT_ROOT_DIR}/include NO_DEFAULT_PATH
	REQUIRED)
	
set(SGCT_LIBS
	debug ${SGCT_DEBUG_LIBRARY}
	optimized ${SGCT_RELEASE_LIBRARY}
)

find_package(OpenGL REQUIRED)

include_directories(${SGCT_INCLUDE_DIRECTORY})

if( MSVC )
	set(LIBS
		${SGCT_LIBS}
		${OPENGL_LIBRARY}
	)
elseif( WIN32 ) #MINGW or similar
	set(LIBS
		${SGCT_LIBS}
		${OPENGL_LIBRARY}
		ws2_32
	)
elseif( APPLE )
	find_library(COCOA_LIBRARY Cocoa REQUIRED)
	find_library(IOKIT_LIBRARY IOKit REQUIRED)

	set(LIBS
		${SGCT_LIBS}
		${OPENGL_LIBRARY}
		${COCOA_LIBRARY}
		${IOKIT_LIBRARY}
	)
else() #linux
	find_library(X11_LIBRARY X11 REQUIRED)
    find_library(XRANDR_LIBRARY Xrandr REQUIRED)
	find_library(XI_LIBRARY Xi REQUIRED)

	set(LIBS
		${SGCT_LIBS}
		${OPENGL_LIBRARY}
		${X11_LIBRARY}
		${XRANDR_LIBRARY}
		${XI_LIBRARY}
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

target_link_libraries(${APP_NAME} ${LIBS})
