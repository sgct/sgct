# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(example1_opengl3 main.cpp simple.frag simple.vert)
set_compile_options(example1_opengl3)
target_link_libraries(example1_opengl3 PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/simple.frag"
  "${CMAKE_CURRENT_SOURCE_DIR}/simple.vert"
)
copy_files(example1_opengl3 ${files})
copy_sgct_dynamic_libraries(example1_opengl3)
set_target_properties(example1_opengl3 PROPERTIES FOLDER "Examples")
