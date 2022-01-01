##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2022                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

add_executable(heightmappingndisender main.cpp ndi.h ndi.cpp heightmap.frag heightmap.vert)
set_compile_options(heightmappingndisender)
target_link_libraries(heightmappingndisender PRIVATE sgct)

find_path(NDI_INCLUDE_DIRECTORY NAMES NDI REQUIRED)
find_library(NDI_LIBRARY NAMES Processing.NDI.Lib.x64 REQUIRED)
target_include_directories(heightmappingndisender PRIVATE ${NDI_INCLUDE_DIRECTORY})
target_link_libraries(heightmappingndisender PRIVATE ${NDI_LIBRARY})

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/fisheye.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/heightmap.frag"
  "${CMAKE_CURRENT_SOURCE_DIR}/heightmap.vert"
)
copy_files(heightmappingndisender ${files})
copy_sgct_dynamic_libraries(heightmappingndisender)
set_property(TARGET heightmappingndisender PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:heightmappingndisender>)
set_target_properties(heightmappingndisender PROPERTIES FOLDER "Examples")
