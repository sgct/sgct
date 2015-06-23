cmake_minimum_required(VERSION 2.6 FATAL_ERROR)
cmake_policy(VERSION 2.6)

SET(CMAKE_DEBUG_POSTFIX "d" CACHE STRING "add a postfix, usually d on windows")
SET(CMAKE_RELEASE_POSTFIX "" CACHE STRING "add a postfix, usually empty on windows")

project(tinyxml2)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/.")

if(MSVC)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	if(NOT "${MSVC_VERSION}" LESS 1400)
		add_definitions( "/MP" )
	endif()
	
	#disable pdb-file debugging
	#STRING(REPLACE "/Zi" "/Z7" replacementFlags ${CMAKE_CXX_FLAGS_DEBUG})
	#set(CMAKE_CXX_FLAGS_DEBUG "${replacementFlags}")
endif(MSVC)

################################
# Add targets
add_library(tinyxml2static STATIC
	tinyxml2.cpp
	tinyxml2.h)
set_target_properties(tinyxml2static PROPERTIES OUTPUT_NAME tinyxml2)

configure_file(tinyxml2.pc.in tinyxml2.pc @ONLY)