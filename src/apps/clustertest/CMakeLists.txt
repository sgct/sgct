##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2022                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

add_executable(clustertest main.cpp)
set_compile_options(clustertest)

find_package(glm REQUIRED)
target_link_libraries(clustertest PRIVATE sgct glm)
copy_sgct_dynamic_libraries(clustertest)
set_property(TARGET clustertest PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:clustertest>)
set_target_properties(clustertest PROPERTIES FOLDER "Examples")
