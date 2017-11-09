# Copyright Linkoping University 2011-2015
# SGCT Project
#
# General project settings
#
cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type")
endif()

SET(BUILD_SHARED_LIBS OFF)
PROJECT("sgct")
set(SGCT_VERSION_MAJOR "2")
set(SGCT_VERSION_MINOR "8")
set(SGCT_VERSION_REVISION "0")
set(SGCT_VERSION "${SGCT_VERSION_MAJOR}_${SGCT_VERSION_MINOR}_${SGCT_VERSION_REVISION}")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

STRING(REGEX REPLACE "/" "\\\\" SGCT_WIN_SOURCE_DIR ${PROJECT_SOURCE_DIR})
STRING(REGEX REPLACE "/" "\\\\" SGCT_WIN_BINARY_DIR ${PROJECT_BINARY_DIR}) 

SET(CMAKE_DEBUG_POSTFIX "d" CACHE STRING "add a postfix, usually d on windows")
SET(CMAKE_RELEASE_POSTFIX "" CACHE STRING "add a postfix, usually empty on windows")

option(SGCT_INSTALL "Install SGCT" OFF)
option(SGCT_EXAMPLES "Build SGCT examples" OFF)
option(SGCT_TEXT "Build SGCT with Freetype2" ON)
option(SGCT_DOXYGEN "Build doxygen documentation" OFF)
option(SGCT_DOXYGEN_QUIET "Suppress warnings" ON)
option(SGCT_CUSTOMOUTPUTDIRS "Use custom output directories" ON)


#if(NOT WIN32)
#	option(SGCT_MAKE_RELEASE "make realease" OFF)
#endif()

if (MSVC)
	option(SGCT_USE_MSVC_RUNTIMES "To use MSVC DLLs or to create a static build" ON)
	option(SGCT_BUILD_CSHARP_PROJECTS "Build C# projects" ON)
endif()
option(SGCT_BUILD_ALUT "Build ALUT" OFF)
option(SGCT_LIGHT_ONLY "Don't merge external libs" OFF)
option(SGCT_NO_EXTERNAL_LIBRARIES "Do not include external libraries" OFF)
if( MINGW  )
	option(SGCT_MINGW64 "Use MinGW64" ON)
endif()

macro (set_xcode_property TARGET XCODE_PROPERTY XCODE_VALUE)
set_property (TARGET ${TARGET} PROPERTY XCODE_ATTRIBUTE_${XCODE_PROPERTY}
	    ${XCODE_VALUE})
endmacro (set_xcode_property)

#png & zlib options
set(SKIP_INSTALL_ALL ON)

if( APPLE )
	option(SGCT_CPP11 "Use libc++ instead of libstdc++" ON)
	set(CMAKE_OSX_ARCHITECTURES "x86_64")
	if(CMAKE_GENERATOR STREQUAL Xcode)
		set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9")
	endif()
endif()

set(SGCT_TARGET_PATH ${PROJECT_BINARY_DIR}) 
set(SGCT_BINARY_PATH ${PROJECT_BINARY_DIR})
set(SGCT_SOURCE_PATH ${PROJECT_SOURCE_DIR})
MESSAGE(STATUS "SGCT target path: ${SGCT_TARGET_PATH}")

if( APPLE )
	if( SGCT_CPP11 )
		set(SGCT_LIB_PATH "${SGCT_TARGET_PATH}/lib/mac_cpp11")
		set(SGCT_LIB_SRC_PATH "${PROJECT_SOURCE_DIR}/lib/mac_cpp11")
	else()
		set(SGCT_LIB_PATH "${SGCT_TARGET_PATH}/lib/mac")
		set(SGCT_LIB_SRC_PATH "${PROJECT_SOURCE_DIR}/lib/mac")
	endif()
elseif( MINGW )
	set(SGCT_COMPILER "mingw")
	if( SGCT_MINGW64 )
		set(SGCT_ARCHITECTURE "x64")
		set(SGCT_LIB_PATH "${SGCT_TARGET_PATH}/lib/${SGCT_COMPILER}_${SGCT_ARCHITECTURE}")
		set(SGCT_LIB_SRC_PATH "${PROJECT_SOURCE_DIR}/lib/${SGCT_COMPILER}_${SGCT_ARCHITECTURE}")
	else()
		set(SGCT_ARCHITECTURE "x86")
		set(SGCT_LIB_PATH "${SGCT_TARGET_PATH}/lib/${SGCT_COMPILER}")
		set(SGCT_LIB_SRC_PATH "${PROJECT_SOURCE_DIR}/lib/${SGCT_COMPILER}")
	endif()
elseif( MSVC )
	if( CMAKE_GENERATOR STREQUAL "Visual Studio 9 2008" OR CMAKE_GENERATOR STREQUAL "Visual Studio 9 2008" )
		set(SGCT_COMPILER "msvc9")
		set(SGCT_ARCHITECTURE "x86")
	elseif( CMAKE_GENERATOR STREQUAL "Visual Studio 9 2008 Win64" OR CMAKE_GENERATOR STREQUAL "Visual Studio 9 2008 Win64" )
		set(SGCT_COMPILER "msvc9")
		set(SGCT_ARCHITECTURE "x64")
	elseif( CMAKE_GENERATOR STREQUAL "Visual Studio 10" OR CMAKE_GENERATOR STREQUAL "Visual Studio 10 2010" )
		set(SGCT_COMPILER "msvc10")
		set(SGCT_ARCHITECTURE "x86")
	elseif( CMAKE_GENERATOR STREQUAL "Visual Studio 10 Win64" OR CMAKE_GENERATOR STREQUAL "Visual Studio 10 2010 Win64" )
		set(SGCT_COMPILER "msvc10")
		set(SGCT_ARCHITECTURE "x64")
	elseif( CMAKE_GENERATOR STREQUAL "Visual Studio 11" OR CMAKE_GENERATOR STREQUAL "Visual Studio 11 2012" )
		set(SGCT_COMPILER "msvc11")
		set(SGCT_ARCHITECTURE "x86")
	elseif( CMAKE_GENERATOR STREQUAL "Visual Studio 11 Win64" OR CMAKE_GENERATOR STREQUAL "Visual Studio 11 2012 Win64" )
		set(SGCT_COMPILER "msvc11")
		set(SGCT_ARCHITECTURE "x64")
	elseif( CMAKE_GENERATOR STREQUAL "Visual Studio 12" OR CMAKE_GENERATOR STREQUAL "Visual Studio 12 2013")
		set(SGCT_COMPILER "msvc12")
		set(SGCT_ARCHITECTURE "x86")
	elseif( CMAKE_GENERATOR STREQUAL "Visual Studio 12 Win64" OR CMAKE_GENERATOR STREQUAL "Visual Studio 12 2013 Win64")
		set(SGCT_COMPILER "msvc12")
		set(SGCT_ARCHITECTURE "x64")
	elseif( CMAKE_GENERATOR STREQUAL "Visual Studio 14" OR CMAKE_GENERATOR STREQUAL "Visual Studio 14 2015")
		set(SGCT_COMPILER "msvc14")
		set(SGCT_ARCHITECTURE "x86")
	elseif( CMAKE_GENERATOR STREQUAL "Visual Studio 14 Win64" OR CMAKE_GENERATOR STREQUAL "Visual Studio 14 2015 Win64")
		set(SGCT_COMPILER "msvc14")
		set(SGCT_ARCHITECTURE "x64")
	elseif( CMAKE_GENERATOR STREQUAL "Visual Studio 15" OR CMAKE_GENERATOR STREQUAL "Visual Studio 15 2017")
		set(SGCT_COMPILER "msvc15")
		set(SGCT_ARCHITECTURE "x86")
	elseif( CMAKE_GENERATOR STREQUAL "Visual Studio 15 Win64" OR CMAKE_GENERATOR STREQUAL "Visual Studio 15 2017 Win64")
		set(SGCT_COMPILER "msvc15")
		set(SGCT_ARCHITECTURE "x64")
	else()
		#use mingw as template
		set(SGCT_COMPILER "mingw")
		set(SGCT_ARCHITECTURE "x86")
	endif()
	
	if(SGCT_ARCHITECTURE STREQUAL "x64")
		set(SGCT_LIB_PATH "${SGCT_TARGET_PATH}/lib/${SGCT_COMPILER}_${SGCT_ARCHITECTURE}")
		set(SGCT_LIB_SRC_PATH "${PROJECT_SOURCE_DIR}/lib/${SGCT_COMPILER}_${SGCT_ARCHITECTURE}")
	else()
		set(SGCT_LIB_PATH "${SGCT_TARGET_PATH}/lib/${SGCT_COMPILER}")
		set(SGCT_LIB_SRC_PATH "${PROJECT_SOURCE_DIR}/lib/${SGCT_COMPILER}")
	endif()
elseif( UNIX AND NOT APPLE )
	if( CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(SGCT_LIB_PATH "${SGCT_TARGET_PATH}/lib/linux_x64")
		set(SGCT_LIB_SRC_PATH "${PROJECT_SOURCE_DIR}/lib/linux_x64")
	elseif( CMAKE_SYSTEM_NAME STREQUAL "Linux")
		set(SGCT_LIB_PATH "${SGCT_TARGET_PATH}/lib/linux")
		set(SGCT_LIB_SRC_PATH "${PROJECT_SOURCE_DIR}/lib/linux")
	else()
		set(SGCT_LIB_PATH "${SGCT_TARGET_PATH}/lib/unix")
		#use linux as template
		set(SGCT_LIB_SRC_PATH "${PROJECT_SOURCE_DIR}/lib/linux")
	endif()
else()
	set(SGCT_LIB_PATH "${SGCT_TARGET_PATH}/lib/${CMAKE_SYSTEM_NAME}/${CMAKE_GENERATOR}")
	#use linux as template
	set(SGCT_LIB_SRC_PATH "${PROJECT_SOURCE_DIR}/lib/linux")
endif()
MESSAGE(STATUS "SGCT lib path: ${SGCT_LIB_PATH}")

SET(LIB_NAME sgct_light)

set( PDB_OUTPUT_DIRECTORY "${SGCT_LIB_PATH}" )
if(SGCT_CUSTOMOUTPUTDIRS)
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${SGCT_LIB_PATH}")
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${SGCT_LIB_PATH}")
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${SGCT_LIB_PATH}")
endif()
set( SGCT_RELEASE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/release" )

if(SGCT_DOXYGEN_QUIET)
	set(SGCT_DOXYGEN_QUIET_STATE "YES")
else()
	set(SGCT_DOXYGEN_QUIET_STATE "NO")
endif()

if(SGCT_TEXT)
	set(USE_SGCT_TEXT "1")
else()
	set(USE_SGCT_TEXT "0")
endif()

#configure settings
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/SGCTConfig.h.in ${CMAKE_CURRENT_SOURCE_DIR}/include/sgct/SGCTConfig.h @ONLY)

#update version in files
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/SGCTVersion.h.in ${CMAKE_CURRENT_SOURCE_DIR}/include/sgct/SGCTVersion.h @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/release/build_release_xcode.sh.in ${CMAKE_CURRENT_BINARY_DIR}/src/installer/build_release_xcode.sh @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/release/build_release_xcode_cpp11.sh.in ${CMAKE_CURRENT_BINARY_DIR}/src/installer/build_release_xcode_cpp11.sh @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/release/build_release_linux.sh.in ${CMAKE_CURRENT_BINARY_DIR}/src/installer/build_release_linux.sh @ONLY)

#setup merge static lib scripts
if(NOT SGCT_LIGHT_ONLY)
	if(MSVC)
		if(SGCT_TEXT)
			set(SGCT_FREETYPE_RELEASE_LIB_PATH "deps\\freetype.lib")
			set(SGCT_FREETYPE_DEBUG_LIB_PATH "deps\\freetyped.lib")
		
		endif()
		configure_file(${SGCT_LIB_SRC_PATH}/merge_libs.bat.in ${SGCT_LIB_PATH}/merge_libs.bat @ONLY)
	else()
		if(CMAKE_GENERATOR STREQUAL Xcode)
			set(SGCT_SHELL_NAME merge_libs_xcode.sh)
			if(SGCT_TEXT)
				set(SGCT_FREETYPE_RELEASE_LIB_PATH "deps/libfreetype.a")
				set(SGCT_FREETYPE_DEBUG_LIB_PATH "deps/libfreetyped.a")
			endif()
		else()
			set(SGCT_SHELL_NAME merge_libs.sh)
			if(SGCT_TEXT)
				set(SGCT_FREETYPE_RELEASE_LIB_PATH "ar x deps/libfreetype.a")
				set(SGCT_FREETYPE_DEBUG_LIB_PATH "ar x deps/libfreetyped.a")
			else()
				set(SGCT_FREETYPE_RELEASE_LIB_PATH "#") #add empty comment
				set(SGCT_FREETYPE_DEBUG_LIB_PATH "#") #add empty comment
			endif()
		endif()
		
		configure_file(${SGCT_LIB_SRC_PATH}/${SGCT_SHELL_NAME}.in ${SGCT_LIB_PATH}/tmp/${SGCT_SHELL_NAME} @ONLY)
        file(COPY ${SGCT_LIB_PATH}/tmp/${SGCT_SHELL_NAME}
            DESTINATION ${SGCT_LIB_PATH}
            FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ
            GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
        file(REMOVE_RECURSE ${SGCT_LIB_PATH}/tmp)
	endif()
endif()

#crate directories
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin/installer)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/docs)

set(LibPath "${SGCT_LIB_PATH}")
set(EXECUTABLE_OUTPUT_PATH ${LibPath})

file (GLOB_RECURSE sgctSRC "${CMAKE_CURRENT_SOURCE_DIR}/src/sgct/*.cpp")
file (GLOB_RECURSE sgctInc "${CMAKE_CURRENT_SOURCE_DIR}/include/sgct/*.h")

#include_directories("include/external/freetype")
#Set(ALL_SRC ${sgctSRC} ${sgctInc} "${CMAKE_CURRENT_SOURCE_DIR}/include/sgct.h" "include/external/freetype")
Set(ALL_SRC ${sgctSRC} ${sgctInc} "${CMAKE_CURRENT_SOURCE_DIR}/include/sgct.h")

#print all
FOREACH(ALL_SRCName ${ALL_SRC})
	MESSAGE(STATUS "Adding file: ${ALL_SRCName}")
ENDFOREACH(ALL_SRCName)

add_library(${LIB_NAME} STATIC
	${ALL_SRC}
	)

if(SGCT_NO_EXTERNAL_LIBRARIES)	
	INCLUDE_DIRECTORIES(
	  include
	)
else(SGCT_NO_EXTERNAL_LIBRARIES)
	INCLUDE_DIRECTORIES(
	  include
	  include/external
	)
	add_definitions(-DSGCT_DONT_USE_EXTERNAL)
endif(SGCT_NO_EXTERNAL_LIBRARIES)

########################
# Spout section  start #
########################
if (WIN32)
    option(SGCT_SPOUT_SUPPORT "SGCT Spout support" OFF)
endif ()

set(SPOUT_DEFINITIONS "")
if (SGCT_SPOUT_SUPPORT)
    set(SPOUT_DEFINITIONS "-DSGCT_HAS_SPOUT")
endif ()
add_definitions(${SPOUT_DEFINITIONS})

########################
#  Spout section  end  #
########################

find_package(OpenGL REQUIRED)

set( PDB_OUTPUT_DIRECTORY "${SGCT_LIB_PATH}/deps" )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${SGCT_LIB_PATH}/deps" )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${SGCT_LIB_PATH}/deps" )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${SGCT_LIB_PATH}/deps" )
set( CMAKE_EXECUTABLE_OUTPUT_DIRECTORY "${PROJECT_BUILD_DIR}/bin" )
set( CMAKE_EXECUTABLE_OUTPUT_DIRECTORY_DEBUG "${PROJECT_BUILD_DIR}/bin" )
set( CMAKE_EXECUTABLE_OUTPUT_DIRECTORY_RELEASE "${PROJECT_BUILD_DIR}/bin" )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BUILD_DIR}/bin" )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${PROJECT_BUILD_DIR}/bin" )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${PROJECT_BUILD_DIR}/bin" )

if(MSVC)
	set(SGCTCompilerFlags
        CMAKE_CXX_FLAGS
        CMAKE_CXX_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_RELEASE
		CMAKE_CXX_FLAGS_RELWITHDEBINFO
        CMAKE_C_FLAGS
        CMAKE_C_FLAGS_DEBUG
        CMAKE_C_FLAGS_RELEASE
		CMAKE_C_FLAGS_RELWITHDEBINFO
        )
endif(MSVC)

if(NOT SGCT_NO_EXTERNAL_LIBRARIES)

	ADD_SUBDIRECTORY(src/deps)

	if(SGCT_BUILD_ALUT)
		ADD_SUBDIRECTORY(additional_deps/freealut-1.1.0-src)
	endif(SGCT_BUILD_ALUT)
endif()
	
set(COMMON_DEBUG_LIBS
	${OPENGL_gl_LIBRARY}
)

set(COMMON_RELEASE_LIBS
	${OPENGL_gl_LIBRARY}
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
	find_library(COREVIDEO_LIBRARY CoreVideo REQUIRED)

	set(DEBUG_LIBS
		${COMMON_DEBUG_LIBS}
		${COCOA_LIBRARY}
		${IOKIT_LIBRARY}
		${COREVIDEO_LIBRARY}
	)
	
	set(RELEASE_LIBS
		${COMMON_RELEASE_LIBS}
		${COCOA_LIBRARY}
		${IOKIT_LIBRARY}
		${COREVIDEO_LIBRARY}
	)
else() # Linux or FreeBSD
	if (CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
		include(FindPkgConfig)
		pkg_check_modules(X11 REQUIRED x11)
		include_directories(${X11_X11_INCLUDE_PATH})
	else ()
		find_package(X11 REQUIRED)
	endif ()
	find_package(Threads REQUIRED)

	set(LINUX_LIBS
		${X11_X11_LIB}
		${X11_Xrandr_LIB}
		${X11_Xinerama_LIB}
		${X11_Xinput_LIB}
		${X11_Xxf86vm_LIB}
		${X11_Xcursor_LIB}
		${CMAKE_THREAD_LIBS_INIT}		
	)

	set(DEBUG_LIBS
		${COMMON_DEBUG_LIBS}
		${LINUX_LIBS}
	)
	
	set(RELEASE_LIBS
		${COMMON_RELEASE_LIBS}
		${LINUX_LIBS}
	)
endif()

if( MSVC )
	#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LTCG")
	
	if (NOT SGCT_USE_MSVC_RUNTIMES)
        foreach (flag ${SGCTCompilerFlags})

            if (${flag} MATCHES "/MD")
                string(REGEX REPLACE "/MD" "/MT" ${flag} "${${flag}}")
            endif()
            if (${flag} MATCHES "/MDd")
                string(REGEX REPLACE "/MDd" "/MTd" ${flag} "${${flag}}")
            endif()

        endforeach()
    endif()
	
	foreach(CompilerFlag ${SGCTCompilerFlags})
		string(REPLACE "/Zi" "/Z7" ${CompilerFlag} "${${CompilerFlag}}")
	endforeach()
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

add_definitions(-DGLEW_STATIC)
add_definitions(-DGLEW_NO_GLU)
	
#if( CMAKE_COMPILER_IS_GNUCXX )
#	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++ -static")
#endif()

target_link_libraries( ${LIB_NAME} debug ${DEBUG_LIBS} optimized ${RELEASE_LIBS})

#to make sure that everything is build when post_build events runs

set(SGCT_DEPS glew glfw png16_static turbojpeg-static tinyxml2static zlibstatic miniziplibstatic vrpn)
#prepend freetype
if(SGCT_TEXT)
	set(SGCT_DEPS freetype ${SGCT_DEPS})
endif()
add_dependencies(${LIB_NAME} ${SGCT_DEPS})

#including deps
include_directories(BEFORE ${PROJECT_SOURCE_DIR}/src/deps/glfw/include)

#including directories with generated config files in this scope
include_directories(BEFORE ${PROJECT_BINARY_DIR}/src/deps/vrpn)
include_directories(BEFORE ${PROJECT_BINARY_DIR}/src/deps/lpng)
include_directories(BEFORE ${PROJECT_BINARY_DIR}/src/deps/lpng/zlib)
include_directories(BEFORE ${PROJECT_BINARY_DIR}/src/deps/libjpeg-turbo)

#merge libs
if(NOT SGCT_LIGHT_ONLY)
	if(MSVC)
		add_custom_command(TARGET ${LIB_NAME}
			POST_BUILD
			COMMAND "merge_libs.bat"
			WORKING_DIRECTORY ${SGCT_LIB_PATH}
			)
	else()
		add_custom_command(TARGET ${LIB_NAME}
			POST_BUILD
			COMMAND "./${SGCT_SHELL_NAME}"
			WORKING_DIRECTORY ${SGCT_LIB_PATH})
	endif()
endif()
		
#if(SGCT_MAKE_RELEASE)
#	if( APPLE )
#		if( SGCT_CPP11 )
#			add_custom_command(TARGET ${LIB_NAME}
#				POST_BUILD
#				COMMAND "${PROJECT_BINARY_DIR}/src/installer/build_release_xcode_cpp11.sh"
#				WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/src/installer)
#		else()
#			add_custom_command(TARGET ${LIB_NAME}
#				POST_BUILD
#				COMMAND "${PROJECT_BINARY_DIR}/src/installer/build_release_xcode.sh"
#				WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/src/installer)
#		endif()
#	elseif( NOT WIN32 ) #linux
#		add_custom_command(TARGET ${LIB_NAME}
#			POST_BUILD
#			COMMAND "${PROJECT_BINARY_DIR}/src/installer/build_release_linux.sh"
#			WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/src/installer)
#	endif()
#endif()

SET(SPOUT_ENABLED "0")
if(SGCT_EXAMPLES)
	if( APPLE AND SGCT_CPP11 )
		set(SGCT_RELEASE_LIBRARY "${SGCT_LIB_PATH}/libsgct_cpp11.a")
		set(SGCT_DEBUG_LIBRARY "${SGCT_LIB_PATH}/libsgct_cpp11d.a")
	elseif( MSVC )
		set(SGCT_RELEASE_LIBRARY "${SGCT_LIB_PATH}/sgct.lib")
		set(SGCT_DEBUG_LIBRARY "${SGCT_LIB_PATH}/sgctd.lib")
	else() #mac, mingw and linux
		set(SGCT_RELEASE_LIBRARY "${SGCT_LIB_PATH}/libsgct.a")
		set(SGCT_DEBUG_LIBRARY "${SGCT_LIB_PATH}/libsgctd.a")
	endif()
	set(SGCT_INCLUDE_DIRECTORY "${SGCT_SOURCE_PATH}/include")

	file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin/examples)
	file(COPY "${SGCT_SOURCE_PATH}/src/apps/SharedResources" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/bin/examples")
	file(COPY "${SGCT_SOURCE_PATH}/config" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/bin")
	
	set(SGCT_EXAMPLE_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/bin/examples)
	#message(STATUS "Example out directory=${SGCT_EXAMPLE_OUTPUT_DIR}")
	ADD_SUBDIRECTORY(src/apps)
endif()

#must be placed after examples subdirectory
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/installers/sgct_osg_all_win.nsi.in ${CMAKE_CURRENT_BINARY_DIR}/src/installer/sgct_osg_all_win.nsi @ONLY)
	
if(SGCT_DOXYGEN)
	find_package(Doxygen)
	if (DOXYGEN_FOUND)
		add_custom_command(TARGET ${LIB_NAME}
			POST_BUILD
			COMMAND ${DOXYGEN_EXECUTABLE} ${PROJECT_SOURCE_DIR}/Doxyfile
			WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
	endif()
endif()

if(SGCT_INSTALL)
	if(SGCT_LIGHT_ONLY)
		if( MSVC )
			INSTALL(FILES ${SGCT_LIB_PATH}/sgct_light.lib DESTINATION lib COMPONENT libraries CONFIGURATIONS Release)
			INSTALL(FILES ${SGCT_LIB_PATH}/sgct_lightd.lib DESTINATION lib COMPONENT libraries CONFIGURATIONS Debug)
		else( MSVC )
			INSTALL(FILES ${SGCT_LIB_PATH}/libsgct_light.a DESTINATION lib COMPONENT libraries CONFIGURATIONS Release)
			INSTALL(FILES ${SGCT_LIB_PATH}/libsgct_lightd.a DESTINATION lib COMPONENT libraries CONFIGURATIONS Debug)
		endif( MSVC )
	else(SGCT_LIGHT_ONLY)
		if( MSVC )
			INSTALL(FILES ${SGCT_LIB_PATH}/sgct.lib DESTINATION lib COMPONENT libraries CONFIGURATIONS Release)
			INSTALL(FILES ${SGCT_LIB_PATH}/sgctd.lib DESTINATION lib COMPONENT libraries CONFIGURATIONS Debug)
		else( MSVC )
			if( APPLE AND SGCT_CPP11 )
				INSTALL(FILES ${SGCT_LIB_PATH}/libsgct_cpp11.a DESTINATION lib COMPONENT libraries CONFIGURATIONS Release)
				INSTALL(FILES ${SGCT_LIB_PATH}/libsgct_cpp11d.a DESTINATION lib COMPONENT libraries CONFIGURATIONS Debug)
			else()
				INSTALL(FILES ${SGCT_LIB_PATH}/libsgct.a DESTINATION lib COMPONENT libraries CONFIGURATIONS Release)
				INSTALL(FILES ${SGCT_LIB_PATH}/libsgctd.a DESTINATION lib COMPONENT libraries CONFIGURATIONS Debug)
			endif()
		endif( MSVC )
		#include files
		INSTALL(DIRECTORY ${PROJECT_SOURCE_DIR}/include/external DESTINATION include COMPONENT headers PATTERN ".svn" EXCLUDE)
		INSTALL(DIRECTORY ${PROJECT_SOURCE_DIR}/include/GL DESTINATION include COMPONENT headers PATTERN ".svn" EXCLUDE)
		INSTALL(DIRECTORY ${PROJECT_SOURCE_DIR}/include/glm DESTINATION include COMPONENT headers PATTERN ".svn" EXCLUDE)
		INSTALL(DIRECTORY ${PROJECT_SOURCE_DIR}/include/vrpn DESTINATION include COMPONENT headers PATTERN ".svn" EXCLUDE)
		INSTALL(DIRECTORY ${PROJECT_SOURCE_DIR}/src/deps/glfw/include/GLFW DESTINATION include COMPONENT headers PATTERN ".svn" EXCLUDE)
	endif()
		
	INSTALL(FILES ${PROJECT_SOURCE_DIR}/include/sgct.h DESTINATION include COMPONENT headers)
	INSTALL(DIRECTORY ${PROJECT_SOURCE_DIR}/include/sgct DESTINATION include COMPONENT headers PATTERN ".svn" EXCLUDE)
	
	#make install on other systems should not copy docs into the compiler's directory
	if(MSVC)
		#docs
		INSTALL(FILES ${PROJECT_SOURCE_DIR}/EULA.rtf DESTINATION docs COMPONENT documentation)
		INSTALL(DIRECTORY ${PROJECT_SOURCE_DIR}/readme DESTINATION docs COMPONENT documentation PATTERN ".svn" EXCLUDE)
		if (DOXYGEN_FOUND)
			INSTALL(DIRECTORY ${PROJECT_SOURCE_DIR}/docs/html DESTINATION docs COMPONENT documentation PATTERN ".svn" EXCLUDE)
		endif()
	endif(MSVC)
	
	set(CPACK_PACKAGE_NAME "SGCT")
	set(CPACK_BUNDLE_NAME "SGCT")
	#path to readme
	set(CPACK_PACKAGE_DESCRIPTION_FILE "${PROJECT_SOURCE_DIR}/installer_readme.txt")
	#path to license
	set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/EULA.rtf")
	set(CPACK_RESOURCE_FILE_README "${PROJECT_SOURCE_DIR}/installer_readme.txt")
	
	#New CMake variables:
	#CPACK_WIX_PRODUCT_ICON
	#CPACK_WIX_UI_BANNER
	#CPACK_WIX_UI_DIALOG

	#Documentation:
	#CPACK_WIX_PRODUCT_ICON -- icon used to the left of the application entry in add/remove programs.
	#CPACK_WIX_UI_BANNER -- 493 by 58 pixels, this bitmap will appear at the top of all but the first page of the installer.
	#CPACK_WIX_UI_DIALOG -- 493 by 312 pixels, this bitmap will appear on the first page of the installer.
	
	#fix nsis path problem
	if(WIN32)
		SET(IMAGE_PATH "${PROJECT_SOURCE_DIR}\\\\image.bmp")
		SET(SIDEBAR_IMAGE "${PROJECT_SOURCE_DIR}/src/installers\\\\sgct.bmp")
		#STRING(REGEX REPLACE "/" "\\\\" IMAGE_PATH "${PROJECT_SOURCE_DIR}/image.bmp")
	else()
		SET(IMAGE_PATH "${PROJECT_SOURCE_DIR}/image.bmp")
		SET(SIDEBAR_IMAGE "${PROJECT_SOURCE_DIR}/src/installers/sgct.bmp")
	endif()
	SET(CPACK_NSIS_INSTALLER_MUI_ICON_CODE "!define MUI_WELCOMEFINISHPAGE_BITMAP \\\"${SIDEBAR_IMAGE}\\\"\n!define MUI_ICON \\\"${PROJECT_SOURCE_DIR}/icon.ico\\\"")
	
	#icon
	#SET(CPACK_NSIS_MUI_ICON "${PROJECT_SOURCE_DIR}/icon.ico")
	SET(CPACK_WIX_PRODUCT_ICON "${PROJECT_SOURCE_DIR}/icon.ico")
	
	if(WIN32)
		if(CMAKE_CL_64)
			SET(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
		else(CMAKE_CL_64)
			SET(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES")
		endif(CMAKE_CL_64)
			
		SET(CPACK_PACKAGE_ICON ${IMAGE_PATH})
	endif(WIN32)
	
	set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "SGCT - Simple Graphics Cluster Toolkit")
	set(CPACK_PACKAGE_VENDOR "Linkoping University")
	set(CPACK_PACKAGE_VERSION_MAJOR	${SGCT_VERSION_MAJOR})
	set(CPACK_PACKAGE_VERSION_MINOR	${SGCT_VERSION_MINOR})
	set(CPACK_PACKAGE_VERSION_PATCH ${SGCT_VERSION_REVISION})
	include(CPack)
endif(SGCT_INSTALL)
