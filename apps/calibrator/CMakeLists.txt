##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2023                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

add_executable(calibrator main.cpp)
set_compile_options(calibrator)
find_package(glm REQUIRED)
target_link_libraries(calibrator PRIVATE sgct::sgct glm::glm)

add_custom_command(
  TARGET calibrator POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/test-pattern-0.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/test-pattern-1.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/test-pattern-2.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/test-pattern-3.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/test-pattern-4.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/test-pattern-5.png"

    $<TARGET_FILE_DIR:calibrator>
)
set_property(TARGET calibrator PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:calibrator>)
set_target_properties(calibrator PROPERTIES FOLDER "Examples")

if (WIN32 AND $<TARGET_RUNTIME_DLLS:calibrator>)
  add_custom_command(
    TARGET calibrator POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:calibrator> $<TARGET_FILE_DIR:calibrator>
    COMMAND_EXPAND_LISTS
  )
endif ()
