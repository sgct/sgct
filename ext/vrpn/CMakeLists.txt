cmake_minimum_required(VERSION 2.8.3)
project(VRPN)

#-----------------------------------------------------------------------------
# XXX Things to make better.
#
# Repeat for all other configurable headers/libraries - see below for a list
# Move the shared-library code over to CMake's normal definition
# Improve this CPack installer.

SET(CMAKE_DEBUG_POSTFIX "d" CACHE STRING "add a postfix, usually d on windows")
SET(CMAKE_RELEASE_POSTFIX "" CACHE STRING "add a postfix, usually empty on windows")

if((NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR) OR VRPN_SUBPROJECT_BUILD)
	set(SUBPROJECT TRUE)
	set(DEFAULT_OFF_IF_SUBPROJECT OFF_BY_DEFAULT)
	set(TRUE_UNLESS_SUBPROJECT FALSE)
else()
	set(SUBPROJECT FALSE)
	set(DEFAULT_OFF_IF_SUBPROJECT)
	set(TRUE_UNLESS_SUBPROJECT TRUE)
endif()

# TODO Remove the following when it is fixed in the Android CMake.
if(BUILD_WITH_ANDROID_NDK)
	set(CMAKE_CXX_FLAGS
		"--sysroot=${ANDROID_NDK_SYSROOT} ${CMAKE_CXX_FLAGS}")
	set(CMAKE_C_FLAGS "--sysroot=${ANDROID_NDK_SYSROOT} ${CMAKE_C_FLAGS}")
endif()

###
# On Windows 7, it does not work to install in the default location,
# which is the Program Files directory, because you have to not only have
# file permission to write there but also "run as administrator."  This
# means that "make install" from a Visual Studio project fails.  To get
# around that, we need to set CMAKE_INSTALL_PREFIX to something other
# than the default.  However, it is a cache variable that has already been
# set.  If we make a local variable, it uses this rather than the cache
# variable and never tells the poor user what happened (the GUI location
# looks standard but the files end up somewhere else).  If we make it a
# non-forced cache variable, it already has a value so does not change.
# If we make it a forced cache variable, it gets overwritten every time
# and the user cannot change it on the GUI.  So we have a workaround here.
# We make a cache variable that records whether we have ever forced the
# install prefix.  If not, we force it.  If so, we don't force it again.
# This has the effect of setting it the first time cmake is run, showing
# the change in the GUI, and also letting the user change the value in
# the GUI if they don't like what we did.  If I knew how to do this only
# happen on Windows 7, I'd make the if(WIN32) more specific.
if(WIN32 AND NOT SUBPROJECT)
	if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT
		AND
		(NOT
		ONCE_SET_CMAKE_INSTALL_PREFIX))
		set(ONCE_SET_CMAKE_INSTALL_PREFIX
			true
			CACHE
			INTERNAL
			"Have we set the install prefix yet?"
			FORCE)
		set(CMAKE_INSTALL_PREFIX
			C:/usr/local
			CACHE
			PATH
			"Install path prefix, prepended onto install directories"
			FORCE)
	endif()
endif()

if( MSVC )
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LTCG")
	
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

#-----------------------------------------------------------------------------
# Compiler flags we got from Hans for Windows and from Sheldon Andrews
# for other architectures.
if(MSVC)	# MS-Windows Visual Studio, both 32 and 64 bits
    #set(CMAKE_DEBUG_POSTFIX "d" CACHE STRING "add a postfix, usually d on windows")
	if(MSVC_VERSION GREATER 1310) # This compiler flag needs newer than VS.NET 2003 (7.1)
		# Choose fast, possibly less accurate floating point
		# See http://msdn.microsoft.com/en-us/library/e7s85ffb(v=vs.80).aspx
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /fp:fast")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:fast")
	endif()

	# Do not assume fixed base address (probably for DLL integration?)
	# http://msdn.microsoft.com/en-us/library/w368ysh2(v=vs.80).aspx
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /FIXED:NO")
else()
	# GCC compilers on 64-bit machines require -fPIC for shared libraries or libs
	# linked into shared libraries.

	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_SHARED_LIBRARY_C_FLAGS}")
	set(CMAKE_CXX_FLAGS
		"${CMAKE_CXX_FLAGS} ${CMAKE_SHARED_LIBRARY_CXX_FLAGS}")
endif()

# Set up correct defines for Windows header compilation:
# This theoretically sets the lower-bound on operating system compatibility
# http://msdn.microsoft.com/en-us/library/windows/desktop/aa383745(v=vs.85).aspx
# NT4	0x0400
# Win2k	0x0500
# WinXP	0x0501
# WS2003SP1/WinXPSP2 	0x0502
# Vista 0x0600
# Win7  0x0601
# Win8  0x0601
if(WIN32)
	set(WIN_MIN_VER 0x0500) # Default to Windows 2000
	if(MSVC AND MSVC_VERSION GREATER 1699)
		# VS2012: defaults to vista+, update 1 added XP support in additional toolset.
		if("${CMAKE_VS_PLATFORM_TOOLSET}" MATCHES "_xp")
			set(WIN_MIN_VER 0x0501) # WinXP
		else()
			set(WIN_MIN_VER 0x0600) # Vista
		endif()
	endif()
	add_definitions("-D_WIN32_WINNT=${WIN_MIN_VER}" "-DNTDDI_VERSION=${WIN_MIN_VER}0000")
endif()

#-----------------------------------------------------------------------------
# Options that control what gets built and how.

#if(NOT SUBPROJECT)
	# We can build two configurations (passing defs to the compile lines) - which ones do we want?
	# Note that we can build both now, if desired!
	SET(VRPN_BUILD_CLIENT_LIBRARY ON)
	SET(VRPN_BUILD_SERVER_LIBRARY OFF)

	# Build various applications if we want them.
	SET(VRPN_BUILD_CLIENTS OFF)
	SET(VRPN_BUILD_SERVERS OFF)

	# Development tools
	if(MSVC)
		SET(VRPN_BUILD_WITH_PROJECT_FOLDERS OFF)
			
		#disable pdb-file debugging
		#STRING(REPLACE "/Zi" "/Z7" replacementFlags ${CMAKE_CXX_FLAGS_DEBUG})
		#set(CMAKE_CXX_FLAGS_DEBUG "${replacementFlags}")
		
		if(NOT "${MSVC_VERSION}" LESS 1400)
			add_definitions( "/MP" )
		endif()
	else()
		set(VRPN_BUILD_WITH_PROJECT_FOLDERS ON)
	endif()
#endif()

# Force use of our CMake-processed configuration header before the stock one.
include_directories("${PROJECT_BINARY_DIR}")


# Include directory needed by all of the files
include_directories(${VRPN_SOURCE_DIR}
	${VRPN_SOURCE_DIR}/quat)


#-----------------------------------------------------------------------------
# Libraries we need to do our thing.
#
# CMake variables:
#   SERVER_EXTRA_LIBS - libraries to link against when building the server lib
#   EXTRA_LIBS - libraries to link against when building any VRPN lib
#
# Note that library linking is, by default, transitive:
# Specify linking here (even though static libraries might not use it
# directly - think of shared libs and your fellow developer) rather than
# in the included apps.

###
# Quatlib
###
add_subdirectory(quat)
list(APPEND EXTRA_LIBS quat)

###
# Threading (not on win32)
# Remove the test for Android when threads are fixed in the Android CMake.
###
if(NOT WIN32 AND NOT BUILD_WITH_ANDROID_NDK)
	find_package(Threads REQUIRED)
	list(APPEND EXTRA_LIBS ${CMAKE_THREAD_LIBS_INIT})
endif()

###
# Windows-specific (non-Cygwin) dependencies
###
if(WIN32 AND NOT UNIX)
	# Winsock - needed for endianness conversion
	list(APPEND EXTRA_LIBS ws2_32)

	# Windows multimedia - needed for joywin32
	list(APPEND EXTRA_LIBS winmm)
endif()

set(VRPN_CLIENT_ONLY)
if(VRPN_BUILD_CLIENT_LIBRARY AND NOT VRPN_BUILD_SERVER_LIBRARY)
	# We can define VRPN_CLIENT_ONLY in the header in this case!
	set(VRPN_CLIENT_ONLY ON)
endif()

#-----------------------------------------------------------------------------
# configure a header file to pass some of the CMake settings
# to the source code
configure_file("${PROJECT_SOURCE_DIR}/vrpn_Configure.h.cmake_in"
	"${PROJECT_BINARY_DIR}/vrpn_Configure.h")
file(RELATIVE_PATH
	VRPN_PATH_TO_CMAKE_CONFIG
	"${CMAKE_CURRENT_SOURCE_DIR}"
	"${PROJECT_BINARY_DIR}/vrpn_Configure.h")
add_definitions("-DVRPN_USING_CMAKE=\"${VRPN_PATH_TO_CMAKE_CONFIG}\"")

if(SUBPROJECT)
	set(BUILD_TESTING FALSE)
endif()

if(APPLE)
	if(NOT CMAKE_OSX_ARCHITECTURES OR CMAKE_OSX_ARCHITECTURES STREQUAL "")
		if(_CMAKE_OSX_MACHINE MATCHES "ppc")
			set(CMAKE_OSX_ARCHITECTURES
				"ppc;ppc64"
				CACHE
				STRING
				"Build architectures for OS X"
				FORCE)
		else()
			set(CMAKE_OSX_ARCHITECTURES
				"i386;x86_64"
				CACHE
				STRING
				"Build architectures for OS X"
				FORCE)
		endif()
	endif()
	set(CMAKE_INCLUDE_SYSTEM_FLAG_C "-isystem ")
	set(CMAKE_INCLUDE_SYSTEM_FLAG_CXX "-isystem ")
	if(NOT CMAKE_INSTALL_NAME_DIR)
		set(CMAKE_INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib")
	endif()
	message(STATUS
		"Building ${CMAKE_PROJECT_NAME} for ${CMAKE_OSX_ARCHITECTURES}")
endif()

#-----------------------------------------------------------------------------
# Build the library itself and declare what bits need to be installed

set(VRPN_CLIENT_SOURCES
	vrpn_Analog.c
	vrpn_Analog_Output.c
	vrpn_Auxiliary_Logger.c
	vrpn_BaseClass.c
	vrpn_Button.c
	vrpn_Connection.c
	vrpn_Dial.c
	vrpn_FileConnection.c
	vrpn_FileController.c
	vrpn_ForceDevice.c
	vrpn_Forwarder.c
	vrpn_ForwarderController.c
	vrpn_FunctionGenerator.c
	vrpn_Imager.c
	vrpn_LamportClock.c
	vrpn_Mutex.c
	vrpn_Poser.c
	vrpn_RedundantTransmission.c
	vrpn_Serial.c
	vrpn_SerialPort.c
	vrpn_Shared.c
	vrpn_SharedObject.c
	vrpn_Sound.c
	vrpn_Text.c
	vrpn_Tracker.c)

set(VRPN_CLIENT_PUBLIC_HEADERS
	"${PROJECT_BINARY_DIR}/vrpn_Configure.h"
	vrpn_Analog.h
	vrpn_Analog_Output.h
	vrpn_Auxiliary_Logger.h
	vrpn_BaseClass.h
	vrpn_Button.h
	vrpn_Connection.h
	vrpn_ConnectionPtr.h
	vrpn_Dial.h
	vrpn_FileConnection.h
	vrpn_FileController.h
	vrpn_ForceDevice.h
	vrpn_ForwarderController.h
	vrpn_Forwarder.h
	vrpn_FunctionGenerator.h
	vrpn_Imager.h
	vrpn_LamportClock.h
	vrpn_Log.h
	vrpn_MainloopContainer.h
	vrpn_MainloopObject.h
	vrpn_Mutex.h
	vrpn_RedundantTransmission.h
	vrpn_SendTextMessageStreamProxy.h
	vrpn_Serial.h
	vrpn_SerialPort.h
	vrpn_Shared.h
	vrpn_SharedObject.h
	vrpn_Sound.h
	vrpn_Text.h
	vrpn_Tracker.h
	vrpn_Types.h)

set(VRPN_SERVER_LIBRARY)
set(VRPN_CLIENT_LIBRARY)

if(VRPN_BUILD_CLIENT_LIBRARY)
	set_source_files_properties(vrpn ${VRPN_CLIENT_SOURCES} PROPERTIES LANGUAGE "CXX" )
	add_library(vrpn ${VRPN_CLIENT_SOURCES} ${VRPN_CLIENT_PUBLIC_HEADERS})
	target_link_libraries(vrpn ${EXTRA_LIBS})
	set_target_properties(vrpn PROPERTIES DEFINE_SYMBOL VRPNDLL_EXPORTS)
	target_include_directories(vrpn PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
	target_include_directories(vrpn PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
	set(VRPN_CLIENT_LIBRARY vrpn)

	set_property(TARGET
		vrpn
		PROPERTY
		PUBLIC_HEADER
		${VRPN_CLIENT_PUBLIC_HEADERS})

	if(NOT VRPN_CLIENT_ONLY)
		set_property(TARGET
			vrpn
			PROPERTY
			COMPILE_DEFINITIONS
			"VRPN_CLIENT_ONLY")
	endif()
	set_property(TARGET
		vrpn
		PROPERTY
		PROJECT_LABEL
		"VRPN")
	set_property(TARGET
		vrpn
		PROPERTY
		FOLDER
		"External")
endif()

add_subdirectory(client_src)

#-----------------------------------------------------------------------------
# Set some internal cache variables to make life easier if we're a subproject

set(VRPN_DEFINITIONS "-DVRPN_USING_CMAKE=\"${CMAKE_CURRENT_BINARY_DIR}/vrpn_Configure.h\""
	CACHE INTERNAL "Definition to add if using as a subproject" FORCE)
set(VRPN_CLIENT_LIBRARY ${VRPN_CLIENT_LIBRARY} CACHE INTERNAL "" FORCE)
set(VRPN_INCLUDE_DIRS
	"${CMAKE_CURRENT_BINARY_DIR}"
	"${VRPN_SOURCE_DIR}"
	"${VRPN_SOURCE_DIR}/quat"
	CACHE INTERNAL "" FORCE)

#-----------------------------------------------------------------------------
# Build the RPC generation if we've been asked to

if(VRPN_BUILD_TEST_RPC_GENERATION)
	add_subdirectory(util/gen_rpc)
endif()

if(VRPN_BUILD_CLIENTS)
	add_subdirectory(util/printStream)
endif()

#-----------------------------------------------------------------------------
# Create documentation
if(NOT SUBPROJECT)
	add_subdirectory(doxygen)
endif()

#-----------------------------------------------------------------------------
# Do a little check for GPL and GPL-incompatible libraries

# What flags cause us to link against GPL libraries?

# TODO: Figure out about the kernel headers we use.
# Conflicting stories:
# ML post by Linus: https://lkml.org/lkml/2003/12/5/13
# Interview of Linus: http://www.itworld.com/open-source/140916/android-sued-microsoft-not-linux

set(ALL_GPL_SERVER_FLAGS
	#VRPN_USE_DEV_INPUT # Kernel header: GPL2
	#VRPN_USE_JOYLIN # Kernel header: GPL2
	VRPN_USE_GPM_MOUSE # GPL2+
	VRPN_USE_WIIUSE # GPL3
)

# What flags cause us to link against GPL-incompatible libraries?
set(ALL_GPLINCOMPAT_SERVER_FLAGS
	VRPN_INCLUDE_TIMECODE_SERVER
	VRPN_USE_PHANTOM_SERVER)

set(GPL_SERVER_FLAGS)
foreach(POSSIBLE_GPL_FLAG ${ALL_GPL_SERVER_FLAGS})
	if(${POSSIBLE_GPL_FLAG})
		list(APPEND GPL_SERVER_FLAGS "${POSSIBLE_GPL_FLAG}")
	endif()
endforeach()

set(GPLINCOMPAT_SERVER_FLAGS)
foreach(POSSIBLE_GPLINCOMPAT_FLAG ${ALL_GPLINCOMPAT_SERVER_FLAGS})
	if(${POSSIBLE_GPLINCOMPAT_FLAG})
		list(APPEND GPLINCOMPAT_SERVER_FLAGS "${POSSIBLE_GPLINCOMPAT_FLAG}")
	endif()
endforeach()


if(GPL_SERVER_FLAGS)
	# Some GPL options are enabled
	message(STATUS "")
	message(STATUS
		"NOTE: The following build options may produce a GPL-licensed server library/binary.")
	message(STATUS "  ${GPL_SERVER_FLAGS}")
	message(STATUS
		"NOTE: I am not a lawyer, and this is not legal advice!")

	option(VRPN_GPL_SERVER
		"Check this to accept the possibility of linking GPL libraries with the server."
		OFF)
endif()

if(GPLINCOMPAT_SERVER_FLAGS)
	# Some GPL-incompatible options are enabled
	message(STATUS "")
	message(STATUS
		"NOTE: The following build options may produce a server library/binary")
	message(STATUS
		"  that is incompatible with the GPL/undistributable if linked with GPL libraries.")
	message(STATUS "  ${GPLINCOMPAT_SERVER_FLAGS}")
	message(STATUS
		"NOTE: I am not a lawyer, and this is not legal advice!")
endif()

# Check for errors.
if(VRPN_BUILD_SERVER_LIBRARY)
	if(GPL_SERVER_FLAGS AND NOT VRPN_GPL_SERVER)
		message(STATUS "")
		message(STATUS "Selected build options produce a GPL server library.")
		message(STATUS
			"You may disable them, otherwise set VRPN_GPL_SERVER to acknowledge this and build anyway.")
		message(FATAL_ERROR
			"Need VRPN_GPL_SERVER to build server library with GPL options enabled!")
	endif()

	if(GPL_SERVER_FLAGS AND GPLINCOMPAT_SERVER_FLAGS)
		message(STATUS "")
		message(STATUS "IMPORTANT LICENSING NOTE!")
		message(STATUS
			"Building with the current settings may produce a legally non-distributable server binary!")
		message(STATUS
			"NOTE: I am not a lawyer, and this is not legal advice!")
	endif()
endif()
