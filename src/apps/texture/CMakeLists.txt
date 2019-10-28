# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(texture main.cpp)
set_compile_options(texture)
target_link_libraries(texture PRIVATE sgct)

set(files "${CMAKE_CURRENT_SOURCE_DIR}/../SharedResources/box.png")
copy_files(texture ${files})
copy_sgct_dynamic_libraries(texture)
set_target_properties(texture PROPERTIES FOLDER "Examples")
