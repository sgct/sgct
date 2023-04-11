##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2023                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

add_executable(heightmapping main.cpp)
set_compile_options(heightmapping)
find_package(glm REQUIRED)
target_link_libraries(heightmapping PRIVATE sgct::sgct glm::glm)

add_custom_command(
  TARGET heightmapping POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
  "${CMAKE_CURRENT_SOURCE_DIR}/fisheye.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/heightmap.png"
  "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/normalmap.png"

  $<TARGET_FILE_DIR:heightmapping>
)
set_property(TARGET heightmapping PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:heightmapping>)
set_target_properties(heightmapping PROPERTIES FOLDER "Examples")

if (WIN32 AND $<TARGET_RUNTIME_DLLS:heightmapping>)
  add_custom_command(TARGET heightmapping POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:heightmapping> $<TARGET_FILE_DIR:heightmapping>
    COMMAND_EXPAND_LISTS
  )
endif ()
