##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2020                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

add_executable(datatransfer main.cpp)
set_compile_options(datatransfer)
find_package(GLM REQUIRED)
target_link_libraries(datatransfer PRIVATE sgct glm)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/two_nodes.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/box.png"
)
copy_files(datatransfer ${files})
copy_sgct_dynamic_libraries(datatransfer)
set_property(TARGET datatransfer PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:datatransfer>)
set_target_properties(datatransfer PROPERTIES FOLDER "Examples")
