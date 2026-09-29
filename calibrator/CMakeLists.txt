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

target_link_libraries(calibrator PRIVATE sgct::sgct glm::glm unofficial::stbimage::stbimage-read-impl)

# The calibrator resolves these against its working directory rather than against its own
# location, so every place that provides the executable has to provide them alongside it
set(CALIBRATOR_TEST_PATTERNS
  "${CMAKE_CURRENT_SOURCE_DIR}/test-pattern-0.png"
  "${CMAKE_CURRENT_SOURCE_DIR}/test-pattern-1.png"
  "${CMAKE_CURRENT_SOURCE_DIR}/test-pattern-2.png"
  "${CMAKE_CURRENT_SOURCE_DIR}/test-pattern-3.png"
  "${CMAKE_CURRENT_SOURCE_DIR}/test-pattern-4.png"
  "${CMAKE_CURRENT_SOURCE_DIR}/test-pattern-5.png"
)

add_custom_command(
  TARGET calibrator
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CALIBRATOR_TEST_PATTERNS}
    $<TARGET_FILE_DIR:calibrator>
)
set_property(TARGET calibrator PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:calibrator>)

# Same condition as the library install in src/CMakeLists.txt: a project that vendors SGCT
# brings its own install rules and should not have ours appear in its packages
if (PROJECT_IS_TOP_LEVEL)
  include(GNUInstallDirs)

  # Into the binary folder and not a data folder of their own, because that is the working
  # directory the calibrator is started from. It is also what the NSIS installer runs at
  # the end, see CPACK_NSIS_DEFINES in the top-level CMakeLists
  install(TARGETS calibrator RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  install(FILES ${CALIBRATOR_TEST_PATTERNS} DESTINATION ${CMAKE_INSTALL_BINDIR})
endif ()
