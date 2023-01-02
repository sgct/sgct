##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2023                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

add_executable(spoutbrowser main.cpp)
set_compile_options(spoutbrowser)
find_package(GLM REQUIRED)
target_link_libraries(spoutbrowser PRIVATE sgct glm)
copy_sgct_dynamic_libraries(spoutbrowser)
set_property(TARGET spout PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:spoutbrowser>)
set_target_properties(spoutbrowser PROPERTIES FOLDER "Examples")
