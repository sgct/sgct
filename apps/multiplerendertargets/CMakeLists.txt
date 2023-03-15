##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2023                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

add_executable(multiplerendertargets main.cpp)
set_compile_options(multiplerendertargets)
find_package(glm REQUIRED)
target_link_libraries(multiplerendertargets PRIVATE sgct::sgct glm::glm)

add_custom_command(
  TARGET multiplerendertargets POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
  "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/box.png"

  $<TARGET_FILE_DIR:multiplerendertargets>
)
set_property(TARGET multiplerendertargets PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:multiplerendertargets>)
set_target_properties(multiplerendertargets PROPERTIES FOLDER "Examples")

if (WIN32 AND $<TARGET_RUNTIME_DLLS:multiplerendertargets>)
  add_custom_command(TARGET multiplerendertargets POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:multiplerendertargets> $<TARGET_FILE_DIR:multiplerendertargets>
    COMMAND_EXPAND_LISTS
  )
endif ()
