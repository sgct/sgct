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
copy_sgct_dynamic_libraries(test-runner)
set_target_properties(test-runner PROPERTIES FOLDER "Examples")
