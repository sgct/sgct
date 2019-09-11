# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(simpleShading main.cpp)
set_compile_options(simpleShading)
target_link_libraries(simpleShading PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/run.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/run.command"
)
copy_files(simpleShading ${files})
copy_sgct_dynamic_libraries(simpleShading)
