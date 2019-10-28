# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(postfx main.cpp base.vert blur.frag blur_h.vert blur_v.vert
    glow.frag simple.frag simple.vert threshold.frag)
set_compile_options(postfx)
target_link_libraries(postfx PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/base.vert"
  "${CMAKE_CURRENT_SOURCE_DIR}/blur.frag"
  "${CMAKE_CURRENT_SOURCE_DIR}/blur_h.vert"
  "${CMAKE_CURRENT_SOURCE_DIR}/blur_v.vert"
  "${CMAKE_CURRENT_SOURCE_DIR}/glow.frag"
  "${CMAKE_CURRENT_SOURCE_DIR}/simple.frag"
  "${CMAKE_CURRENT_SOURCE_DIR}/simple.vert"
  "${CMAKE_CURRENT_SOURCE_DIR}/threshold.frag"
  "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/box.png"
)
copy_files(postfx ${files})
copy_sgct_dynamic_libraries(postfx)
set_target_properties(postfx PROPERTIES FOLDER "Examples")
