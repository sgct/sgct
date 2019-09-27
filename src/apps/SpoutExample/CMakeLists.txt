# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(SpoutExample_opengl3 main.cpp)
set_compile_options(SpoutExample_opengl3)
target_link_libraries(SpoutExample_opengl3 PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/xform.frag"
  "${CMAKE_CURRENT_SOURCE_DIR}/xform.vert"
)
copy_files(SpoutExample_opengl3 ${files})
copy_sgct_dynamic_libraries(SpoutExample_opengl3)
set_target_properties(SpoutExample_opengl3 PROPERTIES FOLDER "Examples")
