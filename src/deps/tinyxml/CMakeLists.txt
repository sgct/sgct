cmake_minimum_required(VERSION 2.6)

project(TinyXml)

include_directories(./)

file(
	GLOB_RECURSE
	source_files
	*.h
	*.cpp
)

SET(CMAKE_C_FLAGS -DTIXML_USE_STL)
ADD_DEFINITIONS(-DTIXML_USE_STL)
add_library( tinyxml ${source_files} )

INSTALL( FILES tinyxml.h tinystr.h DESTINATION include/ )
INSTALL( FILES lib/libtinyxml.so DESTINATION lib/ )
