##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2020                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

add_executable(spout main.cpp)
set_compile_options(spout)
target_link_libraries(spout PRIVATE sgct)
copy_sgct_dynamic_libraries(spout)
set_target_properties(spout PROPERTIES FOLDER "Examples")
