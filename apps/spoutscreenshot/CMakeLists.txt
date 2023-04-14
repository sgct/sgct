##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2023                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

add_executable(spoutscreenshot main.cpp)
set_compile_options(spoutscreenshot)
target_link_libraries(spoutscreenshot PRIVATE sgct::sgct)
set_property(TARGET spoutscreenshot PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:spoutscreenshot>)
set_target_properties(spoutscreenshot PROPERTIES FOLDER "Examples")

if (WIN32 AND $<TARGET_RUNTIME_DLLS:spoutscreenshot>)
  add_custom_command(TARGET spoutscreenshot POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:spoutscreenshot> $<TARGET_FILE_DIR:spoutscreenshot>
    COMMAND_EXPAND_LISTS
  )
endif ()
