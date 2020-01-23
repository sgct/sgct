##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2020                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

add_executable(test-runner main.cpp)
set_compile_options(test-runner)
target_link_libraries(test-runner PRIVATE sgct)

set(files
    "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/test-pattern-0.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/test-pattern-1.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/test-pattern-2.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/test-pattern-3.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/test-pattern-4.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/test-pattern-5.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/test-windows.xml"
)
copy_files(test-runner ${files})
copy_sgct_dynamic_libraries(test-runner)
set_target_properties(test-runner PROPERTIES FOLDER "Examples")
