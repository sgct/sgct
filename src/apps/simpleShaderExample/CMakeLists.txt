# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(simpleShaderExample main.cpp)
set_compile_options(simpleShaderExample)
target_link_libraries(simpleShaderExample PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/run.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/run.command"
  "${CMAKE_CURRENT_SOURCE_DIR}/simple.frag"
  "${CMAKE_CURRENT_SOURCE_DIR}/simple.vert"
)
copy_files(simpleShaderExample ${files})
copy_sgct_dynamic_libraries(simpleShaderExample)
