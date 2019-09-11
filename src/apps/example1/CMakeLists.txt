# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(example1 main.cpp)
set_compile_options(example1)
target_link_libraries(example1 PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/run.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/run.command"
)
copy_files(example1 ${files})
copy_sgct_dynamic_libraries(example1)
