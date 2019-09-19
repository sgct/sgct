# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(postFXExample_opengl3 main.cpp)
set_compile_options(postFXExample_opengl3)
target_link_libraries(postFXExample_opengl3 PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/base.vert"
  "${CMAKE_CURRENT_SOURCE_DIR}/blur.frag"
  "${CMAKE_CURRENT_SOURCE_DIR}/blur_h.vert"
  "${CMAKE_CURRENT_SOURCE_DIR}/blur_v.vert"
  "${CMAKE_CURRENT_SOURCE_DIR}/glow.frag"
  "${CMAKE_CURRENT_SOURCE_DIR}/run.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/run.command"
  "${CMAKE_CURRENT_SOURCE_DIR}/simple.frag"
  "${CMAKE_CURRENT_SOURCE_DIR}/simple.vert"
  "${CMAKE_CURRENT_SOURCE_DIR}/threshold.frag"
  "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/box.png"
)
copy_files(postFXExample_opengl3 ${files})
copy_sgct_dynamic_libraries(postFXExample_opengl3)
