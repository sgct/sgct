# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(spout main.cpp)
set_compile_options(spout)
target_link_libraries(spout PRIVATE sgct)
copy_sgct_dynamic_libraries(spout)
set_target_properties(spout PROPERTIES FOLDER "Examples")
