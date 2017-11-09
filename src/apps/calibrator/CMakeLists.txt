# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

cmake_minimum_required(VERSION 2.8)
SET(APP_NAME calibrator)

SET(CMAKE_DEBUG_POSTFIX "d" CACHE STRING "add a postfix, usually d on windows")
SET(CMAKE_RELEASE_POSTFIX "" CACHE STRING "add a postfix, usually empty on windows")

PROJECT(${APP_NAME})

macro (set_xcode_property TARGET XCODE_PROPERTY XCODE_VALUE)
set_property (TARGET ${TARGET} PROPERTY XCODE_ATTRIBUTE_${XCODE_PROPERTY}
	    ${XCODE_VALUE})
endmacro (set_xcode_property)

add_executable(${APP_NAME}
	main.cpp
	Dome.h
	Dome.cpp)

option(SGCT_PLACE_TARGETS_IN_SOURCE_TREE "Place targets in source tree" OFF)
option(SGCT_COPY_EXAMPLE_RESOURCES "Copy example resources" ON)
if( SGCT_PLACE_TARGETS_IN_SOURCE_TREE )
	set(EXAMPE_TARGET_PATH ${PROJECT_SOURCE_DIR})
else()
	set(EXAMPE_TARGET_PATH ${SGCT_EXAMPLE_OUTPUT_DIR}/${APP_NAME}) 
endif()
	
set(EXECUTABLE_OUTPUT_PATH ${EXAMPE_TARGET_PATH})
	
#set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_target_properties(${APP_NAME} PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY_DEBUG ${EXAMPE_TARGET_PATH}
	RUNTIME_OUTPUT_DIRECTORY_RELEASE ${EXAMPE_TARGET_PATH}
	FOLDER "Examples"
)

#copy data
if(SGCT_COPY_EXAMPLE_RESOURCES AND NOT SGCT_PLACE_TARGETS_IN_SOURCE_TREE )
	file(GLOB files
		"${PROJECT_SOURCE_DIR}/*.png"
		"${PROJECT_SOURCE_DIR}/*.jpg"
		"${PROJECT_SOURCE_DIR}/*.xml"
		"${PROJECT_SOURCE_DIR}/*.mpcdi"
		"${PROJECT_SOURCE_DIR}/*.vert*"
		"${PROJECT_SOURCE_DIR}/*.frag*")
	file(COPY ${files} DESTINATION ${EXAMPE_TARGET_PATH})
	if(WIN32)
		file(GLOB scripts "${PROJECT_SOURCE_DIR}/*.bat")
		file(COPY ${scripts} DESTINATION ${EXAMPE_TARGET_PATH})
	elseif(UNIX)
		file(GLOB scripts "${PROJECT_SOURCE_DIR}/*.command")
		file(COPY ${scripts} DESTINATION ${EXAMPE_TARGET_PATH})
	endif()
endif()		
	
if (MSVC)
	option(USE_MSVC_RUNTIMES "To use MSVC DLLs or to create a static build" ON)
endif()

if( APPLE )
	option(SGCT_CPP11 "Use libc++ instead of libstdc++" ON)
	set(CMAKE_OSX_ARCHITECTURES "x86_64")
	if(CMAKE_GENERATOR STREQUAL Xcode)
		set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9")
	endif()
endif()
	
if(NOT DEFINED SGCT_RELEASE_LIBRARY)
	if(SGCT_CPP11)	
		find_library(SGCT_RELEASE_LIBRARY
			NAMES sgct_cpp11 libsgct_cpp11 REQUIRED)
	else()
		find_library(SGCT_RELEASE_LIBRARY
			NAMES sgct libsgct REQUIRED)
	endif()
endif()
		
if(NOT DEFINED SGCT_DEBUG_LIBRARY)		
	if(SGCT_CPP11)	
		find_library(SGCT_DEBUG_LIBRARY 
			NAMES sgct_cpp11d libsgct_cpp11d REQUIRED)
	else()
		find_library(SGCT_DEBUG_LIBRARY 
			NAMES sgctd libsgctd REQUIRED)
	endif()
endif()

if(NOT DEFINED SGCT_INCLUDE_DIRECTORY)
	if( WIN32 )
		find_path(SGCT_INCLUDE_DIRECTORY 
			NAMES sgct
			PATHS $ENV{SGCT_ROOT_DIR}/include NO_DEFAULT_PATH
			REQUIRED)
	else()
		find_path(SGCT_INCLUDE_DIRECTORY 
			NAMES sgct
			PATH_SUFFIXES sgct
			PATHS $ENV{SGCT_ROOT_DIR}/include
			REQUIRED)
	endif()
endif()
set(GLFW_INCLUDE_DIRECTORY $ENV{SGCT_ROOT_DIR}/src/deps/glfw/include)
	
set(SGCT_LIBS
	debug ${SGCT_DEBUG_LIBRARY}
	optimized ${SGCT_RELEASE_LIBRARY}
)

find_package(OpenGL REQUIRED)

include_directories(${SGCT_INCLUDE_DIRECTORY})
include_directories(${GLFW_INCLUDE_DIRECTORY})

if( MSVC )
	set(LIBS
		${SGCT_LIBS}
		${OPENGL_gl_LIBRARY}
		ws2_32
	)
elseif( WIN32 ) #MINGW or similar
	set(LIBS
		${SGCT_LIBS}
		${OPENGL_gl_LIBRARY}
		ws2_32
	)
elseif( APPLE )
	find_library(COCOA_LIBRARY Cocoa REQUIRED)
	find_library(IOKIT_LIBRARY IOKit REQUIRED)
	find_library(COREVIDEO_LIBRARY CoreVideo REQUIRED)

	set(LIBS
		${SGCT_LIBS}
		${OPENGL_gl_LIBRARY}
		${COCOA_LIBRARY}
		${IOKIT_LIBRARY}
		${COREVIDEO_LIBRARY}
	)
else() #linux
	find_package(X11 REQUIRED)
	find_package(Threads REQUIRED)

	set(LIBS
		${SGCT_LIBS}
		${OPENGL_gl_LIBRARY}
		${X11_X11_LIB}
		${X11_Xrandr_LIB}
		${X11_Xinerama_LIB}
		${X11_Xinput_LIB}
		${X11_Xxf86vm_LIB}
		${X11_Xcursor_LIB}
		${CMAKE_THREAD_LIBS_INIT}		
	)
endif()

if( MSVC )
	#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LTCG")
	
		
	if (NOT USE_MSVC_RUNTIMES)
		foreach (flag ${CompilerFlags})
			if (${flag} MATCHES "/MD")
				string(REGEX REPLACE "/MD" "/MT" ${flag} "${${flag}}")
			endif()
			if (${flag} MATCHES "/MDd")
				string(REGEX REPLACE "/MDd" "/MTd" ${flag} "${${flag}}")
			endif()

		endforeach()

	endif()

	if( "${MSVC_VERSION}" LESS 1600 ) #less than visual studio 2010
		STRING(REPLACE "INCREMENTAL:YES" "INCREMENTAL:NO" replacementFlags
			${CMAKE_EXE_LINKER_FLAGS_DEBUG})
		SET(CMAKE_EXE_LINKER_FLAGS_DEBUG "${replacementFlags}" )
		
		STRING(REPLACE "INCREMENTAL:YES" "INCREMENTAL:NO" replacementFlags
			${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO})
		SET(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${replacementFlags}" )
	else()
		STRING(REPLACE "INCREMENTAL" "INCREMENTAL:NO" replacementFlags
			${CMAKE_EXE_LINKER_FLAGS_DEBUG})
		SET(CMAKE_EXE_LINKER_FLAGS_DEBUG "${replacementFlags}" )
		
		STRING(REPLACE "INCREMENTAL" "INCREMENTAL:NO" replacementFlags
			${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO})
		SET(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${replacementFlags}" )
	endif()
	
	#MESSAGE(STATUS "flags: ${CMAKE_EXE_LINKER_FLAGS_DEBUG}")
endif()
	
if(MSVC AND NOT "${MSVC_VERSION}" LESS 1400)
	add_definitions( "/MP" )
endif()

if( WIN32 )
	add_definitions(-D__WIN32__)
	if( MINGW )
		set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")
	endif()
elseif( APPLE  )
	add_definitions(-D__APPLE__)
	if(SGCT_CPP11)
		set(CMAKE_CXX_FLAGS "-std=c++11 -stdlib=libc++ ${CMAKE_CXX_FLAGS}")
		set_xcode_property(${APP_NAME} CLANG_CXX_LANGUAGE_STANDARD "c++11")
		set_xcode_property(${APP_NAME} CLANG_CXX_LIBRARY "libc++")
	else()
		set(CMAKE_CXX_FLAGS "-std=c++0x -stdlib=libstdc++ ${CMAKE_CXX_FLAGS}")
		set_xcode_property(${APP_NAME} CLANG_CXX_LANGUAGE_STANDARD "c++0x")
		set_xcode_property(${APP_NAME} CLANG_CXX_LIBRARY "libstdc++")
	endif()
else()
	add_definitions(-D__LINUX__)
	set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")
endif()
	
#if( CMAKE_COMPILER_IS_GNUCXX )
#	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++ -static")
#endif()
if( WIN32 )
  target_link_libraries(${APP_NAME} ${LIBS})
else()
  target_link_libraries(${APP_NAME} ${LIBS} ${CMAKE_DL_LIBS})
endif()
