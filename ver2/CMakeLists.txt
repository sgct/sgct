# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

#
# General project settings
#
cmake_minimum_required(VERSION 2.8)

SET(BUILD_SHARED_LIBS OFF)
SET(LIB_NAME sgct_light)

SET(CMAKE_DEBUG_POSTFIX "d" CACHE STRING "add a postfix, usually d on windows")
SET(CMAKE_RELEASE_POSTFIX "" CACHE STRING "add a postfix, usually empty on windows")

PROJECT(${LIB_NAME})
set(LibPath ${CMAKE_BINARY_DIR})
set(EXECUTABLE_OUTPUT_PATH ${LibPath})

mark_as_advanced(EXECUTABLE_OUTPUT_PATH LIBRARY_OUTPUT_PATH)

add_library(${LIB_NAME} STATIC
	src/sgct/utils/SGCTBox.cpp
	src/sgct/utils/SGCTDome.cpp
	src/sgct/utils/SGCTSphere.cpp
	src/sgct/ClusterManager.cpp
	src/sgct/CorrectionMesh.cpp
	src/sgct/Engine.cpp
	src/sgct/Font.cpp
	src/sgct/FontManager.cpp
	src/sgct/freetype.cpp
	src/sgct/Image.cpp
	src/sgct/MessageHandler.cpp
	src/sgct/NetworkManager.cpp
	src/sgct/OffScreenBuffer.cpp
	src/sgct/PostFX.cpp
	src/sgct/ReadConfig.cpp
	src/sgct/ScreenCapture.cpp
	src/sgct/SGCTMutexManager.cpp
	src/sgct/SGCTNetwork.cpp
	src/sgct/SGCTNode.cpp
	src/sgct/SGCTSettings.cpp
	src/sgct/SGCTTracker.cpp
	src/sgct/SGCTTrackingDevice.cpp
	src/sgct/SGCTTrackingManager.cpp
	src/sgct/SGCTUser.cpp
	src/sgct/SGCTVersion.cpp
	src/sgct/SGCTWindow.cpp
	src/sgct/Shader.cpp
	src/sgct/ShaderManager.cpp
	src/sgct/ShaderProgram.cpp
	src/sgct/SharedData.cpp
	src/sgct/SharedDataTypes.cpp
	src/sgct/Statistics.cpp
	src/sgct/TextureManager.cpp
	src/sgct/Viewport.cpp
	)
 
INCLUDE_DIRECTORIES(
  include
)

find_package(OpenGL REQUIRED)

find_library(GLEW_DEBUG_LIBRARY REQUIRED)
find_library(GLEW_RELEASE_LIBRARY REQUIRED)
find_library(GLFW3_DEBUG_LIBRARY REQUIRED)
find_library(GLFW3_RELEASE_LIBRARY REQUIRED)
find_library(GLFW3_DEBUG_LIBRARY REQUIRED)
find_library(GLFW3_RELEASE_LIBRARY REQUIRED)
find_library(FREETYPE2_DEBUG_LIBRARY REQUIRED)
find_library(FREETYPE2_RELEASE_LIBRARY REQUIRED)
find_library(VRPN_DEBUG_LIBRARY REQUIRED)
find_library(VRPN_RELEASE_LIBRARY REQUIRED)
find_library(PNG_DEBUG_LIBRARY REQUIRED)
find_library(PNG_RELEASE_LIBRARY REQUIRED)
find_library(Z_DEBUG_LIBRARY REQUIRED)
find_library(Z_RELEASE_LIBRARY REQUIRED)
find_library(TINYXML2_DEBUG_LIBRARY REQUIRED)
find_library(TINYXML2_RELEASE_LIBRARY REQUIRED)
find_library(TINYTHREAD_DEBUG_LIBRARY REQUIRED)
find_library(TINYTHREAD_RELEASE_LIBRARY REQUIRED)

set(COMMON_DEBUG_LIBS
	${GLEW_DEBUG_LIBRARY}
	${GLFW3_DEBUG_LIBRARY}
	${OPENGL_LIBRARY}
	${FREETYPE2_DEBUG_LIBRARY}
	${VRPN_DEBUG_LIBRARY}
	${PNG_DEBUG_LIBRARY}
	${Z_DEBUG_LIBRARY}
	${TINYXML2_DEBUG_LIBRARY}
	${TINYTHREAD_DEBUG_LIBRARY}
)

set(COMMON_RELEASE_LIBS
	${GLEW_RELEASE_LIBRARY}
	${GLFW3_RELEASE_LIBRARY}
	${OPENGL_LIBRARY}
	${FREETYPE2_RELEASE_LIBRARY}
	${VRPN_RELEASE_LIBRARY}
	${PNG_RELEASE_LIBRARY}
	${Z_RELEASE_LIBRARY}
	${TINYXML2_RELEASE_LIBRARY}
	${TINYTHREAD_RELEASE_LIBRARY}
)

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

target_link_libraries( ${LIB_NAME} debug ${DEBUG_LIBS})
target_link_libraries( ${LIB_NAME} optimized ${RELEASE_LIBS}) 

