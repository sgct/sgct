# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(openvrExample main.cpp)
set_compile_options(openvrExample)
target_link_libraries(openvrExample PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/gridShader.frag"
  "${CMAKE_CURRENT_SOURCE_DIR}/gridShader.vert"
  "${CMAKE_CURRENT_SOURCE_DIR}/openvr_window_same_as_hmd.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/openvr_with_fullscreen_mono.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/pyramidShader.frag"
  "${CMAKE_CURRENT_SOURCE_DIR}/pyramidShader.vert"
  "${CMAKE_CURRENT_SOURCE_DIR}/run.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/run.command"
  "${CMAKE_CURRENT_SOURCE_DIR}/run_fullscreen.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/run_fullscreen.command"
)
copy_files(openvrExample ${files})
copy_sgct_dynamic_libraries(openvrExample)
