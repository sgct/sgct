# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(heightMappingExample_opengl3 main.cpp)
set_compile_options(heightMappingExample_opengl3)
target_link_libraries(heightMappingExample_opengl3 PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/fisheye.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/heightmap.frag"
  "${CMAKE_CURRENT_SOURCE_DIR}/heightmap.vert"
  "${CMAKE_CURRENT_SOURCE_DIR}/run.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/run.command"
  "${CMAKE_CURRENT_SOURCE_DIR}/run_fisheye.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/run_fisheye.command"
)
copy_files(heightMappingExample_opengl3 ${files})
copy_sgct_dynamic_libraries(heightMappingExample_opengl3)
