##########################################################################################
# SGCT                                                                                   #
# Simple Graphics Cluster Toolkit                                                        #
#                                                                                        #
# Copyright (c) 2012-2023                                                                #
# For conditions of distribution and use, see copyright notice in LICENSE.md             #
##########################################################################################

add_executable(stitcher main.cpp)
set_compile_options(stitcher)
target_link_libraries(stitcher PRIVATE sgct::sgct)

add_custom_command(
  TARGET stitcher POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
  "${CMAKE_CURRENT_SOURCE_DIR}/bottom.png"
  "${CMAKE_CURRENT_SOURCE_DIR}/left.png"
  "${CMAKE_CURRENT_SOURCE_DIR}/right.png"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_mono.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_stereo.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/top.png"
  "${CMAKE_CURRENT_SOURCE_DIR}/fisheye.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/fisheye_stereo.xml"

  $<TARGET_FILE_DIR:stitcher>
)
set_property(TARGET stitcher PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:stitcher>)
set_target_properties(stitcher PROPERTIES FOLDER "Examples")

if (MSVC)
  configure_file(
    ${PROJECT_SOURCE_DIR}/src/apps/stitcher/Stitcher_GUI/Stitcher_GUI/Stitcher_GUI.csproj.in
    ${PROJECT_SOURCE_DIR}/src/apps/stitcher/Stitcher_GUI/Stitcher_GUI/Stitcher_GUI.csproj
  )
  include_external_msproject(Stitcher_GUI
    ${PROJECT_SOURCE_DIR}/src/apps/stitcher/Stitcher_GUI/Stitcher_GUI/Stitcher_GUI.csproj
  )
  set_target_properties(Stitcher_GUI PROPERTIES FOLDER "Examples")
endif ()

if (WIN32 AND $<TARGET_RUNTIME_DLLS:stitcher>)
  add_custom_command(TARGET stitcher POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:stitcher> $<TARGET_FILE_DIR:stitcher>
    COMMAND_EXPAND_LISTS
  )
endif ()
