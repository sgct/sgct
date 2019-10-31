# Copyright Linkoping University 2011
# SGCT Project Authors see Authors.txt

add_executable(test-runner main.cpp)
set_compile_options(test-runner)
target_link_libraries(test-runner PRIVATE sgct)
copy_sgct_dynamic_libraries(test-runner)
set_target_properties(test-runner PROPERTIES FOLDER "Examples")
