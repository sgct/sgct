##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2023                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

add_executable(network main.cpp)
set_compile_options(network)
find_package(glm REQUIRED)
target_link_libraries(network PRIVATE sgct::sgct glm::glm)

add_custom_command(
  TARGET network POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
  "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/box.png"

  $<TARGET_FILE_DIR:network>
)
set_property(TARGET network PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:network>)
set_target_properties(network PROPERTIES FOLDER "Examples")

if (WIN32 AND $<TARGET_RUNTIME_DLLS:network>)
  add_custom_command(TARGET network POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:network> $<TARGET_FILE_DIR:network>
    COMMAND_EXPAND_LISTS
  )
endif ()
