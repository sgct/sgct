##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2023                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

add_executable(domeimageviewer main.cpp)
set_compile_options(domeimageviewer)
target_link_libraries(domeimageviewer PRIVATE sgct::sgct)

add_custom_command(
  TARGET domeimageviewer POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
  "${CMAKE_CURRENT_SOURCE_DIR}/two_nodes.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/two_nodes_stereo_dummy.xml"

  $<TARGET_FILE_DIR:domeimageviewer>
)
set_property(TARGET domeimageviewer PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:domeimageviewer>)
set_target_properties(domeimageviewer PROPERTIES FOLDER "Examples")

if (WIN32 AND $<TARGET_RUNTIME_DLLS:domeimageviewer>)
  add_custom_command(TARGET domeimageviewer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:domeimageviewer> $<TARGET_FILE_DIR:domeimageviewer>
    COMMAND_EXPAND_LISTS
  )
endif ()
