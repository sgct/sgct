# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(sgct_template main.cpp)
set_compile_options(sgct_template)
target_link_libraries(sgct_template PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/run.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/run.command"
)
copy_files(sgct_template ${files})
copy_sgct_dynamic_libraries(sgct_template)
