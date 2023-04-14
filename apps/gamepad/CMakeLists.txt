##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2023                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

add_executable(gamepad main.cpp)
set_compile_options(gamepad)
target_link_libraries(gamepad PRIVATE sgct::sgct)
set_property(TARGET gamepad PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:gamepad>)
set_target_properties(gamepad PROPERTIES FOLDER "Examples")

if (WIN32 AND $<TARGET_RUNTIME_DLLS:gamepad>)
  add_custom_command(TARGET gamepad POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:gamepad> $<TARGET_FILE_DIR:gamepad>
    COMMAND_EXPAND_LISTS
  )
endif ()
