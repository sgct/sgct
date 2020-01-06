##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2020                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

add_executable(example1 main.cpp)
set_compile_options(example1)
target_link_libraries(example1 PRIVATE sgct)

copy_sgct_dynamic_libraries(example1)
set_target_properties(example1 PROPERTIES FOLDER "Examples")
