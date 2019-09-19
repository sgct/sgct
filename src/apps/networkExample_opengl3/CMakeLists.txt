# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(networkExample_opengl3 main.cpp)
set_compile_options(networkExample_opengl3)
target_link_libraries(networkExample_opengl3 PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/box.frag"
  "${CMAKE_CURRENT_SOURCE_DIR}/box.vert"
  "${CMAKE_CURRENT_SOURCE_DIR}/run.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/run.command"
  "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/box.png"
)
copy_files(networkExample_opengl3 ${files})
copy_sgct_dynamic_libraries(networkExample_opengl3)
