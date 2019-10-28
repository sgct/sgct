# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(simpleshader main.cpp)
set_compile_options(simpleshader)
target_link_libraries(simpleshader PRIVATE sgct)
copy_sgct_dynamic_libraries(simpleshader)
set_target_properties(simpleshader PROPERTIES FOLDER "Examples")
