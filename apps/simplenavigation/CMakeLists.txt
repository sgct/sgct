##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2023                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

add_executable(simplenavigation main.cpp)
set_compile_options(simplenavigation)
find_package(glm REQUIRED)
target_link_libraries(simplenavigation PRIVATE sgct::sgct glm::glm)

add_custom_command(
  TARGET simplenavigation POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
  "${CMAKE_CURRENT_SOURCE_DIR}/single_fisheye.xml"

  $<TARGET_FILE_DIR:simplenavigation>
)
set_property(TARGET simplenavigation PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:simplenavigation>)
set_target_properties(simplenavigation PROPERTIES FOLDER "Examples")

if (WIN32 AND $<TARGET_RUNTIME_DLLS:simplenavigation>)
  add_custom_command(TARGET simplenavigation POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:simplenavigation> $<TARGET_FILE_DIR:simplenavigation>
    COMMAND_EXPAND_LISTS
  )
endif ()
