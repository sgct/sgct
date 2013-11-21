# cmake project file by Prakash Punnoor
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

SET(CMAKE_DEBUG_POSTFIX "d" CACHE STRING "add a postfix, usually d on windows")
SET(CMAKE_RELEASE_POSTFIX "" CACHE STRING "add a postfix, usually empty on windows")

PROJECT(Alut C)
SET(PACKAGE "freealut")
SET(PACKAGE_TARNAME "freealut")
SET(PACKAGE_NAME "freealut library")
SET(PACKAGE_MAJOR_VERSION "1")
SET(PACKAGE_MINOR_VERSION "0")
SET(PACKAGE_BUILD_VERSION "1")
SET(PACKAGE_VERSION "${PACKAGE_MAJOR_VERSION}.${PACKAGE_MINOR_VERSION}.${PACKAGE_BUILD_VERSION}")
SET(MAJOR_VERSION "0")
SET(MINOR_VERSION "0")
SET(BUILD_VERSION "0")
SET(VERSION "${MAJOR_VERSION}.${MINOR_VERSION}.${BUILD_VERSION}")
SET(PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")
SET(PACKAGE_BUGREPORT "openal-devel@opensource.creative.com")
SET(OPERATING_SYSTEM "${CMAKE_SYSTEM_NAME}")

SET(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/admin/CMakeModules")
MESSAGE(STATUS "Alut CMake module path: ${CMAKE_MODULE_PATH}")

if( APPLE  )
	option(ALUT_CPP11 "Use c++11" OFF)
	set(CMAKE_OSX_ARCHITECTURES "x86_64")
	if(CMAKE_GENERATOR STREQUAL Xcode)
		set(CMAKE_OSX_DEPLOYMENT_TARGET "10.8")
	endif()
endif()

if( DEFINED SGCT_PLACE_TARGETS_IN_SOURCE_TREE AND NOT SGCT_PLACE_TARGETS_IN_SOURCE_TREE )
	set(ALUT_TARGET_PATH ${PROJECT_BINARY_DIR}/additional_libs/ALUT)
else()
	set(ALUT_TARGET_PATH ${PROJECT_SOURCE_DIR}/../../additional_libs/ALUT)
endif()
		
if( APPLE )
	set(ALUT_OUTPUT_PATH "${ALUT_TARGET_PATH}/mac")
elseif( MINGW )
	set(ALUT_OUTPUT_PATH "${ALUT_TARGET_PATH}/mingw")
elseif( MSVC )
	if( CMAKE_GENERATOR STREQUAL "Visual Studio 9 2008" )
		set(ALUT_OUTPUT_PATH "${ALUT_TARGET_PATH}/msvc9")
	elseif( CMAKE_GENERATOR STREQUAL "Visual Studio 9 2008 Win64" )
		set(ALUT_OUTPUT_PATH "${ALUT_TARGET_PATH}/msvc9_x64")
	elseif( CMAKE_GENERATOR STREQUAL "Visual Studio 10" )
		set(ALUT_OUTPUT_PATH "${ALUT_TARGET_PATH}/msvc10")
	elseif( CMAKE_GENERATOR STREQUAL "Visual Studio 10 Win64" )
		set(ALUT_OUTPUT_PATH "${ALUT_TARGET_PATH}/msvc10_x64")
	elseif( CMAKE_GENERATOR STREQUAL "Visual Studio 11" )
		set(ALUT_OUTPUT_PATH "${ALUT_TARGET_PATH}/msvc11")
	elseif( CMAKE_GENERATOR STREQUAL "Visual Studio 11 Win64" )
		set(ALUT_OUTPUT_PATH "${ALUT_TARGET_PATH}/msvc11_x64")
	elseif( CMAKE_GENERATOR STREQUAL "Visual Studio 12" )
		set(ALUT_OUTPUT_PATH "${ALUT_TARGET_PATH}/msvc12")
	elseif( CMAKE_GENERATOR STREQUAL "Visual Studio 12 Win64" )
		set(ALUT_OUTPUT_PATH "${ALUT_TARGET_PATH}/msvc12_x64")
	else()
		set(ALUT_OUTPUT_PATH "${ALUT_TARGET_PATH}/${CMAKE_GENERATOR}")
	endif()
elseif( UNIX AND NOT APPLE )
	if( CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(ALUT_OUTPUT_PATH "${ALUT_TARGET_PATH}/linux_x64")
	elseif( CMAKE_SYSTEM_NAME STREQUAL "Linux")
		set(ALUT_OUTPUT_PATH "${ALUT_TARGET_PATH}/linux")
	else()
		set(ALUT_OUTPUT_PATH "${ALUT_TARGET_PATH}/unix")
	endif()
else()
	set(ALUT_OUTPUT_PATH "${ALUT_TARGET_PATH}/${CMAKE_SYSTEM_NAME}/${CMAKE_GENERATOR}")
endif()

set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${ALUT_OUTPUT_PATH}" )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${ALUT_OUTPUT_PATH}" )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${ALUT_OUTPUT_PATH}" )

INCLUDE(${CMAKE_ROOT}/Modules/CheckIncludeFile.cmake)
INCLUDE(${CMAKE_ROOT}/Modules/CheckIncludeFiles.cmake)
INCLUDE(${CMAKE_ROOT}/Modules/CheckCSourceCompiles.cmake)

find_package(ConfigHelper)
find_package(OpenAL REQUIRED)

SET(ALUT_SOURCES src/alutBufferData.c
                 src/alutCodec.c
                 src/alutError.c
                 src/alutInit.c
                 src/alutInputStream.c
                 src/alutInternal.h
                 src/alutLoader.c
                 src/alutOutputStream.c
                 src/alutUtil.c
                 src/alutVersion.c
                 src/alutWaveform.c)

set(ALUT_INC ${OPENAL_INCLUDE_DIR} ${Alut_SOURCE_DIR}/include)
include_directories(${ALUT_INC})
#MESSAGE(STATUS "Include dirs: ${ALUT_INC}")

SET(ADD_WFLAGS "")
SET(ADD_CFLAGS "")
SET(ADD_LDFLAGS "")
SET(ADD_LIBS "")

#ADD_DEFINE(ALUT_BUILD_LIBRARY)
# We could possibly need struct timespec and random(), which are not ANSI.
# Define to 500 if Single Unix conformance is wanted, 600 for sixth revision.
ADD_DEFINE("_XOPEN_SOURCE 500")
# We might need nanosleep, which is a POSIX IEEE Std 1003.1b-1993 feature.
# Define to the POSIX version that should be used.
ADD_DEFINE("_POSIX_C_SOURCE 199309")
# Without __NO_CTYPE tolower and friends are macros which introduce a GLIBC 2.3
# dependency. By defining this identifier we are currently backwards compatible
# to GLIBC 2.1.3, which is a good thing. In addition, the macros lead to code
# which triggers warnings with -Wunreachable-code.
ADD_DEFINE("__NO_CTYPE 1")

ADD_DEFINITIONS(-DALUT_STATIC)
ADD_DEFINITIONS(-DHAVE_CONFIG_H)
ADD_DEFINITIONS(-DNDEBUG)

IF(UNIX)
 SET(ADD_LIBS ${ADD_LIBS} m)
ENDIF(UNIX)

FIND_PACKAGE(SleepFunction)

CHECK_INCLUDE_FILE_DEFINE(stdint.h HAVE_STDINT_H)

IF(WIN32)
  CHECK_INCLUDE_FILE_DEFINE(basetsd.h HAVE_BASETSD_H)
ENDIF(WIN32)

# FindSleepFunction will check for unistd.h if time.h wasn't found
IF(HAVE_TIME_H)
  CHECK_INCLUDE_FILE_DEFINE(unistd.h HAVE_UNISTD_H)
ENDIF(HAVE_TIME_H)

IF(HAVE_UNISTD_H)
  CHECK_FUNCTION_DEFINE("
  #include <sys/types.h>
  #include <sys/stat.h>
  #include <unistd.h>
  " stat "(\"\", (struct stat*)0)" HAVE_STAT)
ENDIF(HAVE_UNISTD_H)

IF(NOT HAVE_STAT)
  CHECK_FUNCTION_DEFINE("
  #include <sys/types.h>
  #include <sys/stat.h>
  " _stat "(\"\", (struct _stat*)0)" HAVE__STAT)
ENDIF(NOT HAVE_STAT)

# compiler specific settings
#FIND_PACKAGE(CompilerAttribute)
#FIND_PACKAGE(CompilerVisibility)
#FIND_PACKAGE(CompilerFlagsSet)

GENERATE_CONFIG_H()

if( MSVC )
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LTCG")
	#disable pdb-file debugging
	#STRING(REPLACE "/Zi" "/Z7" replacementFlags ${CMAKE_CXX_FLAGS_DEBUG})
	#set(CMAKE_CXX_FLAGS_DEBUG "${replacementFlags}")
endif()

if(MSVC AND NOT "${MSVC_VERSION}" LESS 1400)
	add_definitions( "/MP" )
endif()
	
if( APPLE )
	if( ALUT_CPP11 )
   		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -stdlib=libc++")
		MESSAGE(STATUS "ALUT will be using c++11")
	elseif(DEFINED SGCT_CPP11 AND SGCT_CPP11)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -stdlib=libc++")
		MESSAGE(STATUS "ALUT will be using c++11")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -stdlib=libstdc++")
		MESSAGE(STATUS "ALUT will be using c++0x")
	endif()
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
endif()

if( CMAKE_COMPILER_IS_GNUCXX )
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++ -s")
endif()

SET(CMAKE_C_FLAGS "${ADD_WFLAGS} ${ADD_CFLAGS} ${CMAKE_C_FLAGS}")
SET(CMAKE_CXX_FLAGS "${ADD_CFLAGS} ${CMAKE_CXX_FLAGS}")
SET(CMAKE_SHARED_LINKER_FLAGS "${ADD_LDFLAGS} ${CMAKE_SHARED_LINKER_FLAGS}")
SET(CMAKE_MODULE_LINKER_FLAGS "${ADD_LDFLAGS} ${CMAKE_MODULE_LINKER_FLAGS}")

set(ALUT_LIB_NAME "alut")
if(APPLE AND ALUT_CPP11)
	set(ALUT_LIB_NAME "${ALUT_LIB_NAME}_cpp11")
elseif(DEFINED SGCT_CPP11 AND SGCT_CPP11)
	set(ALUT_LIB_NAME "${ALUT_LIB_NAME}_cpp11")
endif()
	
	
ADD_LIBRARY(${ALUT_LIB_NAME} STATIC ${ALUT_SOURCES})
TARGET_LINK_LIBRARIES(${ALUT_LIB_NAME} ${OPENAL_LIBRARY} ${ADD_LIBS})

INSTALL(TARGETS ${ALUT_LIB_NAME} ARCHIVE DESTINATION lib)
INSTALL(FILES include/AL/alut.h DESTINATION include/AL)