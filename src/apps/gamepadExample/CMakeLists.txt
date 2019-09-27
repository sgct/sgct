# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(gamepadExample main.cpp)
set_compile_options(gamepadExample)
target_link_libraries(gamepadExample PRIVATE sgct)
copy_sgct_dynamic_libraries(gamepadExample)
set_target_properties(gamepadExample PROPERTIES FOLDER "Examples")
