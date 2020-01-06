##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2020                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

add_executable(gamepad main.cpp)
set_compile_options(gamepad)
target_link_libraries(gamepad PRIVATE sgct)
copy_sgct_dynamic_libraries(gamepad)
set_target_properties(gamepad PROPERTIES FOLDER "Examples")
