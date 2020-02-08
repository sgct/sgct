##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2020                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

add_executable(spout main.cpp)
set_compile_options(spout)
target_link_libraries(spout PRIVATE sgct)
copy_sgct_dynamic_libraries(spout)
set_target_properties(spout PROPERTIES FOLDER "Examples")

set(GLM_ROOT_DIR "${PROJECT_SOURCE_DIR}/ext/glm")
find_package(GLM REQUIRED)
mark_as_advanced(GLM_INCLUDE_DIR GLM_INCLUDE_DIRS)
target_include_directories(spout SYSTEM PRIVATE ${GLM_INCLUDE_DIRS})