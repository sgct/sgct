# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(simpleShaderExample_opengl3 main.cpp)
set_compile_options(simpleShaderExample_opengl3)
target_link_libraries(simpleShaderExample_opengl3 PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/simple.frag"
  "${CMAKE_CURRENT_SOURCE_DIR}/simple.vert"
)
copy_files(simpleShaderExample_opengl3 ${files})
copy_sgct_dynamic_libraries(simpleShaderExample_opengl3)
