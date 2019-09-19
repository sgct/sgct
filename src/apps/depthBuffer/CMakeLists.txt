# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(depthBufferExample main.cpp)
set_compile_options(depthBufferExample)
target_link_libraries(depthBufferExample PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/depth.frag"
  "${CMAKE_CURRENT_SOURCE_DIR}/depth.vert"
  "${CMAKE_CURRENT_SOURCE_DIR}/fisheye.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/heightmap.frag"
  "${CMAKE_CURRENT_SOURCE_DIR}/heightmap.vert"
  "${CMAKE_CURRENT_SOURCE_DIR}/run.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/run.command"
  "${CMAKE_CURRENT_SOURCE_DIR}/run_fisheye.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/heightmap.png"
  "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/normalmap.png"
)
copy_files(depthBufferExample ${files})
copy_sgct_dynamic_libraries(depthBufferExample)
