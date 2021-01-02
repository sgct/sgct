##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2021                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

add_executable(heightmapping main.cpp)
set_compile_options(heightmapping)
find_package(glm REQUIRED)
target_link_libraries(heightmapping PRIVATE sgct glm)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/fisheye.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/heightmap.png"
  "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/normalmap.png"
)
copy_files(heightmapping ${files})
copy_sgct_dynamic_libraries(heightmapping)
set_property(TARGET heightmapping PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:heightmapping>)
set_target_properties(heightmapping PROPERTIES FOLDER "Examples")
