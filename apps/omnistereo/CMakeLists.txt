##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2023                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

add_executable(omnistereo main.cpp)
set_compile_options(omnistereo)
find_package(glm REQUIRED)
target_link_libraries(omnistereo PRIVATE sgct::sgct glm::glm)

add_custom_command(
  TARGET omnistereo POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
  "${CMAKE_CURRENT_SOURCE_DIR}/sepmap.png"
  "${CMAKE_CURRENT_SOURCE_DIR}/test.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/turnmap.jpg"
  "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/box.png"

  $<TARGET_FILE_DIR:omnistereo>
)
set_property(TARGET omnistereo PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:omnistereo>)
set_target_properties(omnistereo PROPERTIES FOLDER "Examples")

if (WIN32 AND $<TARGET_RUNTIME_DLLS:omnistereo>)
  add_custom_command(TARGET omnistereo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:omnistereo> $<TARGET_FILE_DIR:omnistereo>
    COMMAND_EXPAND_LISTS
  )
endif ()
