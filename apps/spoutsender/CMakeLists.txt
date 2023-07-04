##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2023                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

add_executable(spoutsender main.cpp)
set_compile_options(spoutsender)
find_package(glm REQUIRED)
target_link_libraries(spoutsender PRIVATE sgct::sgct glm::glm)

add_custom_command(
  TARGET spoutsender POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
  "${SGCT_EXAMPLE_SHARED_RESOURCES}/box.png"

  $<TARGET_FILE_DIR:spoutsender>
)
set_property(TARGET spoutsender PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:spoutsender>)
set_target_properties(spoutsender PROPERTIES FOLDER "Examples")

if (WIN32 AND $<TARGET_RUNTIME_DLLS:spoutsender>)
  add_custom_command(TARGET spoutsender POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:spoutsender> $<TARGET_FILE_DIR:spoutsender>
    COMMAND_EXPAND_LISTS
  )
endif ()
