##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2020                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

add_executable(calibrator main.cpp)
set_compile_options(calibrator)
target_link_libraries(calibrator PRIVATE sgct)

copy_sgct_dynamic_libraries(calibrator)
set_target_properties(calibrator PROPERTIES FOLDER "Examples")
