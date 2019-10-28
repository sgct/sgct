# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(network main.cpp)
set_compile_options(network)
target_link_libraries(network PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/run.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/run.command"
  "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/box.png"
)
copy_files(network ${files})
copy_sgct_dynamic_libraries(network)
set_target_properties(network PROPERTIES FOLDER "Examples")
