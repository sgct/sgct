# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(simpleShaderExample main.cpp)
set_compile_options(simpleShaderExample)
target_link_libraries(simpleShaderExample PRIVATE sgct)
copy_sgct_dynamic_libraries(simpleShaderExample)
set_target_properties(simpleShaderExample PROPERTIES FOLDER "Examples")
