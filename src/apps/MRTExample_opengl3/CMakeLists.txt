# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(MRTExample_opengl3 main.cpp)
set_compile_options(MRTExample_opengl3)
target_link_libraries(MRTExample_opengl3 PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/mrt.frag"
  "${CMAKE_CURRENT_SOURCE_DIR}/mrt.vert"
  "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/box.png"
)
copy_files(MRTExample_opengl3 ${files})
copy_sgct_dynamic_libraries(MRTExample_opengl3)
