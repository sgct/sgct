##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2022                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

add_executable(spout main.cpp)
set_compile_options(spout)
find_package(GLM REQUIRED)
target_link_libraries(spout PRIVATE sgct::sgct glm::glm)
copy_sgct_dynamic_libraries(spout)
set_property(TARGET spout PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:spout>)
set_target_properties(spout PROPERTIES FOLDER "Examples")
