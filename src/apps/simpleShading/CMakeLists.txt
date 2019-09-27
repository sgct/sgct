# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(simpleShading main.cpp)
set_compile_options(simpleShading)
target_link_libraries(simpleShading PRIVATE sgct)

copy_sgct_dynamic_libraries(simpleShading)
set_target_properties(simpleShading PROPERTIES FOLDER "Examples")
