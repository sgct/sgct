# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(domeImageViewer_opengl3 main.cpp)
set_compile_options(domeImageViewer_opengl3)
target_link_libraries(domeImageViewer_opengl3 PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/run.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/run.command"
  "${CMAKE_CURRENT_SOURCE_DIR}/run_stereo_dummy.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/run_stereo_dummy.command"
  "${CMAKE_CURRENT_SOURCE_DIR}/two_nodes.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/two_nodes_stereo_dummy.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/xform.frag"
  "${CMAKE_CURRENT_SOURCE_DIR}/xform.vert"
)
copy_files(domeImageViewer_opengl3 ${files})
copy_sgct_dynamic_libraries(domeImageViewer_opengl3)
