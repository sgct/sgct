# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(domeimageviewer main.cpp)
set_compile_options(domeimageviewer)
target_link_libraries(domeimageviewer PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/run.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/run.command"
  "${CMAKE_CURRENT_SOURCE_DIR}/run_stereo_dummy.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/run_stereo_dummy.command"
  "${CMAKE_CURRENT_SOURCE_DIR}/two_nodes.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/two_nodes_stereo_dummy.xml"
)
copy_files(domeimageviewer ${files})
copy_sgct_dynamic_libraries(domeimageviewer)
set_target_properties(domeimageviewer PROPERTIES FOLDER "Examples")
