# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(openvrFullDomeViewer main.cpp xform.frag xform.vert)
set_compile_options(openvrFullDomeViewer)
target_link_libraries(openvrFullDomeViewer PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/fisheye_left.jpg"
  "${CMAKE_CURRENT_SOURCE_DIR}/fisheye_right.jpg"
  "${CMAKE_CURRENT_SOURCE_DIR}/openvr_mono.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/openvr_stereo.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/run_mono.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/run_mono.command"
  "${CMAKE_CURRENT_SOURCE_DIR}/run_stereo.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/run_stereo.command"
)
copy_files(openvrFullDomeViewer ${files})
copy_sgct_dynamic_libraries(openvrFullDomeViewer)
set_target_properties(openvrFullDomeViewer PROPERTIES FOLDER "Examples")
