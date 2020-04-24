##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2020                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

add_executable(example1 main.cpp)
set_compile_options(example1)
find_package(GLM REQUIRED)
target_link_libraries(example1 PRIVATE sgct glm)

copy_sgct_dynamic_libraries(example1)
set_property(TARGET example1 PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:example1>)
set_target_properties(example1 PROPERTIES FOLDER "Examples")
