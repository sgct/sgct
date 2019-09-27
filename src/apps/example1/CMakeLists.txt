# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(example1 main.cpp)
set_compile_options(example1)
target_link_libraries(example1 PRIVATE sgct)
copy_sgct_dynamic_libraries(example1)
set_target_properties(example1 PROPERTIES FOLDER "Examples")
