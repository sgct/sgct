# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(modelloader main.cpp)
set_compile_options(modelloader)
target_link_libraries(modelloader PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/box.png"
  "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/box.obj")
copy_files(modelloader ${files})
copy_sgct_dynamic_libraries(modelloader)
set_target_properties(modelloader PROPERTIES FOLDER "Examples")
