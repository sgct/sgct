# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(fisheyeTest main.cpp)
set_compile_options(fisheyeTest)
target_link_libraries(fisheyeTest PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/grid_bottom.png"
  "${CMAKE_CURRENT_SOURCE_DIR}/grid_left.png"
  "${CMAKE_CURRENT_SOURCE_DIR}/grid_right.png"
  "${CMAKE_CURRENT_SOURCE_DIR}/grid_top.png"
)
copy_files(fisheyeTest ${files})
copy_sgct_dynamic_libraries(fisheyeTest)
