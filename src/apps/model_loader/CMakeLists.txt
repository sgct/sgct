# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(model_loader main.cpp objloader.h objloader.cpp)
set_compile_options(model_loader)
target_link_libraries(model_loader PRIVATE sgct)

set(files
  "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/box.png"
  "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/box.obj")
copy_files(model_loader ${files})
copy_sgct_dynamic_libraries(model_loader)
set_target_properties(model_loader PROPERTIES FOLDER "Examples")
