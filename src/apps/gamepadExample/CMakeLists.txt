# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(gamepadExample main.cpp)
set_compile_options(gamepadExample)
target_link_libraries(gamepadExample PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/run.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/run.command"
)
copy_files(gamepadExample ${files})
copy_sgct_dynamic_libraries(gamepadExample)
