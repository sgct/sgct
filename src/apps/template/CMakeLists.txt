# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(template main.cpp)
set_compile_options(template)
target_link_libraries(template PRIVATE sgct)
copy_sgct_dynamic_libraries(template)
set_target_properties(template PROPERTIES FOLDER "Examples")
