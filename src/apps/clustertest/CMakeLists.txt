##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2020                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

add_executable(clustertest main.cpp)
set_compile_options(clustertest)
target_link_libraries(clustertest PRIVATE sgct)
copy_sgct_dynamic_libraries(clustertest)
set_property(TARGET clustertest PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:clustertest>)
set_target_properties(clustertest PROPERTIES FOLDER "Examples")

set(GLM_ROOT_DIR "${PROJECT_SOURCE_DIR}/ext/glm")
find_package(GLM REQUIRED)
mark_as_advanced(GLM_INCLUDE_DIR GLM_INCLUDE_DIRS)
target_include_directories(clustertest SYSTEM PRIVATE ${GLM_INCLUDE_DIRS})
