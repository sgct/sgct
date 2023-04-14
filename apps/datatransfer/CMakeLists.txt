##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2023                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

add_executable(datatransfer main.cpp)
set_compile_options(datatransfer)
find_package(glm REQUIRED)
target_link_libraries(datatransfer PRIVATE sgct::sgct glm::glm)

add_custom_command(
  TARGET datatransfer POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
  "${CMAKE_CURRENT_SOURCE_DIR}/two_nodes.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/box.png"

  $<TARGET_FILE_DIR:datatransfer>
)
set_property(TARGET datatransfer PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:datatransfer>)
set_target_properties(datatransfer PROPERTIES FOLDER "Examples")

if (WIN32 AND $<TARGET_RUNTIME_DLLS:datatransfer>)
  add_custom_command(TARGET datatransfer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:datatransfer> $<TARGET_FILE_DIR:datatransfer>
    COMMAND_EXPAND_LISTS
  )
endif ()
