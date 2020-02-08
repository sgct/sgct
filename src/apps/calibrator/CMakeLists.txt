##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2020                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

add_executable(calibrator main.cpp)
set_compile_options(calibrator)
target_link_libraries(calibrator PRIVATE sgct)

set(GLM_ROOT_DIR "${PROJECT_SOURCE_DIR}/ext/glm")
find_package(GLM REQUIRED)
mark_as_advanced(GLM_INCLUDE_DIR GLM_INCLUDE_DIRS)
target_include_directories(calibrator SYSTEM PRIVATE ${GLM_INCLUDE_DIRS})

set(files
    "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/test-pattern-0.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/test-pattern-1.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/test-pattern-2.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/test-pattern-3.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/test-pattern-4.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/test-pattern-5.png"
)
copy_files(calibrator ${files})
copy_sgct_dynamic_libraries(calibrator)
set_target_properties(calibrator PROPERTIES FOLDER "Examples")
