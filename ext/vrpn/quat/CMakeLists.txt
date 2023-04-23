cmake_minimum_required(VERSION 2.6)
project(quatlib)

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

set(QUATLIB_SOURCES matrix.c quat.c vector.c xyzquat.c)
set(QUATLIB_HEADER quat.h)

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

# Build the library itself and declare what bits need to be installed
add_library(quat STATIC ${QUATLIB_SOURCES} ${QUATLIB_HEADER})
set_property(TARGET quat PROPERTY PUBLIC_HEADER ${QUATLIB_HEADER})
#set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_target_properties(quat PROPERTIES FOLDER "External")
if(VRPN_INSTALL)
	install(TARGETS
		quat
		ARCHIVE
		DESTINATION
		lib
		COMPONENT
		serversdk
		PUBLIC_HEADER
		DESTINATION
		include
		COMPONENT
		serversdk)
	install(TARGETS
		quat
		ARCHIVE
		DESTINATION
		lib
		COMPONENT
		clientsdk
		PUBLIC_HEADER
		DESTINATION
		include
		COMPONENT
		clientsdk)
endif()
