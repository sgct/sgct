##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2020                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

add_executable(spoutscreenshot main.cpp)
set_compile_options(spoutscreenshot)
target_link_libraries(spoutscreenshot PRIVATE sgct)
copy_sgct_dynamic_libraries(spoutscreenshot)
set_property(TARGET spoutscreenshot PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:spoutscreenshot>)
set_target_properties(spoutscreenshot PROPERTIES FOLDER "Examples")
