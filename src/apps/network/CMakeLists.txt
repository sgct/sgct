##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2022                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

add_executable(network main.cpp)
set_compile_options(network)
find_package(glm REQUIRED)
target_link_libraries(network PRIVATE sgct::sgct glm::glm)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/box.png"
)
copy_files(network ${files})
copy_sgct_dynamic_libraries(network)
set_property(TARGET network PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:network>)
set_target_properties(network PROPERTIES FOLDER "Examples")
