##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2026                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

add_executable(calibrator main.cpp)
set_compile_options(calibrator)
find_package(stbimage CONFIG REQUIRED)
find_package(glm REQUIRED)

# calibrator produces a final link (not just a static library), so it needs the compiled
# stbimage implementation, not just the declarations sgct::sgct already carries.
target_link_libraries(calibrator PRIVATE sgct::sgct glm::glm unofficial::stbimage::impl)

add_custom_command(
  TARGET calibrator
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/test-pattern-0.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/test-pattern-1.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/test-pattern-2.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/test-pattern-3.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/test-pattern-4.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/test-pattern-5.png"

    $<TARGET_FILE_DIR:calibrator>
)
set_property(TARGET calibrator PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:calibrator>)
