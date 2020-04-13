##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2020                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

add_executable(simplenavigation main.cpp)
set_compile_options(simplenavigation)
target_link_libraries(simplenavigation PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/single_fisheye.xml"
)
copy_files(simplenavigation ${files})
copy_sgct_dynamic_libraries(simplenavigation)
set_property(TARGET simplenavigation PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:simplenavigation>)
set_target_properties(simplenavigation PROPERTIES FOLDER "Examples")

set(GLM_ROOT_DIR "${PROJECT_SOURCE_DIR}/ext/glm")
find_package(GLM REQUIRED)
mark_as_advanced(GLM_INCLUDE_DIR GLM_INCLUDE_DIRS)
target_include_directories(simplenavigation SYSTEM PRIVATE ${GLM_INCLUDE_DIRS})