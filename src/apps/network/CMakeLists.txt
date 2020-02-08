##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2020                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

add_executable(network main.cpp)
set_compile_options(network)
target_link_libraries(network PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/box.png"
)
copy_files(network ${files})
copy_sgct_dynamic_libraries(network)
set_target_properties(network PROPERTIES FOLDER "Examples")

set(GLM_ROOT_DIR "${PROJECT_SOURCE_DIR}/ext/glm")
find_package(GLM REQUIRED)
mark_as_advanced(GLM_INCLUDE_DIR GLM_INCLUDE_DIRS)
target_include_directories(network SYSTEM PRIVATE ${GLM_INCLUDE_DIRS})