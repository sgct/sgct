# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

cmake_minimum_required(VERSION 2.8)
SET(APP_NAME osgExample)

SET(CMAKE_DEBUG_POSTFIX "d" CACHE STRING "add a postfix, usually d on windows")
SET(CMAKE_RELEASE_POSTFIX "" CACHE STRING "add a postfix, usually empty on windows")

PROJECT(${APP_NAME})

add_executable(${APP_NAME}
	main.cpp)
	
set(EXAMPE_TARGET_PATH ${PROJECT_SOURCE_DIR})
set(EXECUTABLE_OUTPUT_PATH ${EXAMPE_TARGET_PATH})
	
set_target_properties(${APP_NAME} PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY_DEBUG ${EXAMPE_TARGET_PATH}
	RUNTIME_OUTPUT_DIRECTORY_RELEASE ${EXAMPE_TARGET_PATH}
)
	
if( APPLE  )
	set(CMAKE_OSX_ARCHITECTURES "x86_64")
	if(CMAKE_GENERATOR STREQUAL Xcode)
		set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9")
	endif()
endif()
	
find_library(SGCT_RELEASE_LIBRARY
	NAMES sgct libsgct REQUIRED)
find_library(SGCT_DEBUG_LIBRARY 
	NAMES sgctd libsgctd REQUIRED)

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
	
set(SGCT_LIBS
	debug ${SGCT_DEBUG_LIBRARY}
	optimized ${SGCT_RELEASE_LIBRARY}
)

find_package(OpenGL REQUIRED)
find_package(OpenSceneGraph REQUIRED osgUtil osgDB osgGA osgViewer)

include_directories(${SGCT_INCLUDE_DIRECTORY}
	${OPENSCENEGRAPH_INCLUDE_DIRS})

if( MSVC )
	set(LIBS
		${SGCT_LIBS}
		${OPENSCENEGRAPH_LIBRARIES}
		${OPENGL_LIBRARY}
		ws2_32
	)
elseif( WIN32 ) #MINGW or similar
	set(LIBS
		${SGCT_LIBS}
		${OPENSCENEGRAPH_LIBRARIES}
		${OPENGL_LIBRARY}
		ws2_32
	)
elseif( APPLE )
	find_library(COCOA_LIBRARY Cocoa REQUIRED)
	find_library(IOKIT_LIBRARY IOKit REQUIRED)
	find_library(COREVIDEO_LIBRARY CoreVideo REQUIRED)

	set(LIBS
		${SGCT_LIBS}
		${OPENSCENEGRAPH_LIBRARIES}
		${OPENGL_LIBRARY}
		${COCOA_LIBRARY}
		${IOKIT_LIBRARY}
		${COREVIDEO_LIBRARY}
	)
else() #linux
	find_library(X11_LIBRARY X11 REQUIRED)
	find_library(XRANDR_LIBRARY Xrandr REQUIRED)
	find_library(XI_LIBRARY Xi REQUIRED)
	find_library(XXF86VM_LIBRARY Xxf86vm)
	find_library(PTHREAD pthread REQUIRED)	

	set(LIBS
		${SGCT_LIBS}
		${OPENSCENEGRAPH_LIBRARIES}
		${OPENGL_LIBRARY}
		${X11_LIBRARY}
		${XRANDR_LIBRARY}
		${XI_LIBRARY}
		${XXF86VM_LIBRARY}
		${PTHREAD}
	)
endif()

if( MSVC )
	#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LTCG")
	
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
		set(CMAKE_CXX_FLAGS "-std=c++0x ${CMAKE_CXX_FLAGS}")
	endif()
elseif( APPLE  )
	add_definitions(-D__APPLE__)
	set(CMAKE_CXX_FLAGS "-std=c++11 -stdlib=libstdc++ ${CMAKE_CXX_FLAGS}")
else()
	add_definitions(-D__LINUX__)
	set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")
endif()
	
if( CMAKE_COMPILER_IS_GNUCXX )
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++ -static")
endif()

target_link_libraries(${APP_NAME} ${LIBS})
