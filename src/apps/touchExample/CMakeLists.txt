# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(touchExample main.cpp)
set_compile_options(touchExample)
target_link_libraries(touchExample PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/gridShader.frag"
  "${CMAKE_CURRENT_SOURCE_DIR}/gridShader.vert"
  "${CMAKE_CURRENT_SOURCE_DIR}/pyramidShader.frag"
  "${CMAKE_CURRENT_SOURCE_DIR}/pyramidShader.vert"
  "${CMAKE_CURRENT_SOURCE_DIR}/run_fisheye.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/run_fisheye.command"
  "${CMAKE_CURRENT_SOURCE_DIR}/run_two_nodes.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/run_two_nodes.command"
  "${CMAKE_CURRENT_SOURCE_DIR}/two_nodes.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/single_fisheye.xml"
)
copy_files(touchExample ${files})
copy_sgct_dynamic_libraries(touchExample)
