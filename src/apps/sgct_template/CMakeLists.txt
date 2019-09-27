# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(sgct_template main.cpp)
set_compile_options(sgct_template)
target_link_libraries(sgct_template PRIVATE sgct)
copy_sgct_dynamic_libraries(sgct_template)
set_target_properties(sgct_template PROPERTIES FOLDER "Examples")
