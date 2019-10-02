# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(networkExample main.cpp)
set_compile_options(networkExample)
target_link_libraries(networkExample PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/run.bat"
  "${CMAKE_CURRENT_SOURCE_DIR}/run.command"
  "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/box.png"
)
copy_files(networkExample ${files})
copy_sgct_dynamic_libraries(networkExample)
set_target_properties(networkExample PROPERTIES FOLDER "Examples")
