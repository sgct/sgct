# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(example1_opengl3 main.cpp)
set_compile_options(example1_opengl3)
target_link_libraries(example1_opengl3 PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/run.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/run.command"
  "${CMAKE_CURRENT_SOURCE_DIR}/SimpleFragmentShader.fragmentshader"
  "${CMAKE_CURRENT_SOURCE_DIR}/SimpleVertexShader.vertexshader"
)
copy_files(example1_opengl3 ${files})
copy_sgct_dynamic_libraries(example1_opengl3)
