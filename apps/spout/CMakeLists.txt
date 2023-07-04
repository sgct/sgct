##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2023                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

add_executable(spoutapp main.cpp)
set_compile_options(spoutapp)
find_package(glm REQUIRED)
target_link_libraries(spoutapp PRIVATE sgct::sgct glm::glm)
set_property(TARGET spoutapp PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:spoutapp>)
set_target_properties(spoutapp PROPERTIES FOLDER "Examples")

if (WIN32 AND $<TARGET_RUNTIME_DLLS:spoutapp>)
  add_custom_command(TARGET spoutapp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:spoutapp> $<TARGET_FILE_DIR:spoutapp>
    COMMAND_EXPAND_LISTS
  )
endif ()
