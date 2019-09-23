# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(textureExample_opengl3 main.cpp)
set_compile_options(textureExample_opengl3)
target_link_libraries(textureExample_opengl3 PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/simple.frag"
  "${CMAKE_CURRENT_SOURCE_DIR}/simple.vert"
  "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/box.png"
)
copy_files(textureExample_opengl3 ${files})
copy_sgct_dynamic_libraries(textureExample_opengl3)
