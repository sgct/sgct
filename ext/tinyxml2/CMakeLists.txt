cmake_minimum_required(VERSION 2.6 FATAL_ERROR)
cmake_policy(VERSION 2.6)

SET(CMAKE_DEBUG_POSTFIX "d" CACHE STRING "add a postfix, usually d on windows")
SET(CMAKE_RELEASE_POSTFIX "" CACHE STRING "add a postfix, usually empty on windows")

project(tinyxml2)

if(MSVC)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	if(NOT "${MSVC_VERSION}" LESS 1400)
		add_definitions( "/MP" )
	endif()

	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LTCG")
	set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:LIBCMTD.lib")
	set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:LIBCMT.lib")
	
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
endif(MSVC)

################################
# Add targets
add_library(tinyxml2 tinyxml2.cpp tinyxml2.h)
add_library(tinyxml2::tinyxml2 ALIAS tinyxml2)
target_include_directories(tinyxml2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

configure_file(tinyxml2.pc.in tinyxml2.pc @ONLY)