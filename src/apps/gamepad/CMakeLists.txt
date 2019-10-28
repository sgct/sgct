# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(gamepad main.cpp)
set_compile_options(gamepad)
target_link_libraries(gamepad PRIVATE sgct)
copy_sgct_dynamic_libraries(gamepad)
set_target_properties(gamepad PROPERTIES FOLDER "Examples")
