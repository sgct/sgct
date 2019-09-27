# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(postFXExample main.cpp)
set_compile_options(postFXExample)
target_link_libraries(postFXExample PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/base.vert"
  "${CMAKE_CURRENT_SOURCE_DIR}/blur.frag"
  "${CMAKE_CURRENT_SOURCE_DIR}/blur_h.vert"
  "${CMAKE_CURRENT_SOURCE_DIR}/blur_v.vert"
  "${CMAKE_CURRENT_SOURCE_DIR}/glow.frag"
  "${CMAKE_CURRENT_SOURCE_DIR}/threshold.frag"
  "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/box.png"
)
copy_files(postFXExample ${files})
copy_sgct_dynamic_libraries(postFXExample)
set_target_properties(postFXExample PROPERTIES FOLDER "Examples")
